/*
 * sim/omcid2_sim.c — end-to-end simulator demo (M3 acceptance)
 *
 * Boots the omcid2 core (activation tracker + OMCC engine + MIB +
 * codec) on the loopback driver, walks the ONU to O5 and lets a
 * simulated OLT drive a full MIB upload session plus the common
 * provisioning verbs.
 *
 * Usage: omcid2_sim [seed.ini]   (default: tests/data/custom_mib_gist.ini)
 *
 * omcid2 — SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <stdio.h>
#include <string.h>

#include "core/activation.h"
#include "core/omcc.h"
#include "omci/mib.h"
#include "platform/ini.h"
#include "sim/drv_mock.h"
#include "sim/olt_sim.h"

static int g_fail;

static void pump(void *ud)
{
    struct {
        struct activation act;
        struct omcc omcc;
        const struct drv_ops *drv;
        void *drv_ctx;
    } *d = ud;
    struct drv_event ev;

    while (d->drv->event_get(d->drv_ctx, &ev) == 0) {
        activation_event(&d->act, &ev);
        if (ev.id == DRV_EVENT_OMCI_RECEIVE)
            omcc_rx_pdu(&d->omcc, ev.u.omci.pdu, ev.u.omci.len);
    }
}

static int emit(const char *line, void *ud)
{
    (void)ud;
    printf("%s\n", line);
    return 0;
}

int main(int argc, char **argv)
{
    const char *ini = argc > 1 ? argv[1]
                    : "tests/data/custom_mib_gist.ini";
    struct {
        struct activation act;
        struct omcc omcc;
        const struct drv_ops *drv;
        void *drv_ctx;
    } d;
    struct olt_sim olt;
    struct mib m;
    struct drv_config cfg = {
        .mib_file = ini,
        .image0_version = "SIM0.0.1",
        .omcc_version = 0x80,
        .debug_level = 1,
    };
    unsigned per_class[512];
    unsigned images, expected;
    uint8_t attrs[OMCI_CONTENT_MAX];
    size_t attrs_len;
    int rc;

    printf("omcid2 simulator — seed %s\n\n", ini);

    /* boot */
    if (drv_mock_open(&d.drv, &d.drv_ctx) != 0 ||
        d.drv->init(d.drv_ctx, &cfg) != 0) {
        fprintf(stderr, "driver boot failed\n");
        return 1;
    }
    mib_init(&m);
    rc = ini_load_file(ini, &m, NULL, NULL);
    if (rc <= 0) {
        fprintf(stderr, "MIB seed failed (%d)\n", rc);
        return 1;
    }
    printf("[boot] seeded %d instances from %s\n", rc, ini);
    omcc_init(&d.omcc, &m, d.drv, d.drv_ctx);
    activation_init(&d.act);

    /* activation */
    drv_mock_inject_state(d.drv_ctx, 0, DRV_PLOAM_O1);          pump(&d);
    drv_mock_inject_state(d.drv_ctx, DRV_PLOAM_O1, DRV_PLOAM_O5); pump(&d);
    if (!activation_is_o5(&d.act)) {
        fprintf(stderr, "activation failed\n");
        return 1;
    }
    printf("[act ] PLOAM state: %s — OMCC may open\n\n",
           activation_state_name(d.act.state));

    /* OLT session */
    olt_init(&olt, d.drv_ctx, pump, &d);
    memset(per_class, 0, sizeof(per_class));
    expected = mib_upload_num(&m);
    images = olt_full_upload_session(&olt, pump, &d, stdout,
                                     per_class, 512);
    printf("\n[olt ] upload session: %u/%u images, errors=%u, ",
           images, expected, olt.err_count);
    printf("per-class: ME2=%u ME256=%u ME262=%u ME277=%u\n\n",
           per_class[2], per_class[256], per_class[262], per_class[277]);
    if (images != expected)
        g_fail++;

    /* identity read-back */
    rc = olt_get(&olt, pump, &d, 256, 0, 0x8000, attrs, sizeof(attrs),
                 &attrs_len);
    printf("[olt ] Get ME256 vendor: rc=%d \"%.*s\"\n", rc,
           (int)attrs_len, attrs);
    if (rc != 0)
        g_fail++;

    /* provisioning verbs */
    attrs[0] = 0x01; attrs[1] = 0x00;
    rc = olt_create(&olt, pump, &d, 262, 0x8010, attrs, 2);
    printf("[olt ] Create T-CONT 0x8010 (alloc 0x0100): rc=%d\n", rc);
    if (rc != 0)
        g_fail++;
    attrs[0] = 0x02; attrs[1] = 0x00;
    rc = olt_set(&olt, pump, &d, 262, 0x8010, 0x8000, attrs, 2);
    printf("[olt ] Set T-CONT 0x8010 alloc=0x0200: rc=%d\n", rc);
    if (rc != 0)
        g_fail++;
    rc = olt_get(&olt, pump, &d, 262, 0x8010, 0x8000, attrs, sizeof(attrs),
                 &attrs_len);
    printf("[olt ] Get T-CONT 0x8010 alloc: rc=%d value=0x%02x%02x\n",
           rc, attrs[0], attrs[1]);
    if (rc != 0 || ((attrs[0] << 8) | attrs[1]) != 0x0200)
        g_fail++;
    rc = olt_delete(&olt, pump, &d, 262, 0x8010);
    printf("[olt ] Delete T-CONT 0x8010: rc=%d\n\n", rc);
    if (rc != 0)
        g_fail++;

    /* MIB reset + reseed + re-upload */
    rc = olt_mib_reset(&olt, pump, &d);
    printf("[olt ] MIB reset: rc=%d\n", rc);
    if (rc != 0)
        g_fail++;
    if (ini_load_file(ini, &m, NULL, NULL) <= 0)
        g_fail++;
    images = olt_full_upload_session(&olt, pump, &d, NULL, NULL, 0);
    printf("[olt ] post-reset reseed + upload: %u images\n\n", images);
    if (images != expected)
        g_fail++;

    /* omci_pipe.sh-style dump */
    printf("[pipe] meg 256 0:\n");
    mib_dump_me(&m, 256, 0, emit, NULL);
    printf("errorcode=0\n");

    printf("\n[stat ] omcc rx=%u tx=%u — %s\n", d.omcc.rx_count,
           d.omcc.tx_count, g_fail ? "FAILURES PRESENT" : "all good");
    d.drv->close(d.drv_ctx);
    mib_free(&m);
    return g_fail ? 1 : 0;
}
