/*
 * platform/drv_ioctl.c — real interface-A client over /dev/onu
 *
 * omcid2 — clean-room OMCI daemon for Lantiq Falcon GPON sticks
 *
 * Includes the GPL driver headers directly (they are dual GPL/LGPL,
 * see ~/gpon-port/gpon_onu_drv/src/include/) so ioctl numbers and
 * struct layouts can never drift from interface A. Requires compile
 * flags: -DLINUX -I<gpon_onu_drv>/src/include -I<lib_ifxos>/src/include
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "drv_adapter.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#include <drv_onu_common_interface.h>
#include <drv_onu_event_interface.h>
#include <drv_onu_gpe_interface.h>
#include <drv_onu_gtc_interface.h>
#include <drv_onu_interface.h>
#include <drv_onu_ploam_interface.h>

struct drv_ioctl_ctx {
    int fd;
    /* one-deep event staging (event FIFO payloads must be copied out
     * of the ioctl buffer before returning to the caller) */
    struct {
        struct drv_event ev;
        uint8_t pdu[ONU_GPE_MAX_OMCI_FRAME_LENGTH];
    } pending;
    int has_pending;
};

/* fio_exchange call convention: tools/onu/onu_control_rpc.c */
static int iocmd(int fd, unsigned long cmd, void *data, unsigned int size)
{
    struct fio_exchange ex;
    int err;

    ex.p_data = data;
    ex.length = size;
    ex.error = 0;

    err = ioctl(fd, cmd, (long)&ex);
    if (err != 0)
        return -1;
    if (ex.error == 0)
        return 0;
    if (cmd == (unsigned long)FIO_ONU_EVENT_FIFO && ex.error == 1)
        return 1;   /* FIFO overflow: event dropped */
    return -1;
}

static int drv_ioctl_init(void *ctx_, const struct drv_config *cfg)
{
    struct drv_ioctl_ctx *ctx = ctx_;
    struct onu_event_mask mask = { .val = 0xFFFFFFFFu };
    struct gpe_cfg gpe_cfg;
    int rc;

    (void)cfg;

    /* [HW] full sequence: PLOAM init, GTC init/cfg, serial number,
     * GPE init (PE firmware), GPE cfg, LAN init + GPHY firmware.
     * The exact stock argument set comes from the golden ioctl trace
     * (specs/interface-a.md section 2); the OMCI-relevant minimum is
     * configured here. */
    rc = iocmd(ctx->fd, FIO_PLOAM_INIT, NULL, 0);
    if (rc != 0) {
        fprintf(stderr, "drv_ioctl: FIO_PLOAM_INIT failed\n");
        return -1;
    }

    memset(&gpe_cfg, 0, sizeof(gpe_cfg));
    gpe_cfg.gem_blk_len = 48;
    gpe_cfg.pdu_size_max_omci = 48;     /* baseline OMCI frame length */
    rc = iocmd(ctx->fd, FIO_GPE_CFG_SET, &gpe_cfg, sizeof(gpe_cfg));
    if (rc != 0) {
        fprintf(stderr, "drv_ioctl: FIO_GPE_CFG_SET failed\n");
        return -1;
    }

    rc = iocmd(ctx->fd, FIO_ONU_EVENT_ENABLE_SET, &mask, sizeof(mask));
    if (rc != 0) {
        fprintf(stderr, "drv_ioctl: event FIFO enable failed\n");
        return -1;
    }
    return 0;
}

static int drv_ioctl_omci_send(void *ctx_, const uint8_t *pdu, uint16_t len)
{
    struct drv_ioctl_ctx *ctx = ctx_;
    struct gpe_omci_msg msg;

    if (len > ONU_GPE_MAX_OMCI_FRAME_LENGTH)
        return -1;
    memset(&msg, 0, sizeof(msg));
    msg.length = len;
    memcpy(msg.message, pdu, len);
    return iocmd(ctx->fd, FIO_GPE_OMCI_SEND, &msg, sizeof(msg));
}

static int drv_ioctl_event_get(void *ctx_, struct drv_event *ev)
{
    struct drv_ioctl_ctx *ctx = ctx_;
    struct onu_fifo_data fifo;
    fd_set rfds;
    struct timeval tv = { .tv_sec = 0, .tv_usec = 0 };
    int rc;

    if (ctx->has_pending) {
        *ev = ctx->pending.ev;
        if (ev->id == DRV_EVENT_OMCI_RECEIVE)
            ev->u.omci.pdu = ctx->pending.pdu;
        ctx->has_pending = 0;
        return 0;
    }

    /* poll() equivalent of the GPL daemon loop: select first so the
     * FIFO ioctl is only issued when data is available */
    FD_ZERO(&rfds);
    FD_SET(ctx->fd, &rfds);
    rc = select(ctx->fd + 1, &rfds, NULL, NULL, &tv);
    if (rc <= 0)
        return 1;

    memset(&fifo, 0, sizeof(fifo));
    rc = iocmd(ctx->fd, FIO_ONU_EVENT_FIFO, &fifo, sizeof(fifo));
    if (rc < 0)
        return -1;
    if (rc == 1)
        return 1;   /* overflow marker: treated as no-event */

    switch (fifo.header.id) {
    case ONU_EVENT_OMCI_RECEIVE:
        ctx->pending.ev.id = DRV_EVENT_OMCI_RECEIVE;
        ctx->pending.ev.u.omci.len =
            (uint16_t)fifo.data.omci_message.length;
        if (ctx->pending.ev.u.omci.len > ONU_GPE_MAX_OMCI_FRAME_LENGTH)
            ctx->pending.ev.u.omci.len = ONU_GPE_MAX_OMCI_FRAME_LENGTH;
        memcpy(ctx->pending.pdu, fifo.data.omci_message.message,
               ctx->pending.ev.u.omci.len);
        break;
    case ONU_EVENT_STATE_CHANGE:
        ctx->pending.ev.id = DRV_EVENT_STATE_CHANGE;
        ctx->pending.ev.u.state.curr_state =
            (unsigned)fifo.data.state.curr_state;
        ctx->pending.ev.u.state.prev_state =
            (unsigned)fifo.data.state.previous_state;
        break;
    case ONU_EVENT_OMCI_SENT:
        ctx->pending.ev.id = DRV_EVENT_OMCI_SENT;
        break;
    default:
        return 1;   /* event not relevant to omcid2 (yet) */
    }

    ctx->has_pending = 1;
    return drv_ioctl_event_get(ctx_, ev);
}

static int drv_ioctl_ploam_state_get(void *ctx_, unsigned *state)
{
    struct drv_ioctl_ctx *ctx = ctx_;
    struct ploam_state_data_get st;

    memset(&st, 0, sizeof(st));
    if (iocmd(ctx->fd, FIO_PLOAM_STATE_GET, &st, sizeof(st)) != 0)
        return -1;
    *state = (unsigned)st.curr_state;
    return 0;
}

static void drv_ioctl_close(void *ctx_)
{
    struct drv_ioctl_ctx *ctx = ctx_;
    if (ctx->fd >= 0)
        close(ctx->fd);
    free(ctx);
}

int drv_ioctl_open(const char *device, const struct drv_ops **ops_out,
                   void **ctx_out)
{
    static const struct drv_ops ops = {
        .name            = "ioctl",
        .init            = drv_ioctl_init,
        .omci_send       = drv_ioctl_omci_send,
        .event_get       = drv_ioctl_event_get,
        .ploam_state_get = drv_ioctl_ploam_state_get,
        .close           = drv_ioctl_close,
    };
    struct drv_ioctl_ctx *ctx = calloc(1, sizeof(*ctx));

    if (!ctx)
        return -1;
    ctx->fd = open(device ? device : DRV_IO_GPON_ONU, O_RDWR, 0644);
    if (ctx->fd < 0) {
        free(ctx);
        return -1;
    }
    *ops_out = &ops;
    *ctx_out = ctx;
    return 0;
}
