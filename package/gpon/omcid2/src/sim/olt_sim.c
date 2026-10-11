/*
 * sim/olt_sim.c — simulated OLT implementation
 *
 * omcid2 — clean-room OMCI daemon for Lantiq Falcon GPON sticks
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "olt_sim.h"

#include <stdio.h>
#include <string.h>

#include "drv_mock.h"
#include "omci/me_defs.h"

static void olt_capture_tx(const uint8_t *pdu, uint16_t len, void *ud)
{
    struct olt_sim *olt = ud;

    if (len >= OMCI_HDR_LEN &&
        omci_decode(pdu, len, &olt->last_rsp) == OMCI_OK)
        olt->rsp_count++;
}

/* Meaningful attribute length for a mask: response frames are padded
 * to 32 contents bytes, a real OLT sizes the payload from the ME
 * schema (mask bits -> attribute sizes). */
static size_t attrs_len_for_mask(uint16_t cls, uint16_t mask,
                                 size_t content_len)
{
    const struct omci_class_def *cd = omci_class_find(cls);
    int n;

    if (!cd)
        return content_len;
    n = omci_mask_packed_len(cd, mask);
    if (n < 0 || (size_t)n > content_len)
        return content_len;
    return (size_t)n;
}

int olt_init(struct olt_sim *olt, void *mock_ctx, olt_pump_fn pump,
              void *pump_ud)
{
    memset(olt, 0, sizeof(*olt));
    olt->mock_ctx = mock_ctx;
    drv_mock_set_tx_hook(mock_ctx, olt_capture_tx, olt);
    if (pump)
        pump(pump_ud);
    return 0;
}

/* Build+send a request, pump the daemon, decode the response. */
static int olt_xchg(struct olt_sim *olt, olt_pump_fn pump, void *pump_ud,
                    struct omci_msg *req)
{
    uint8_t frame[OMCI_FRAME_BASELINE_LEN];
    unsigned before;
    int n;

    req->tc = (uint16_t)++olt->tc;
    req->msg_type = (uint8_t)(omci_mt(req) | OMCI_F_AR);
    n = omci_encode(req, frame, sizeof(frame));
    if (n != OMCI_FRAME_BASELINE_LEN)
        return OMCI_RES_PROC_ERROR;

    before = olt->rsp_count;
    drv_mock_inject_omci(olt->mock_ctx, frame, (uint16_t)n);
    pump(pump_ud);

    if (olt->rsp_count == before) {
        olt->err_count++;
        return OMCI_RES_PROC_ERROR;
    }
    return olt->last_rsp.result;
}

int olt_get(struct olt_sim *olt, olt_pump_fn pump, void *pump_ud,
            uint16_t cls, uint16_t inst, uint16_t mask,
            uint8_t *attrs, size_t cap, size_t *attrs_len)
{
    struct omci_msg req;
    int rc;

    memset(&req, 0, sizeof(req));
    req.msg_type = OMCI_MT_GET;
    req.me_class = cls;
    req.me_instance = inst;
    req.attr_mask = mask;
    rc = olt_xchg(olt, pump, pump_ud, &req);
    if (rc == OMCI_RES_SUCCESS && attrs) {
        size_t n = attrs_len_for_mask(cls, mask,
                                      olt->last_rsp.content_len);
        if (n > cap)
            n = cap;
        memcpy(attrs, olt->last_rsp.content, n);
        if (attrs_len)
            *attrs_len = n;
    }
    return rc;
}

int olt_set(struct olt_sim *olt, olt_pump_fn pump, void *pump_ud,
            uint16_t cls, uint16_t inst, uint16_t mask,
            const uint8_t *val, uint16_t len)
{
    struct omci_msg req;

    memset(&req, 0, sizeof(req));
    req.msg_type = OMCI_MT_SET;
    req.me_class = cls;
    req.me_instance = inst;
    req.attr_mask = mask;
    if (len) {
        if (len > OMCI_CONTENT_MAX - 2)
            return OMCI_RES_PARAM_ERROR;
        memcpy(req.content, val, len);
        req.content_len = len;
    }
    return olt_xchg(olt, pump, pump_ud, &req);
}

int olt_create(struct olt_sim *olt, olt_pump_fn pump, void *pump_ud,
               uint16_t cls, uint16_t inst,
               const uint8_t *attrs, uint16_t len)
{
    struct omci_msg req;

    memset(&req, 0, sizeof(req));
    req.msg_type = OMCI_MT_CREATE;
    req.me_class = cls;
    req.me_instance = inst;
    if (len) {
        if (len > OMCI_CONTENT_MAX)
            return OMCI_RES_PARAM_ERROR;
        memcpy(req.content, attrs, len);
        req.content_len = len;
    }
    return olt_xchg(olt, pump, pump_ud, &req);
}

int olt_delete(struct olt_sim *olt, olt_pump_fn pump, void *pump_ud,
               uint16_t cls, uint16_t inst)
{
    struct omci_msg req;

    memset(&req, 0, sizeof(req));
    req.msg_type = OMCI_MT_DELETE;
    req.me_class = cls;
    req.me_instance = inst;
    return olt_xchg(olt, pump, pump_ud, &req);
}

int olt_getnext(struct olt_sim *olt, olt_pump_fn pump, void *pump_ud,
                uint16_t cls, uint16_t inst, uint16_t mask,
                uint8_t *attrs, size_t cap, size_t *attrs_len,
                uint16_t *next_inst)
{
    struct omci_msg req;
    int rc;

    memset(&req, 0, sizeof(req));
    req.msg_type = OMCI_MT_GET_NEXT;
    req.me_class = cls;
    req.me_instance = inst;
    req.attr_mask = mask;
    req.seq = 0;
    rc = olt_xchg(olt, pump, pump_ud, &req);
    if (rc == OMCI_RES_SUCCESS) {
        /* response: mask-selected attrs + 2-byte next-instance;
         * size the attribute part from the schema */
        size_t n = attrs_len_for_mask(cls, mask,
                                      olt->last_rsp.content_len);
        if (n > cap)
            n = cap;
        if (attrs) {
            memcpy(attrs, olt->last_rsp.content, n);
            if (attrs_len)
                *attrs_len = n;
        }
        if (next_inst) {
            const uint8_t *p = &olt->last_rsp.content[n];
            if (n + 2 <= olt->last_rsp.content_len)
                *next_inst = (uint16_t)((p[0] << 8) | p[1]);
            else
                *next_inst = 0;
        }
    }
    return rc;
}

int olt_mib_upload(struct olt_sim *olt, olt_pump_fn pump, void *pump_ud,
                   unsigned *num_images)
{
    struct omci_msg req;
    int rc;

    memset(&req, 0, sizeof(req));
    req.msg_type = OMCI_MT_MIB_UPLOAD;
    req.me_class = 2;
    req.me_instance = 0;
    rc = olt_xchg(olt, pump, pump_ud, &req);
    if (rc == OMCI_RES_SUCCESS && num_images)
        *num_images = olt->last_rsp.num_cmds;
    return rc;
}

int olt_mib_upload_next(struct olt_sim *olt, olt_pump_fn pump, void *ud,
                        unsigned seq, uint16_t *cls, uint16_t *inst,
                        uint16_t *mask, uint8_t *attrs, size_t cap,
                        size_t *attrs_len)
{
    struct omci_msg req;
    int rc;

    memset(&req, 0, sizeof(req));
    req.msg_type = OMCI_MT_MIB_UPLOAD_NEXT;
    req.me_class = 2;
    req.me_instance = 0;
    req.seq = (uint16_t)seq;
    rc = olt_xchg(olt, pump, ud, &req);
    if (rc == OMCI_RES_SUCCESS) {
        uint16_t rcls = olt->last_rsp.reported_class;
        uint16_t rmask = olt->last_rsp.attr_mask;
        if (cls)  *cls  = rcls;
        if (inst) *inst = olt->last_rsp.reported_instance;
        if (mask) *mask = rmask;
        if (attrs) {
            size_t n = attrs_len_for_mask(rcls, rmask,
                                          olt->last_rsp.content_len);
            if (n > cap)
                n = cap;
            memcpy(attrs, olt->last_rsp.content, n);
            if (attrs_len)
                *attrs_len = n;
        }
    }
    return rc;
}

int olt_mib_reset(struct olt_sim *olt, olt_pump_fn pump, void *pump_ud)
{
    struct omci_msg req;

    memset(&req, 0, sizeof(req));
    req.msg_type = OMCI_MT_MIB_RESET;
    req.me_class = 2;
    req.me_instance = 0;
    return olt_xchg(olt, pump, pump_ud, &req);
}

unsigned olt_full_upload_session(struct olt_sim *olt, olt_pump_fn pump,
                                 void *pump_ud, FILE *log,
                                 unsigned *per_class_stats,
                                 size_t cls_max)
{
    unsigned total = 0, seq;
    int rc;

    rc = olt_mib_upload(olt, pump, pump_ud, &total);
    if (rc != OMCI_RES_SUCCESS)
        return 0;

    if (log)
        fprintf(log, "OLT: MIB upload -> %u images\n", total);

    for (seq = 0; seq < total; seq++) {
        uint16_t cls, inst, mask;
        uint8_t attrs[OMCI_CONTENT_MAX];
        size_t attrs_len;

        rc = olt_mib_upload_next(olt, pump, pump_ud, seq, &cls, &inst,
                                 &mask, attrs, sizeof(attrs), &attrs_len);
        if (rc != OMCI_RES_SUCCESS) {
            olt->err_count++;
            if (log)
                fprintf(log, "OLT: upload next %u FAILED (%d)\n", seq, rc);
            break;
        }
        if (log && seq < 8)
            fprintf(log,
                    "OLT: upload next %-3u -> ME %u inst 0x%04x mask 0x%04x (%zu bytes)\n",
                    seq, cls, inst, mask, attrs_len);
        else if (log && seq + 1 == total)
            fprintf(log, "OLT: ... %u images collected\n", total);
        if (per_class_stats && cls < cls_max)
            per_class_stats[cls]++;
    }
    return total;
}
