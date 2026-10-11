/*
 * sim/olt_sim.h — simulated OLT driving OMCI over the mock driver
 *
 * omcid2 — clean-room OMCI daemon for Lantiq Falcon GPON sticks
 *
 * The OLT builds requests, injects them as ONU-side driver events and
 * collects the response frame the daemon transmits (via the mock TX
 * hook), mimicking a synchronous request/response OLT.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef OMCI2_OLT_SIM_H
#define OMCI2_OLT_SIM_H

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#include "omci/omci.h"
#include "platform/drv_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

struct olt_sim {
    void *mock_ctx;             /* ONU-side mock driver */
    unsigned tc;                /* transaction correlation id */
    struct omci_msg last_rsp;   /* decoded last response */
    unsigned rsp_count;
    /* stats */
    unsigned err_count;
};

/* pump_fn drains the daemon: it must process all pending driver
 * events (call the daemon loop once). */
typedef void (*olt_pump_fn)(void *ud);

int  olt_init(struct olt_sim *olt, void *mock_ctx, olt_pump_fn pump,
              void *pump_ud);

/* Request primitives; all return the OMCI result code. */
int olt_get(struct olt_sim *olt, olt_pump_fn pump, void *pump_ud,
            uint16_t cls, uint16_t inst, uint16_t mask,
            uint8_t *attrs, size_t cap, size_t *attrs_len);
int olt_set(struct olt_sim *olt, olt_pump_fn pump, void *pump_ud,
            uint16_t cls, uint16_t inst, uint16_t mask,
            const uint8_t *val, uint16_t len);
int olt_create(struct olt_sim *olt, olt_pump_fn pump, void *pump_ud,
               uint16_t cls, uint16_t inst,
               const uint8_t *attrs, uint16_t len);
int olt_delete(struct olt_sim *olt, olt_pump_fn pump, void *pump_ud,
               uint16_t cls, uint16_t inst);
int olt_getnext(struct olt_sim *olt, olt_pump_fn pump, void *pump_ud,
                uint16_t cls, uint16_t inst, uint16_t mask,
                uint8_t *attrs, size_t cap, size_t *attrs_len,
                uint16_t *next_inst);
int olt_mib_upload(struct olt_sim *olt, olt_pump_fn pump, void *pump_ud,
                   unsigned *num_images);
int olt_mib_upload_next(struct olt_sim *olt, olt_pump_fn pump, void *ud,
                        unsigned seq, uint16_t *cls, uint16_t *inst,
                        uint16_t *mask, uint8_t *attrs, size_t cap,
                        size_t *attrs_len);
int olt_mib_reset(struct olt_sim *olt, olt_pump_fn pump, void *pump_ud);

/* Full MIB upload session: upload + all upload-nexts. Returns the
 * number of images collected; images are also counted per class in
 * per_class_stats if non-NULL (indexed by class id, up to cls_max). */
unsigned olt_full_upload_session(struct olt_sim *olt, olt_pump_fn pump,
                                 void *pump_ud, FILE *log,
                                 unsigned *per_class_stats,
                                 size_t cls_max);

#ifdef __cplusplus
}
#endif
#endif /* OMCI2_OLT_SIM_H */
