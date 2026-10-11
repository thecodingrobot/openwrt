/*
 * omci/omci.h — OMCI (ITU-T G.988) baseline message codec
 *
 * omcid2 — clean-room OMCI daemon for Lantiq Falcon GPON sticks
 *
 * Wire format verified against the public OMCI example capture shipped
 * with the community Wireshark dissector (hack-gpon.org/omci-wireshark)
 * and cross-checked with the Apache-2.0 OpenOMCI omci-lib-go definitions.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef OMCI2_OMCI_H
#define OMCI2_OMCI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Frame geometry (baseline message set) ---------------------------- */

/* Baseline OMCI message = 8-byte header + 32-byte message contents +
 * 4-byte trailer (length 0x0028 + padding) + 4-byte CRC-32 = 48 octets. */
#define OMCI_FRAME_BASELINE_LEN 48
#define OMCI_HDR_LEN             8
#define OMCI_CONTENT_MAX        32
#define OMCI_TRAILER_LEN         4
#define OMCI_CRC_LEN             4

/* Device identifier */
#define OMCI_DEV_BASELINE   0x0A
#define OMCI_DEV_EXTENDED   0x0B

/* ---- Message type byte ------------------------------------------------ */

#define OMCI_F_DB       0x80    /* destination bit (broadcast) */
#define OMCI_F_AR       0x40    /* acknowledge request */
#define OMCI_F_AK       0x20    /* acknowledgement (this is a response) */
#define OMCI_MT_MASK    0x1F    /* message type occupies low 5 bits */

enum omci_msg_type {
    OMCI_MT_CREATE               = 0x04,
    /* 0x05 "set by reference" existed in early G.984.4; G.988 reserves it.
     * Decoded for legacy interop, rejected at the MIB layer. */
    OMCI_MT_SET_BY_REF           = 0x05,
    OMCI_MT_DELETE               = 0x06,
    OMCI_MT_SET                  = 0x08,
    OMCI_MT_GET                  = 0x09,
    OMCI_MT_GET_ALL_ALARMS       = 0x0B,
    OMCI_MT_GET_ALL_ALARMS_NEXT  = 0x0C,
    OMCI_MT_MIB_UPLOAD           = 0x0D,
    OMCI_MT_MIB_UPLOAD_NEXT      = 0x0E,
    OMCI_MT_MIB_RESET            = 0x0F,
    OMCI_MT_ALARM                = 0x10,    /* autonomous ONU->OLT */
    OMCI_MT_AVC                  = 0x11,    /* autonomous ONU->OLT */
    OMCI_MT_TEST                 = 0x12,
    OMCI_MT_START_SW_DOWNLOAD    = 0x13,
    OMCI_MT_DOWNLOAD_SECTION     = 0x14,
    OMCI_MT_END_SW_DOWNLOAD      = 0x15,
    OMCI_MT_ACTIVATE_SW          = 0x16,
    OMCI_MT_COMMIT_SW            = 0x17,
    OMCI_MT_SYNC_TIME            = 0x18,
    OMCI_MT_REBOOT               = 0x19,
    OMCI_MT_GET_NEXT             = 0x1A,
    OMCI_MT_TEST_RESULT          = 0x1B,    /* autonomous ONU->OLT */
    OMCI_MT_GET_CURRENT_DATA     = 0x1C,
    OMCI_MT_SET_TABLE            = 0x1D,    /* extended message set only */
};

/* Message result codes (G.988 Table A.4.x / response "result" field) */
enum omci_result {
    OMCI_RES_SUCCESS           = 0,
    OMCI_RES_PROC_ERROR        = 1,
    OMCI_RES_NOT_SUPPORTED     = 2,
    OMCI_RES_PARAM_ERROR       = 3,
    OMCI_RES_UNKNOWN_ME        = 4,
    OMCI_RES_UNKNOWN_INSTANCE  = 5,
    OMCI_RES_DEVICE_BUSY       = 6,
    OMCI_RES_INSTANCE_EXISTS   = 7,
    /* 8: reserved */
    OMCI_RES_ATTR_FAILED       = 9,
};

/* Decode error codes returned by omci_decode() */
enum omci_decode_err {
    OMCI_OK              =  0,
    OMCI_E_TOOSHORT      = -1,   /* buffer smaller than header */
    OMCI_E_BADLEN        = -2,   /* length inconsistent with layout */
    OMCI_E_BADDEV        = -3,   /* unknown device identifier */
    OMCI_E_BADMT         = -4,   /* reserved/unknown message type */
    OMCI_E_BUFOVER       = -5,   /* content overflows struct */
};

/* Alarm notification geometry: bitmap + 3 pad + sequence number */
#define OMCI_ALARM_BITMAP_LEN 28
#define OMCI_ALARM_SEQ_OFF    (OMCI_HDR_LEN + OMCI_ALARM_BITMAP_LEN + 3)

/* ---- Message model ----------------------------------------------------- */

struct omci_msg {
    /* common header */
    uint16_t tc;            /* transaction correlation id */
    uint8_t  msg_type;      /* full byte: DB|AR|AK|MT */
    uint8_t  dev_id;
    uint16_t me_class;
    uint16_t me_instance;

    /* structured fields; which are valid depends on the message kind
     * (see omci_kind() and the layout table in codec.c) */
    uint8_t  result;        /* responses */
    uint16_t attr_mask;     /* set/get/get-next/avc/upload-next reported */
    uint16_t seq;           /* get-next req: command count
                             * mib-upload-next req / get-all-alarms-next req:
                             *   command sequence number */
    uint16_t num_cmds;      /* mib-upload rsp / get-all-alarms rsp */
    uint16_t reported_class;    /* mib-upload-next rsp: reported ME */
    uint16_t reported_instance;

    /* raw message-contents bytes that follow the structured prefix
     * (attribute values, create payloads, alarm bitmaps, ...) */
    uint8_t  content[OMCI_CONTENT_MAX];
    uint16_t content_len;
};

/* Message classification (see codec.c for the layout table) */
enum omci_kind {
    OMCI_K_REQ,             /* OLT -> ONU, expects a response */
    OMCI_K_RSP,             /* ONU -> OLT, AK set */
    OMCI_K_AUTONOMOUS,      /* ONU -> OLT notification (alarm/avc/test result) */
    OMCI_K_OTHER,           /* decoded generically (sw download etc.) */
};

/* ---- API --------------------------------------------------------------- */

/* IEEE 802.3 CRC-32 (reflected, poly 0xEDB88320, init/final 0xFFFFFFFF),
 * as carried in the last 4 octets of a baseline frame (big-endian). */
uint32_t omci_crc32(const void *buf, size_t len);

static inline unsigned omci_mt(const struct omci_msg *m)
    { return m->msg_type & OMCI_MT_MASK; }
static inline int omci_ar(const struct omci_msg *m)
    { return !!(m->msg_type & OMCI_F_AR); }
static inline int omci_ak(const struct omci_msg *m)
    { return !!(m->msg_type & OMCI_F_AK); }
static inline int omci_db(const struct omci_msg *m)
    { return !!(m->msg_type & OMCI_F_DB); }

enum omci_kind omci_kind(const struct omci_msg *m);

/* True if the message type carries an attribute mask right after the
 * structured prefix (set/set-by-ref/get/get-next/avc). */
int omci_has_attr_mask(const struct omci_msg *m);

/* Decode one baseline frame.
 * len may be OMCI_FRAME_BASELINE_LEN (trailer + CRC verified when the
 * CRC field is non-zero; legacy captures may zero it) or a shorter
 * header+contents buffer (no trailer/CRC check). */
int omci_decode(const uint8_t *buf, size_t len, struct omci_msg *m);

/* Encode one baseline frame (always 48 bytes: contents zero-padded,
 * trailer 0x0028 0x0000, CRC appended).
 * Returns frame length (48) or -1 if cap is too small. */
int omci_encode(const struct omci_msg *m, uint8_t *buf, size_t cap);

/* 1 if the last omci_decode() of a 48-byte frame had a valid CRC. */
int omci_last_crc_ok(void);

/* Hex helper: parse "AABBCC.." (any whitespace/separators ignored) into
 * buf, returns byte count or -1. Useful for tests and the pipe CLI. */
int omci_hex_parse(const char *hex, uint8_t *buf, size_t cap);
/* Format buf as lowercase hex pairs into dst (dst needs 2*len+1). */
void omci_hex_format(const uint8_t *buf, size_t len, char *dst);

/* Result-code / message-type names (static const strings). */
const char *omci_mt_name(unsigned mt);
const char *omci_result_name(unsigned result);

#ifdef __cplusplus
}
#endif
#endif /* OMCI2_OMCI_H */
