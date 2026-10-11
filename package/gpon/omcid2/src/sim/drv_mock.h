/*
 * sim/drv_mock.h — in-process loopback driver (interface A test double)
 *
 * omcid2 — clean-room OMCI daemon for Lantiq Falcon GPON sticks
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef OMCI2_DRV_MOCK_H
#define OMCI2_DRV_MOCK_H

#include <stdint.h>

#include "platform/drv_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRV_MOCK_QUEUE_DEPTH 64

int drv_mock_open(const struct drv_ops **ops, void **ctx);

/* TX hook: invoked for every omci_send (OLT side of the wire). */
void drv_mock_set_tx_hook(void *ctx,
                          void (*fn)(const uint8_t *pdu, uint16_t len,
                                     void *ud), void *ud);

/* OLT-side injections (become driver events for the daemon). */
void drv_mock_inject_omci(void *ctx, const uint8_t *pdu, uint16_t len);
void drv_mock_inject_state(void *ctx, unsigned prev, unsigned curr);

/* Counters for assertions. */
unsigned drv_mock_tx_count(void *ctx);
unsigned drv_mock_rx_count(void *ctx);

#ifdef __cplusplus
}
#endif
#endif /* OMCI2_DRV_MOCK_H */
