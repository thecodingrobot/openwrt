/*
 * omci/mib.h — managed-entity database
 *
 * omcid2 — clean-room OMCI daemon for Lantiq Falcon GPON sticks
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef OMCI2_MIB_H
#define OMCI2_MIB_H

#include <stddef.h>
#include <stdint.h>

#include "omci.h"
#include "me_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Largest attribute area among supported classes (ME 256: 71 bytes
 * including the optional logical id/password attributes).
 * Unsized table attributes consume no storage. */
#define MIB_ATTRS_MAX 80

struct mib_inst {
    uint16_t cls;
    uint16_t inst;
    uint16_t attrs_len;
    uint8_t  attrs[MIB_ATTRS_MAX];
    struct mib_inst *next;
};

struct mib {
    struct mib_inst *head;      /* ordered by (class, instance) */
    unsigned count;
};

/* All APIs return OMCI result codes (enum omci_result) unless stated. */

int  mib_init(struct mib *m);
void mib_free(struct mib *m);

/* Raw instance access */
struct mib_inst *mib_find(struct mib *m, uint16_t cls, uint16_t inst);

/* Create an instance. attr_bytes (may be NULL) is a packed attribute
 * area (attribute 1..n in schema order); len bytes copied (padded). */
int mib_create(struct mib *m, uint16_t cls, uint16_t inst,
               const uint8_t *attr_bytes, uint16_t len);

int mib_delete(struct mib *m, uint16_t cls, uint16_t inst);

/* Set attributes selected by mask; val holds the packed values of the
 * selected attributes in mask-bit order (OMCI wire layout). */
int mib_set(struct mib *m, uint16_t cls, uint16_t inst, uint16_t mask,
            const uint8_t *val, uint16_t len);

/* Read attributes selected by mask into out (packed, mask-bit order).
 * *out_len receives the packed length. */
int mib_get(struct mib *m, uint16_t cls, uint16_t inst, uint16_t mask,
            uint8_t *out, size_t cap, size_t *out_len);

/* GetNext walk: returns attributes of the first instance of class cls
 * with instance id strictly greater than inst (wrapping to the first
 * instance; G.988 restricts this to table attributes but the walk is
 * generic). *next_inst receives the reported instance. */
int mib_get_next(struct mib *m, uint16_t cls, uint16_t inst, uint16_t mask,
                 uint8_t *out, size_t cap, size_t *out_len,
                 uint16_t *next_inst);

/* MIB reset: drop all instances (caller re-seeds). Also resets
 * MIB data sync. */
void mib_reset(struct mib *m);

/* MIB data sync counter (ME 2, attribute 1). Maintained by the engine:
 * incremented on every accepted MIB-changing operation. */
uint8_t mib_data_sync_get(const struct mib *m);
void mib_data_sync_set(struct mib *m, uint8_t v);

/* ---- MIB upload --------------------------------------------------------- */

/* Number of MIB upload next images (instances split to fit the baseline
 * 26-byte contents limit). */
unsigned mib_upload_num(struct mib *m);

/* Fetch image number seq (0-based): the reported ME, the attribute mask
 * chunk and the packed attribute values. */
int mib_upload_next(struct mib *m, unsigned seq,
                    uint16_t *cls, uint16_t *inst, uint16_t *mask,
                    uint8_t *out, size_t cap, size_t *out_len);

/* ---- dumps (omci_pipe.sh compatible, see specs/interface-b.md) ---------- */

/* meg <class> <instance>: full instance block; returns OMCI result. */
int mib_dump_me(struct mib *m, uint16_t cls, uint16_t inst,
                int (*emit)(const char *line, void *ud), void *ud);

/* md: dump every instance of a class; md (class 0): all instances. */
int mib_dump_all(struct mib *m, uint16_t cls,
                 int (*emit)(const char *line, void *ud), void *ud);

#ifdef __cplusplus
}
#endif
#endif /* OMCI2_MIB_H */
