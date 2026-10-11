/*
 * core/activation.c — ONU activation state names
 *
 * omcid2 — clean-room OMCI daemon for Lantiq Falcon GPON sticks
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "activation.h"

const char *activation_state_name(unsigned state)
{
    switch (state) {
    case 0:  return "unknown";
    case DRV_PLOAM_O1: return "O1 (initial)";
    case DRV_PLOAM_O2: return "O2 (standby)";
    case DRV_PLOAM_O3: return "O3 (serial number)";
    case DRV_PLOAM_O4: return "O4 (ranging)";
    case DRV_PLOAM_O5: return "O5 (operation)";
    case 6:  return "O6 (power saving)";
    case 7:  return "O7 (emergency stop)";
    default: return "reserved";
    }
}
