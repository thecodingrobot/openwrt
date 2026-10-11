/*
 * core/omcc.h — OMCC engine: OMCI request dispatch onto the MIB
 *
 * omcid2 — clean-room OMCI daemon for Lantiq Falcon GPON sticks
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef OMCI2_OMCC_H
#define OMCI2_OMCC_H

#include <stddef.h>
#include <stdint.h>

#include "omci/mib.h"
#include "platform/drv_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

struct omcc {
    struct mib *mib;
    const struct drv_ops *drv;
    void *drv_ctx;

    /* stats */
    unsigned rx_count, tx_count;
    unsigned result_counts[10];
};

int  omcc_init(struct omcc *o, struct mib *mib,
               const struct drv_ops *drv, void *drv_ctx);

/* Process one received OMCI PDU (OLP->ONU). Sends the response through
 * the driver when the AR flag is set. Returns the OMCI result code that
 * was produced (or OMCI_RES_PROC_ERROR for framing errors). */
int omcc_rx_pdu(struct omcc *o, const uint8_t *pdu, size_t len);

/* Send an autonomous notification (Alarm / AVC). mask/attrs describe
 * the reported attribute(s). */
int omcc_send_avc(struct omcc *o, uint16_t cls, uint16_t inst,
                  uint16_t mask, const uint8_t *val, uint16_t len);

/* Baseline contents budget for Get responses (32 - result - mask). */
#define OMCC_GET_BUDGET (OMCI_CONTENT_MAX - 3)
/* GetNext adds the 2-byte next-instance to the attribute data. */
#define OMCC_GETNEXT_BUDGET (OMCI_CONTENT_MAX - 3 - 2)

#ifdef __cplusplus
}
#endif
#endif /* OMCI2_OMCC_H */
