/*
 * sim/drv_mock.c — in-process loopback driver implementation
 *
 * omcid2 — clean-room OMCI daemon for Lantiq Falcon GPON sticks
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "drv_mock.h"

#include <stdlib.h>
#include <string.h>

#include "omci/omci.h"

struct mock_event {
    struct drv_event ev;
    uint8_t pdu[OMCI_FRAME_BASELINE_LEN];
};

struct drv_mock_ctx {
    struct mock_event queue[DRV_MOCK_QUEUE_DEPTH];
    unsigned head, tail;
    unsigned ploam_state;
    void (*on_tx)(const uint8_t *pdu, uint16_t len, void *ud);
    void *tx_ud;
    unsigned tx_count, rx_count;
    unsigned overflow;
};

static int mock_init(void *ctx_, const struct drv_config *cfg)
{
    struct drv_mock_ctx *ctx = ctx_;
    (void)cfg;
    ctx->ploam_state = DRV_PLOAM_O1;
    return 0;
}

static int mock_omci_send(void *ctx_, const uint8_t *pdu, uint16_t len)
{
    struct drv_mock_ctx *ctx = ctx_;

    ctx->tx_count++;
    if (ctx->on_tx)
        ctx->on_tx(pdu, len, ctx->tx_ud);
    return 0;
}

static int mock_event_get(void *ctx_, struct drv_event *ev)
{
    struct drv_mock_ctx *ctx = ctx_;
    struct mock_event *e;

    if (ctx->head == ctx->tail)
        return 1;
    e = &ctx->queue[ctx->tail];
    ctx->tail = (ctx->tail + 1) % DRV_MOCK_QUEUE_DEPTH;
    *ev = e->ev;
    if (ev->id == DRV_EVENT_OMCI_RECEIVE)
        ev->u.omci.pdu = e->pdu;    /* valid until next event_get */
    return 0;
}

static int mock_ploam_state_get(void *ctx_, unsigned *state)
{
    struct drv_mock_ctx *ctx = ctx_;
    *state = ctx->ploam_state;
    return 0;
}

static void mock_close(void *ctx_)
{
    free(ctx_);
}

static void enqueue(struct drv_mock_ctx *ctx, const struct drv_event *ev,
                    const uint8_t *pdu, uint16_t len)
{
    struct mock_event *e = &ctx->queue[ctx->head];

    if ((ctx->head + 1) % DRV_MOCK_QUEUE_DEPTH == ctx->tail) {
        ctx->overflow++;
        return;
    }
    e->ev = *ev;
    if (ev->id == DRV_EVENT_OMCI_RECEIVE && pdu && len) {
        uint16_t n = len > OMCI_FRAME_BASELINE_LEN ? OMCI_FRAME_BASELINE_LEN
                                                   : len;
        memcpy(e->pdu, pdu, n);
        e->ev.u.omci.pdu = e->pdu;
        e->ev.u.omci.len = n;
    }
    ctx->head = (ctx->head + 1) % DRV_MOCK_QUEUE_DEPTH;
    if (ev->id == DRV_EVENT_OMCI_RECEIVE)
        ctx->rx_count++;
    if (ev->id == DRV_EVENT_STATE_CHANGE)
        ctx->ploam_state = ev->u.state.curr_state;
}

int drv_mock_open(const struct drv_ops **ops_out, void **ctx_out)
{
    static const struct drv_ops mock_ops = {
        .name            = "mock",
        .init            = mock_init,
        .omci_send       = mock_omci_send,
        .event_get       = mock_event_get,
        .ploam_state_get = mock_ploam_state_get,
        .close           = mock_close,
    };
    struct drv_mock_ctx *ctx = calloc(1, sizeof(*ctx));

    if (!ctx)
        return -1;
    ctx->ploam_state = 0;
    *ops_out = &mock_ops;
    *ctx_out = ctx;
    return 0;
}

void drv_mock_set_tx_hook(void *ctx_,
                          void (*fn)(const uint8_t *pdu, uint16_t len,
                                     void *ud), void *ud)
{
    struct drv_mock_ctx *ctx = ctx_;
    ctx->on_tx = fn;
    ctx->tx_ud = ud;
}

void drv_mock_inject_omci(void *ctx_, const uint8_t *pdu, uint16_t len)
{
    struct drv_mock_ctx *ctx = ctx_;
    struct drv_event ev;

    memset(&ev, 0, sizeof(ev));
    ev.id = DRV_EVENT_OMCI_RECEIVE;
    ev.u.omci.pdu = pdu;
    ev.u.omci.len = len;
    enqueue(ctx, &ev, pdu, len);
}

void drv_mock_inject_state(void *ctx_, unsigned prev, unsigned curr)
{
    struct drv_mock_ctx *ctx = ctx_;
    struct drv_event ev;

    memset(&ev, 0, sizeof(ev));
    ev.id = DRV_EVENT_STATE_CHANGE;
    ev.u.state.curr_state = curr;
    ev.u.state.prev_state = prev;
    enqueue(ctx, &ev, NULL, 0);
}

unsigned drv_mock_tx_count(void *ctx_)
{
    return ((struct drv_mock_ctx *)ctx_)->tx_count;
}

unsigned drv_mock_rx_count(void *ctx_)
{
    return ((struct drv_mock_ctx *)ctx_)->rx_count;
}
