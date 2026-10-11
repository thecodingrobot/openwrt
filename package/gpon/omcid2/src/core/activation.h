/*
 * core/activation.h — ONU activation state tracking (O1..O5)
 *
 * omcid2 — clean-room OMCI daemon for Lantiq Falcon GPON sticks
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef OMCI2_ACTIVATION_H
#define OMCI2_ACTIVATION_H

#include "platform/drv_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

struct activation {
    unsigned state;         /* current PLOAM FSM state, 0 = unknown */
    unsigned prev;
};

static inline void activation_init(struct activation *a)
{
    a->state = 0;
    a->prev = 0;
}

/* Feed one driver event; updates the tracked state. */
static inline void activation_event(struct activation *a,
                                    const struct drv_event *ev)
{
    if (ev->id == DRV_EVENT_STATE_CHANGE) {
        a->prev = ev->u.state.prev_state;
        a->state = ev->u.state.curr_state;
    }
}

static inline int activation_is_o5(const struct activation *a)
{
    return a->state == DRV_PLOAM_O5;
}

const char *activation_state_name(unsigned state);

#ifdef __cplusplus
}
#endif
#endif /* OMCI2_ACTIVATION_H */
