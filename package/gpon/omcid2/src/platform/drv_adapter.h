/*
 * platform/drv_adapter.h — mockable interface-A driver client
 *
 * omcid2 — clean-room OMCI daemon for Lantiq Falcon GPON sticks
 *
 * The ONU driver boundary (see specs/interface-a.md) is abstracted
 * behind a vtable so everything above it (activation, OMCC, MIB) is
 * host-testable against the sim/ loopback driver. The real backend
 * (drv_ioctl.c) talks to /dev/onu using the GPL driver headers.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef OMCI2_DRV_ADAPTER_H
#define OMCI2_DRV_ADAPTER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Driver event ids (drv_onu_event_interface.h values) */
#define DRV_EVENT_HARDWARE          0
#define DRV_EVENT_PLOAM_DS          1
#define DRV_EVENT_PLOAM_US          2
#define DRV_EVENT_STATE_CHANGE      3
#define DRV_EVENT_OMCI_RECEIVE      4
#define DRV_EVENT_OMCI_SENT         11
#define DRV_EVENT_LINK_STATE_CHANGE 13

/* PLOAM FSM states (enum ploam_state values) */
#define DRV_PLOAM_O1 1
#define DRV_PLOAM_O2 2
#define DRV_PLOAM_O3 3
#define DRV_PLOAM_O4 4
#define DRV_PLOAM_O5 5

struct drv_event {
    unsigned id;
    union {
        struct {
            unsigned curr_state;
            unsigned prev_state;
        } state;
        struct {
            /* pdu is owned by the driver backend and valid until the
             * next event_get() call. */
            const uint8_t *pdu;
            uint16_t len;
        } omci;
    } u;
};

struct drv_config {
    /* interface B inputs (see specs/interface-b.md section 5) */
    const char *mib_file;           /* seed profile path */
    const char *image0_version;     /* ME 7 instance 0 version (may be NULL) */
    const char *image1_version;     /* ME 7 instance 1 version (may be NULL) */
    unsigned omcc_version;          /* -o option, 0 = default */
    unsigned iop_mask;              /* -i option */
    int debug_level;                /* -d option, 0..4 */
};

struct drv_ops {
    const char *name;

    /* Bring the driver up through the init sequence
     * (specs/interface-a.md section 2): PLOAM init, GTC init+cfg,
     * GPE init+cfg (pdu_size_max_omci), event FIFO enable. */
    int  (*init)(void *ctx, const struct drv_config *cfg);

    /* OMCI TX (FIO_GPE_OMCI_SEND). pdu: full baseline frame
     * (trailer+CRC included), len = 48. */
    int  (*omci_send)(void *ctx, const uint8_t *pdu, uint16_t len);

    /* Poll one event. Returns 0 and fills *ev, 1 when no event is
     * pending, -1 on hard error. */
    int  (*event_get)(void *ctx, struct drv_event *ev);

    /* FIO_PLOAM_STATE_GET fallback polling. */
    int  (*ploam_state_get)(void *ctx, unsigned *state);

    void (*close)(void *ctx);
};

#ifdef __cplusplus
}
#endif
#endif /* OMCI2_DRV_ADAPTER_H */
