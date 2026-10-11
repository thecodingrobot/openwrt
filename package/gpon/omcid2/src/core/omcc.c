/*
 * core/omcc.c — OMCC engine implementation
 *
 * omcid2 — clean-room OMCI daemon for Lantiq Falcon GPON sticks
 *
 * Request dispatch table follows G.988 message semantics (see
 * omci/codec.c layout table). Notes/simplifications for M3:
 *  - Create request contents are applied as the initial packed
 *    attribute area (set-by-create attributes lead the schema order
 *    for the MEs used so far).
 *  - Set-by-reference is a legacy G.984.4 message; answered with
 *    "command not supported".
 *  - Get with an attribute mask exceeding the 29-byte baseline budget
 *    is answered with a parameter error.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "omcc.h"

#include <string.h>

int omcc_init(struct omcc *o, struct mib *mib,
              const struct drv_ops *drv, void *drv_ctx)
{
    memset(o, 0, sizeof(*o));
    o->mib = mib;
    o->drv = drv;
    o->drv_ctx = drv_ctx;
    return 0;
}

static int omcc_tx(struct omcc *o, struct omci_msg *rsp)
{
    uint8_t frame[OMCI_FRAME_BASELINE_LEN];
    int n;

    n = omci_encode(rsp, frame, sizeof(frame));
    if (n != OMCI_FRAME_BASELINE_LEN)
        return OMCI_RES_PROC_ERROR;
    if (o->drv->omci_send(o->drv_ctx, frame, (uint16_t)n) != 0)
        return OMCI_RES_PROC_ERROR;
    o->tx_count++;
    return OMCI_RES_SUCCESS;
}

static int build_rsp(struct omci_msg *rsp, const struct omci_msg *req,
                     int result)
{
    memset(rsp, 0, sizeof(*rsp));
    rsp->tc = req->tc;
    rsp->msg_type = (uint8_t)((req->msg_type & OMCI_MT_MASK) | OMCI_F_AK);
    rsp->me_class = req->me_class;
    rsp->me_instance = req->me_instance;
    rsp->result = (uint8_t)result;
    return result;
}

int omcc_rx_pdu(struct omcc *o, const uint8_t *pdu, size_t len)
{
    struct omci_msg req, rsp;
    uint8_t out[OMCI_CONTENT_MAX];
    size_t out_len;
    int rc;

    rc = omci_decode(pdu, len, &req);
    if (rc != OMCI_OK)
        return OMCI_RES_PROC_ERROR;

    if (req.dev_id != OMCI_DEV_BASELINE ||
        omci_kind(&req) != OMCI_K_REQ)
        return OMCI_RES_PROC_ERROR;

    o->rx_count++;

    switch (omci_mt(&req)) {
    case OMCI_MT_CREATE:
        /* contents = set-by-create attribute values */
        rc = mib_create(o->mib, req.me_class, req.me_instance,
                        req.content, req.content_len);
        break;

    case OMCI_MT_DELETE:
        rc = mib_delete(o->mib, req.me_class, req.me_instance);
        break;

    case OMCI_MT_SET:
        rc = mib_set(o->mib, req.me_class, req.me_instance,
                     req.attr_mask, req.content, req.content_len);
        break;

    case OMCI_MT_SET_BY_REF:
        rc = OMCI_RES_NOT_SUPPORTED;
        break;

    case OMCI_MT_GET:
    case OMCI_MT_GET_CURRENT_DATA:
        rc = mib_get(o->mib, req.me_class, req.me_instance,
                     req.attr_mask, out, OMCC_GET_BUDGET, &out_len);
        if (rc == OMCI_RES_SUCCESS && omci_ar(&req)) {
            build_rsp(&rsp, &req, rc);
            rsp.attr_mask = req.attr_mask;
            if (out_len) {
                memcpy(rsp.content, out, out_len);
                rsp.content_len = (uint16_t)out_len;
            }
            rc = omcc_tx(o, &rsp);
            return rc;
        }
        break;

    case OMCI_MT_GET_NEXT: {
        uint16_t next_inst;

        rc = mib_get_next(o->mib, req.me_class, req.me_instance,
                          req.attr_mask, out, OMCC_GETNEXT_BUDGET,
                          &out_len, &next_inst);
        if (rc == OMCI_RES_SUCCESS && omci_ar(&req)) {
            build_rsp(&rsp, &req, rc);
            rsp.attr_mask = req.attr_mask;
            if (out_len)
                memcpy(rsp.content, out, out_len);
            /* attribute data followed by the next instance id */
            rsp.content[out_len] = (uint8_t)(next_inst >> 8);
            rsp.content[out_len + 1] = (uint8_t)next_inst;
            rsp.content_len = (uint16_t)(out_len + 2);
            rc = omcc_tx(o, &rsp);
            return rc;
        }
        break;
    }

    case OMCI_MT_MIB_UPLOAD:
        rc = OMCI_RES_SUCCESS;
        if (omci_ar(&req)) {
            build_rsp(&rsp, &req, rc);
            rsp.num_cmds = (uint16_t)mib_upload_num(o->mib);
            rc = omcc_tx(o, &rsp);
            return rc;
        }
        break;

    case OMCI_MT_MIB_UPLOAD_NEXT: {
        uint16_t cls, inst, mask;

        rc = mib_upload_next(o->mib, req.seq, &cls, &inst, &mask,
                             out, OMCI_CONTENT_MAX - 6, &out_len);
        if (omci_ar(&req)) {
            build_rsp(&rsp, &req, OMCI_RES_SUCCESS);
            if (rc == OMCI_RES_SUCCESS) {
                rsp.reported_class = cls;
                rsp.reported_instance = inst;
                rsp.attr_mask = mask;
                if (out_len) {
                    memcpy(rsp.content, out, out_len);
                    rsp.content_len = (uint16_t)out_len;
                }
            } else {
                /* out-of-range sequence: report a null ME */
                rsp.reported_class = 0;
                rsp.reported_instance = 0;
                rsp.attr_mask = 0;
                rsp.content_len = 0;
            }
            rc = omcc_tx(o, &rsp);
            return rc;
        }
        break;
    }

    case OMCI_MT_MIB_RESET:
        mib_reset(o->mib);
        /* the empty MIB still carries ME 2 with a zero data sync */
        mib_create(o->mib, 2, 0, NULL, 0);
        mib_data_sync_set(o->mib, 0);
        rc = OMCI_RES_SUCCESS;
        break;

    case OMCI_MT_TEST:
    case OMCI_MT_START_SW_DOWNLOAD:
    case OMCI_MT_DOWNLOAD_SECTION:
    case OMCI_MT_END_SW_DOWNLOAD:
    case OMCI_MT_ACTIVATE_SW:
    case OMCI_MT_COMMIT_SW:
    case OMCI_MT_SYNC_TIME:
    case OMCI_MT_REBOOT:
        rc = OMCI_RES_NOT_SUPPORTED;
        break;

    default:
        rc = OMCI_RES_NOT_SUPPORTED;
        break;
    }

    if (omci_ar(&req)) {
        build_rsp(&rsp, &req, rc);
        rc = omcc_tx(o, &rsp);
    }
    return rc;
}

int omcc_send_avc(struct omcc *o, uint16_t cls, uint16_t inst,
                  uint16_t mask, const uint8_t *val, uint16_t len)
{
    struct omci_msg m;

    memset(&m, 0, sizeof(m));
    m.tc = 0;                       /* autonomous: TID 0 */
    m.msg_type = (uint8_t)OMCI_MT_AVC;
    m.me_class = cls;
    m.me_instance = inst;
    m.attr_mask = mask;
    if (len) {
        if (len > OMCI_CONTENT_MAX - 2)
            return OMCI_RES_PARAM_ERROR;
        memcpy(m.content, val, len);
        m.content_len = len;
    }
    return omcc_tx(o, &m);
}
