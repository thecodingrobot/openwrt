/*
 * omci/me_defs.h — managed-entity schema (G.988 attribute tables)
 *
 * omcid2 — clean-room OMCI daemon for Lantiq Falcon GPON sticks
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef OMCI2_ME_DEFS_H
#define OMCI2_ME_DEFS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum omci_attr_type {
    A_U8,       /* 1-byte unsigned */
    A_U16,      /* 2-byte unsigned, big-endian on the wire */
    A_U32,      /* 4-byte unsigned, big-endian on the wire */
    A_STR,      /* fixed-length ASCII string, NUL padded */
    A_BYTES,    /* fixed-length octet string */
    A_TABLE,    /* variable table attribute (fixed slot in our store) */
};

#define AF_WRITE 0x01    /* settable via Set/Create */
#define AF_SBC   0x02    /* set-by-create */
#define AF_TABLE 0x04    /* table attribute */
#define AF_NOUP  0x08    /* excluded from MIB upload */

struct omci_attr_def {
    const char *name;
    uint8_t  type;
    uint16_t size;       /* storage size in bytes (table: slot size) */
    uint16_t mask;       /* attribute mask bit */
    uint8_t  flags;
};

struct omci_class_def {
    uint16_t class_id;
    const char *name;
    const struct omci_attr_def *attrs;  /* attribute 1..n (index 0 = ME
                                          * instance id, implied) */
    uint8_t  num_attrs;
    uint8_t  alarm_bitmap_len;           /* 0 = no alarms */
};

extern const struct omci_class_def *const omci_classes[];
extern const unsigned omci_class_count;

/* Look up a class definition; NULL when unsupported. */
const struct omci_class_def *omci_class_find(uint16_t class_id);

/* Attribute helpers (attr_no is 1-based; 0 is the ME id). */
const struct omci_attr_def *omci_attr(const struct omci_class_def *cd,
                                      unsigned attr_no);
/* Byte offset of attribute attr_no inside the packed attribute area. */
int omci_attr_offset(const struct omci_class_def *cd, unsigned attr_no);
int omci_attr_size(const struct omci_class_def *cd, unsigned attr_no);
/* Attribute number for a mask bit position, 0 when not defined. */
unsigned omci_attr_no_for_mask(const struct omci_class_def *cd,
                               uint16_t mask_bit);
/* Mask of attributes included in a MIB upload (excludes ME id and
 * AF_NOUP attributes). */
uint16_t omci_class_upload_mask(const struct omci_class_def *cd);
/* Total packed attribute-area size. */
int omci_class_attr_area_len(const struct omci_class_def *cd);

/* Packed byte length of the attributes selected by mask (wire layout
 * of a Get response / Set request payload). -1 on bad input. */
int omci_mask_packed_len(const struct omci_class_def *cd, uint16_t mask);

/* Permission-string for meg dump (11 chars, see specs/interface-b.md). */
void omci_attr_flags_str(const struct omci_attr_def *ad, char *dst /*12*/);
/* Type name for meg dump ("UINT", "ENUM", "STR", ...). */
const char *omci_attr_type_str(const struct omci_attr_def *ad);

#ifdef __cplusplus
}
#endif
#endif /* OMCI2_ME_DEFS_H */
