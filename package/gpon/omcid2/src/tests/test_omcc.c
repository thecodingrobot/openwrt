/*
 * tests/test_omcc.c — OMCC engine + driver mock + simulated OLT scenario
 *
 * M3 acceptance: full O5 MIB upload sequence against a simulated OLT.
 * The "daemon" under test is the real core: activation tracker + OMCC
 * engine + MIB + codec, connected to a loopback driver (interface A
 * test double) instead of /dev/onu.
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

static int g_pass, g_fail;

#define CHECK(cond, name) do {                                    \
    if (cond) { g_pass++; }                                       \
    else      { g_fail++; printf("FAIL: %s (line %d)\n", name, __LINE__); } \
} while (0)

#define CHECK_EQ(a, b, name) CHECK((a) == (b), name)

/* ---- the daemon core under test ----------------------------------------- */

struct daemon {
    struct activation act;
    struct omcc omcc;
    const struct drv_ops *drv;
    void *drv_ctx;
};

static void pump(void *ud)
{
    struct daemon *d = ud;
    struct drv_event ev;

    while (d->drv->event_get(d->drv_ctx, &ev) == 0) {
        activation_event(&d->act, &ev);
        if (ev.id == DRV_EVENT_OMCI_RECEIVE)
            omcc_rx_pdu(&d->omcc, ev.u.omci.pdu, ev.u.omci.len);
    }
}

static int seed(struct mib *m)
{
    return ini_load_file("tests/data/custom_mib_gist.ini", m, NULL, NULL);
}

int main(void)
{
    struct daemon d;
    struct olt_sim olt;
    struct mib m;
    struct drv_config cfg = {
        .mib_file = "tests/data/custom_mib_gist.ini",
        .image0_version = "SIM0.0.1",
        .image1_version = NULL,
        .omcc_version = 0x80,
    };
    unsigned num;
    uint8_t attrs[OMCI_CONTENT_MAX];
    size_t attrs_len;
    unsigned per_class[512];
    unsigned expected_images;

    /* ---- boot: driver, MIB seed, OMCC ------------------------------ */
    CHECK_EQ(drv_mock_open(&d.drv, &d.drv_ctx), 0, "mock driver open");
    CHECK_EQ(d.drv->init(d.drv_ctx, &cfg), 0, "driver init");

    mib_init(&m);
    CHECK(seed(&m) > 0, "MIB seeded");

    omcc_init(&d.omcc, &m, d.drv, d.drv_ctx);
    activation_init(&d.act);

    /* ---- activation to O5 ------------------------------------------ */
    drv_mock_inject_state(d.drv_ctx, 0, DRV_PLOAM_O1); pump(&d);
    drv_mock_inject_state(d.drv_ctx, DRV_PLOAM_O1, DRV_PLOAM_O2); pump(&d);
    drv_mock_inject_state(d.drv_ctx, DRV_PLOAM_O2, DRV_PLOAM_O3); pump(&d);
    drv_mock_inject_state(d.drv_ctx, DRV_PLOAM_O3, DRV_PLOAM_O4); pump(&d);
    CHECK(!activation_is_o5(&d.act), "not yet O5");
    drv_mock_inject_state(d.drv_ctx, DRV_PLOAM_O4, DRV_PLOAM_O5); pump(&d);
    CHECK(activation_is_o5(&d.act), "O5 reached");
    CHECK(drv_mock_tx_count(d.drv_ctx) == 0, "no TX before OLT dialog");

    /* ---- OLT: full MIB upload session ------------------------------- */
    olt_init(&olt, d.drv_ctx, pump, &d);
    memset(per_class, 0, sizeof(per_class));
    expected_images = mib_upload_num(&m);

    num = olt_full_upload_session(&olt, pump, &d, NULL, per_class, 512);
    CHECK_EQ(num, expected_images, "upload image count matches MIB");
    CHECK_EQ(olt.err_count, 0, "no upload errors");
    CHECK(num > 300, "large upload collected");
    CHECK_EQ(per_class[2], 1, "one ME 2 image");
    CHECK_EQ(per_class[256], 4, "ME 256 split into 4 images");
    CHECK_EQ(per_class[262], 16, "16 T-CONT images");
    CHECK_EQ(per_class[277], 272,
             "136 priority queues, 2 images each (36 > 26 bytes)");
    printf("  O5 upload session: %u images (%u frames TX)\n",
           num, drv_mock_tx_count(d.drv_ctx));

    /* ---- Get / Set ---------------------------------------------------- */
    CHECK_EQ(olt_get(&olt, pump, &d, 256, 0, 0x8000, attrs, sizeof(attrs),
                     &attrs_len), OMCI_RES_SUCCESS, "get ME 256 vendor");
    CHECK_EQ(attrs_len, 4, "vendor len");
    CHECK(memcmp(attrs, "VNPT", 4) == 0, "vendor value");
    CHECK_EQ(olt.last_rsp.tc, olt.tc, "response TC echoes request");

    CHECK_EQ(olt_get(&olt, pump, &d, 256, 0, 0xC000, attrs, sizeof(attrs),
                     &attrs_len), OMCI_RES_SUCCESS,
             "get attrs 1+2 (18 bytes) fits");
    CHECK_EQ(attrs_len, 18, "attrs 1+2 packed length");
    CHECK_EQ(olt_get(&olt, pump, &d, 256, 0, 0xE040, attrs, sizeof(attrs),
                     &attrs_len), OMCI_RES_PARAM_ERROR,
             "oversized get mask rejected (50 > 29 bytes)");

    attrs[0] = 1;
    CHECK_EQ(olt_set(&olt, pump, &d, 256, 0, 0x0400, attrs, 1),
             OMCI_RES_SUCCESS, "set ME 256 battery backup");
    CHECK_EQ(olt_get(&olt, pump, &d, 256, 0, 0x0400, attrs, sizeof(attrs),
                     &attrs_len), OMCI_RES_SUCCESS, "get battery backup");
    CHECK_EQ(attrs[0], 1, "battery backup value");

    /* data sync must advance with successful Sets */
    {
        uint8_t sync[1];
        CHECK_EQ(olt_get(&olt, pump, &d, 2, 0, 0x8000, sync, 1, &attrs_len),
                 OMCI_RES_SUCCESS, "get mib data sync");
        CHECK(sync[0] > 0, "mib data sync advanced");
    }

    /* ---- Create / GetNext / Delete ------------------------------------ */
    {
        uint8_t create_attrs[2] = { 0x01, 0x00 };
        uint16_t next;

        CHECK_EQ(olt_create(&olt, pump, &d, 262, 0x8010, create_attrs, 2),
                 OMCI_RES_SUCCESS, "create T-CONT 0x8010");
        CHECK_EQ(olt_create(&olt, pump, &d, 262, 0x8010, create_attrs, 2),
                 OMCI_RES_INSTANCE_EXISTS, "duplicate create rejected");

        CHECK_EQ(olt_get(&olt, pump, &d, 262, 0x8010, 0x8000,
                         attrs, sizeof(attrs), &attrs_len),
                 OMCI_RES_SUCCESS, "get new tcont alloc");
        CHECK_EQ(attrs_len, 2, "alloc len");
        CHECK_EQ((attrs[0] << 8) | attrs[1], 0x0100, "alloc value");

        CHECK_EQ(olt_getnext(&olt, pump, &d, 262, 0x800f, 0x8000,
                             attrs, sizeof(attrs), &attrs_len, &next),
                 OMCI_RES_SUCCESS, "getnext finds created tcont");
        CHECK_EQ(next, 0x8010, "getnext instance");

        CHECK_EQ(olt_delete(&olt, pump, &d, 262, 0x8010),
                 OMCI_RES_SUCCESS, "delete tcont");
        CHECK_EQ(olt_get(&olt, pump, &d, 262, 0x8010, 0x8000,
                         attrs, sizeof(attrs), &attrs_len),
                 OMCI_RES_UNKNOWN_INSTANCE, "get deleted tcont");
    }

    /* ---- error paths ---------------------------------------------------- */
    CHECK_EQ(olt_get(&olt, pump, &d, 999, 0, 0x8000, attrs, sizeof(attrs),
                     &attrs_len), OMCI_RES_UNKNOWN_ME, "unknown ME class");
    CHECK_EQ(olt_get(&olt, pump, &d, 256, 0x7777, 0x8000,
                     attrs, sizeof(attrs), &attrs_len),
             OMCI_RES_UNKNOWN_INSTANCE, "unknown instance");

    /* ---- MIB reset ------------------------------------------------------ */
    CHECK_EQ(olt_mib_reset(&olt, pump, &d), OMCI_RES_SUCCESS, "mib reset");
    CHECK_EQ(olt_mib_upload(&olt, pump, &d, &num), OMCI_RES_SUCCESS,
             "upload after reset");
    CHECK_EQ(num, 1, "empty MIB has only ME 2");
    CHECK_EQ(mib_data_sync_get(&m), 0, "data sync cleared");
    CHECK_EQ(olt_get(&olt, pump, &d, 256, 0, 0x8000, attrs, sizeof(attrs),
                     &attrs_len), OMCI_RES_UNKNOWN_INSTANCE,
             "ME 256 gone after reset");

    /* reseed and re-upload (procd-style restart simulation) */
    CHECK(seed(&m) > 0, "MIB reseeded");
    num = olt_full_upload_session(&olt, pump, &d, NULL, per_class, 512);
    CHECK_EQ(num, expected_images, "reseeded upload identical");

    /* ---- autonomous AVC ------------------------------------------------- */
    {
        uint8_t val[2] = { 0x00, 0x64 };
        unsigned tx_before = drv_mock_tx_count(d.drv_ctx);

        CHECK_EQ(omcc_send_avc(&d.omcc, 11, 0x0101, 0x0800, val, 2),
                 OMCI_RES_SUCCESS, "send AVC");
        CHECK_EQ(drv_mock_tx_count(d.drv_ctx), tx_before + 1, "AVC on wire");
        CHECK_EQ(olt.rsp_count, tx_before + 1, "OLT captured AVC");
        CHECK_EQ(omci_mt(&olt.last_rsp), OMCI_MT_AVC, "captured type AVC");
        CHECK(!omci_ak(&olt.last_rsp), "AVC has no AK");
        CHECK_EQ(olt.last_rsp.tc, 0, "AVC uses TID 0");
        CHECK_EQ(olt.last_rsp.attr_mask, 0x0800, "AVC mask");
    }

    /* ---- stats ----------------------------------------------------------- */
    CHECK(d.omcc.rx_count > 350, "omcc processed all requests");
    CHECK_EQ(d.omcc.tx_count, drv_mock_tx_count(d.drv_ctx),
             "every TX counted");
    printf("  omcc stats: rx=%u tx=%u\n", d.omcc.rx_count, d.omcc.tx_count);

    /* ---- driver close ---------------------------------------------------- */
    d.drv->close(d.drv_ctx);
    mib_free(&m);

    printf("omcc/sim tests: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
