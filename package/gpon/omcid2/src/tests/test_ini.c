/*
 * tests/test_ini.c — seed MIB loader tests against the reference profile
 *
 * Fixture: tests/data/custom_mib_gist.ini — a complete public custom
 * MIB for the Lantiq stick family (Spektykles gist; stick-format data,
 * see specs/interface-b.md).
 *
 * omcid2 — SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <stdio.h>
#include <string.h>

#include "omci/mib.h"
#include "platform/ini.h"

static int g_pass, g_fail;

#define CHECK(cond, name) do {                                    \
    if (cond) { g_pass++; }                                       \
    else      { g_fail++; printf("FAIL: %s (line %d)\n", name, __LINE__); } \
} while (0)

#define CHECK_EQ(a, b, name) CHECK((a) == (b), name)

static void warn_log(const char *msg, void *ud)
{
    (void)ud;
    printf("  ini warn: %s\n", msg);
}

static int emit_capture(const char *line, void *ud)
{
    char *dst = ud;
    size_t used = strlen(dst);
    snprintf(dst + used, 8192 - used, "%s\n", line);
    return 0;
}

int main(void)
{
    struct mib m;
    int n;
    uint8_t buf[MIB_ATTRS_MAX];
    size_t len;

    /* ---- load the reference profile ------------------------------- */
    mib_init(&m);
    n = ini_load_file("tests/data/custom_mib_gist.ini", &m, warn_log, NULL);
    CHECK(n > 0, "profile loaded");
    printf("  instances seeded: %d\n", n);

    /* every line in the fixture must be a known class (no warnings above);
     * spot-check the expected contents */

    /* ME 256 instance 0: VNPT / XSW250-NSv3 + NULs / serial / TMO=2 */
    CHECK(mib_find(&m, 256, 0) != NULL, "ME 256 inst 0 exists");
    CHECK_EQ(mib_get(&m, 256, 0, 0x8000, buf, sizeof(buf), &len),
             OMCI_RES_SUCCESS, "get ME256 vendor");
    CHECK_EQ(len, 4, "vendor id size");
    CHECK(memcmp(buf, "VNPT", 4) == 0, "vendor id value");

    CHECK_EQ(mib_get(&m, 256, 0, 0x4000, buf, sizeof(buf), &len),
             OMCI_RES_SUCCESS, "get ME256 version");
    CHECK_EQ(len, 14, "version size");
    CHECK(memcmp(buf, "XSW250-NSv3\0\0\0", 14) == 0, "version value");

    CHECK_EQ(mib_get(&m, 256, 0, 0x1000, buf, sizeof(buf), &len),
             OMCI_RES_SUCCESS, "get ME256 tmo");
    CHECK_EQ(len, 1, "tmo size");
    CHECK_EQ(buf[0], 2, "traffic management option = 2");

    /* ME 2 ONU data: exists, mib data sync 0 after seeding */
    CHECK(mib_find(&m, 2, 0) != NULL, "ME 2 exists");
    CHECK_EQ(mib_data_sync_get(&m), 0, "mib data sync 0 after seed");

    /* ME 7 software image: two instances, no attrs in file */
    CHECK(mib_find(&m, 7, 0) != NULL, "ME 7 inst 0");
    CHECK(mib_find(&m, 7, 1) != NULL, "ME 7 inst 1");

    /* ME 262 T-CONT: 16 instances 0x8000..0x800f, alloc id 0xff */
    CHECK(mib_find(&m, 262, 0x8000) != NULL, "T-CONT 0x8000");
    CHECK_EQ(mib_get(&m, 262, 0x800f, 0x8000, buf, sizeof(buf), &len),
             OMCI_RES_SUCCESS, "get T-CONT alloc");
    CHECK_EQ(len, 2, "alloc id size");
    CHECK_EQ((buf[0] << 8) | buf[1], 0x00FF, "alloc id value");

    /* ME 277 priority queue 0x8000: table slot parsed from \xHH escapes */
    CHECK(mib_find(&m, 277, 0x8000) != NULL, "PQ 0x8000 exists");
    CHECK_EQ(mib_get(&m, 277, 0x8000, 0x0008, buf, sizeof(buf), &len),
             OMCI_RES_SUCCESS, "get PQ drop thresholds");
    CHECK_EQ(len, 8, "drop threshold slot size");
    CHECK_EQ(buf[0], 0x35, "US drop threshold byte0");
    CHECK_EQ(buf[1], 0x56, "US drop threshold byte1");

    /* ME 257 ONU2-G numeric attrs */
    CHECK_EQ(mib_get(&m, 257, 0, 0x2000, buf, sizeof(buf), &len),
             OMCI_RES_SUCCESS, "get ME257 vendor product code");
    CHECK_EQ((buf[0] << 8) | buf[1], 0x00CC, "vendor product code");

    /* ---- upload ----------------------------------------------------- */
    {
        unsigned total = mib_upload_num(&m);
        uint16_t cls, inst, mask;
        unsigned i;

        CHECK(total > 100, "upload image count plausible");
        printf("  upload images: %u (instances: %u)\n", total, m.count);

        /* first image must be ME 2 (lowest class) with its 1 attr */
        CHECK_EQ(mib_upload_next(&m, 0, &cls, &inst, &mask,
                                 buf, sizeof(buf), &len), OMCI_RES_SUCCESS,
                 "upload image 0");
        CHECK_EQ(cls, 2, "first image is ME 2");
        CHECK_EQ(inst, 0, "first image inst 0");
        CHECK_EQ(mask, 0x8000, "ME 2 upload mask");
        CHECK_EQ(len, 1, "ME 2 upload attr len");

        /* walk all images: every one must fit the 26-byte budget */
        for (i = 0; i < total; i++) {
            CHECK_EQ(mib_upload_next(&m, i, &cls, &inst, &mask,
                                     buf, sizeof(buf), &len),
                     OMCI_RES_SUCCESS, "upload walk");
            CHECK(len <= 26, "image fits budget");
            CHECK(mask != 0, "image has attrs");
        }
        CHECK_EQ(mib_upload_next(&m, total, &cls, &inst, &mask,
                                 buf, sizeof(buf), &len),
                 OMCI_RES_UNKNOWN_INSTANCE, "upload past end");
    }

    /* ME 256 area is 71 bytes -> must split over multiple images */
    {
        unsigned total = mib_upload_num(&m);
        uint16_t cls, inst, mask;
        unsigned i, me256_images = 0;

        for (i = 0; i < total; i++) {
            mib_upload_next(&m, i, &cls, &inst, &mask, buf, sizeof(buf), &len);
            if (cls == 256 && inst == 0)
                me256_images++;
        }
        CHECK(me256_images >= 2, "ME 256 splits into >=2 images");
        printf("  ME 256 upload images: %u\n", me256_images);
    }

    /* ---- meg dump ---------------------------------------------------- */
    {
        static char out[8192];

        CHECK_EQ(mib_dump_me(&m, 256, 0, emit_capture, out), OMCI_RES_SUCCESS,
                 "dump ME 256");
        CHECK(strncmp(out, "Class ID    = 256 (ONU-G)\n", 26) == 0,
              "dump header line");
        CHECK(strstr(out, "Instance ID = 0\n") != NULL, "dump instance line");
        CHECK(strstr(out, "Upload      = yes\n") != NULL, "dump upload line");
        CHECK(strstr(out, " 1 VendorId") != NULL, "attr 1 name");
        CHECK(strstr(out, " 0x56 0x4e 0x50 0x54") != NULL, "attr 1 hex");
        CHECK(strstr(out, "VNPT") != NULL, "attr 1 ascii");
        CHECK(strstr(out, "R - Readable") != NULL, "legend present");
        CHECK(strstr(out, "errorcode") == NULL,
              "no errorcode inside dump (CLI adds it)");

        /* unknown instance error */
        CHECK_EQ(mib_dump_me(&m, 999, 0, emit_capture, out),
                 OMCI_RES_UNKNOWN_ME, "dump unknown class");
        CHECK_EQ(mib_dump_me(&m, 256, 77, emit_capture, out),
                 OMCI_RES_UNKNOWN_INSTANCE, "dump unknown instance");
    }

    /* ---- create/set/get round trip ----------------------------------- */
    {
        uint16_t next;
        uint8_t setv[8];
        uint8_t getv[8];

        CHECK_EQ(mib_create(&m, 268, 0x0100, NULL, 0), OMCI_RES_SUCCESS,
                 "create GEM CTP");
        CHECK_EQ(mib_create(&m, 268, 0x0100, NULL, 0),
                 OMCI_RES_INSTANCE_EXISTS, "duplicate create");

        /* T-CONT pointer (mask 0x4000) = 0x8000 */
        setv[0] = 0x80; setv[1] = 0x00;
        CHECK_EQ(mib_set(&m, 268, 0x0100, 0x4000, setv, 2), OMCI_RES_SUCCESS,
                 "set GEM CTP tcont ptr");
        CHECK_EQ(mib_get(&m, 268, 0x0100, 0x4000, getv, sizeof(getv), &len),
                 OMCI_RES_SUCCESS, "get GEM CTP tcont ptr");
        CHECK_EQ(len, 2, "tcont ptr size");
        CHECK(memcmp(setv, getv, 2) == 0, "tcont ptr round trip");

        /* data sync increments on set */
        {
            uint8_t before = mib_data_sync_get(&m);
            mib_set(&m, 268, 0x0100, 0x4000, setv, 2);
            CHECK_EQ(mib_data_sync_get(&m),
                     (uint8_t)(before + 1), "data sync bumped");
        }

        /* get-next walk over T-CONTs */
        CHECK_EQ(mib_get_next(&m, 262, 0x8000, 0x8000, getv, sizeof(getv),
                              &len, &next), OMCI_RES_SUCCESS, "getnext");
        CHECK_EQ(next, 0x8001, "getnext finds next tcont");
        CHECK_EQ(mib_get_next(&m, 262, 0x800f, 0x8000, getv, sizeof(getv),
                              &len, &next), OMCI_RES_SUCCESS, "getnext wrap");
        CHECK_EQ(next, 0x8000, "getnext wraps to first");

        CHECK_EQ(mib_delete(&m, 268, 0x0100), OMCI_RES_SUCCESS, "delete");
        CHECK_EQ(mib_delete(&m, 268, 0x0100), OMCI_RES_UNKNOWN_INSTANCE,
                 "delete again");
        CHECK_EQ(mib_get(&m, 268, 0x0100, 0x4000, getv, sizeof(getv), &len),
                 OMCI_RES_UNKNOWN_INSTANCE, "get after delete");
    }

    mib_free(&m);
    printf("ini/mib tests: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
