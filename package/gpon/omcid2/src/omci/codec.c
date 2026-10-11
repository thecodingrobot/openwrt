/*
 * omci/codec.c — OMCI baseline message encode/decode
 *
 * omcid2 — clean-room OMCI daemon for Lantiq Falcon GPON sticks
 *
 * Layout table (message contents after the 8-byte common header),
 * derived from the example capture shipped with the community OMCI
 * Wireshark dissector and the Apache-2.0 OpenOMCI omci-lib-go:
 *
 *   MT  request                     response
 *   --  -------------------------   ----------------------------------
 *   04  set-by-create attrs         result(1)
 *   05  mask(2) + attrs (legacy)    result(1)
 *   06  -                           result(1)
 *   08  mask(2) + attrs             result(1)
 *   09  mask(2)                     result(1) + mask(2) + attrs
 *   0B  alarm retrieval mode(1)     number of commands(2)
 *   0C  sequence number(2)          class(2) + inst(2) + bitmap(28)
 *   0D  -                           number of commands(2)
 *   0E  sequence number(2)          class(2) + inst(2) + mask(2) + attrs
 *   0F  -                           result(1)
 *   10  (autonomous) bitmap(28) + pad(3) + sequence number(1)
 *   11  (autonomous) mask(2) + attr value
 *   12  test id(1) + test params    result(1)
 *   13  window size(2) + ...        result(1)
 *   14  section no.(4) + crc(4) + section data
 *   15  crc(4) + image ptr(1)       result(1)
 *   16  image ptr(1)                result(1)
 *   17  image ptr(1)                result(1)
 *   18  time-of-day(8)              result(1)
 *   19  -                           result(1)
 *   1A  mask(2) + seq(2)            result(1) + mask(2) + attrs
 *   1B  (autonomous) test result data
 *   1C  mask(2)                     result(1) + mask(2) + attrs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "omci.h"

#include <string.h>

/* ---- CRC-32 ------------------------------------------------------------ */

uint32_t omci_crc32(const void *buf, size_t len)
{
    static uint32_t table[256];
    static int table_ready;
    const uint8_t *p = buf;
    uint32_t crc = 0xFFFFFFFFu;
    size_t i;

    if (!table_ready) {
        for (uint32_t n = 0; n < 256; n++) {
            uint32_t c = n;
            for (int k = 0; k < 8; k++)
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[n] = c;
        }
        table_ready = 1;
    }

    for (i = 0; i < len; i++)
        crc = table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);

    return crc ^ 0xFFFFFFFFu;
}

/* ---- Classification ----------------------------------------------------- */

enum omci_kind omci_kind(const struct omci_msg *m)
{
    unsigned mt = omci_mt(m);

    switch (mt) {
    case OMCI_MT_ALARM:
    case OMCI_MT_AVC:
    case OMCI_MT_TEST_RESULT:
        return OMCI_K_AUTONOMOUS;
    case OMCI_MT_CREATE:
    case OMCI_MT_SET_BY_REF:
    case OMCI_MT_DELETE:
    case OMCI_MT_SET:
    case OMCI_MT_GET:
    case OMCI_MT_GET_ALL_ALARMS:
    case OMCI_MT_GET_ALL_ALARMS_NEXT:
    case OMCI_MT_MIB_UPLOAD:
    case OMCI_MT_MIB_UPLOAD_NEXT:
    case OMCI_MT_MIB_RESET:
    case OMCI_MT_TEST:
    case OMCI_MT_START_SW_DOWNLOAD:
    case OMCI_MT_END_SW_DOWNLOAD:
    case OMCI_MT_ACTIVATE_SW:
    case OMCI_MT_COMMIT_SW:
    case OMCI_MT_SYNC_TIME:
    case OMCI_MT_REBOOT:
    case OMCI_MT_GET_NEXT:
    case OMCI_MT_GET_CURRENT_DATA:
        return omci_ak(m) ? OMCI_K_RSP : OMCI_K_REQ;
    default:
        /* download section has responses only when the OLT asked for one */
        if (omci_ak(m))
            return OMCI_K_RSP;
        return OMCI_K_OTHER;
    }
}

int omci_has_attr_mask(const struct omci_msg *m)
{
    switch (omci_mt(m)) {
    case OMCI_MT_SET:
    case OMCI_MT_SET_BY_REF:
    case OMCI_MT_GET:
    case OMCI_MT_GET_NEXT:
    case OMCI_MT_GET_CURRENT_DATA:
    case OMCI_MT_AVC:
        return 1;
    case OMCI_MT_MIB_UPLOAD_NEXT:
        return omci_kind(m) == OMCI_K_RSP;  /* reported ME mask */
    default:
        return 0;
    }
}

/* ---- Decode ------------------------------------------------------------- */

static int g_last_crc_ok;

int omci_last_crc_ok(void)
{
    return g_last_crc_ok;
}

static uint16_t be16(const uint8_t *p)
{
    return (uint16_t)((p[0] << 8) | p[1]);
}

/* Copy remaining contents bytes into msg->content, tracking offset. */
static int take_content(struct omci_msg *m, const uint8_t *c, size_t clen,
                        size_t off)
{
    if (off > clen)
        return OMCI_E_BADLEN;
    if (clen - off > OMCI_CONTENT_MAX)
        return OMCI_E_BUFOVER;
    m->content_len = (uint16_t)(clen - off);
    memcpy(m->content, c + off, m->content_len);
    return OMCI_OK;
}

int omci_decode(const uint8_t *buf, size_t len, struct omci_msg *m)
{
    const uint8_t *c;      /* contents area */
    size_t clen;           /* contents length (trailer/CRC stripped) */
    int rc;

    g_last_crc_ok = 0;

    if (len < OMCI_HDR_LEN)
        return OMCI_E_TOOSHORT;

    memset(m, 0, sizeof(*m));
    m->tc        = be16(buf);
    m->msg_type  = buf[2];
    m->dev_id    = buf[3];
    m->me_class  = be16(buf + 4);
    m->me_instance = be16(buf + 6);

    if (m->dev_id != OMCI_DEV_BASELINE)
        return OMCI_E_BADDEV;

    if (len == OMCI_FRAME_BASELINE_LEN) {
        uint32_t want = omci_crc32(buf, OMCI_FRAME_BASELINE_LEN - OMCI_CRC_LEN);
        uint32_t got = ((uint32_t)buf[44] << 24) | ((uint32_t)buf[45] << 16) |
                       ((uint32_t)buf[46] << 8) | (uint32_t)buf[47];
        g_last_crc_ok = (want == got);
        c = buf + OMCI_HDR_LEN;
        clen = OMCI_CONTENT_MAX;
    } else if (len > OMCI_FRAME_BASELINE_LEN) {
        return OMCI_E_BADLEN;
    } else {
        /* header + contents, no trailer/CRC (e.g. driver path) */
        c = buf + OMCI_HDR_LEN;
        clen = len - OMCI_HDR_LEN;
    }

    switch (omci_kind(m)) {
    case OMCI_K_REQ:
        switch (omci_mt(m)) {
        case OMCI_MT_CREATE:
            /* all contents bytes are set-by-create attributes */
            return take_content(m, c, clen, 0);
        case OMCI_MT_SET_BY_REF:
        case OMCI_MT_SET:
        case OMCI_MT_GET:
        case OMCI_MT_GET_CURRENT_DATA:
            if (clen < 2)
                return OMCI_E_BADLEN;
            m->attr_mask = be16(c);
            return take_content(m, c, clen, 2);
        case OMCI_MT_GET_NEXT:
            if (clen < 4)
                return OMCI_E_BADLEN;
            m->attr_mask = be16(c);
            m->seq = be16(c + 2);
            return take_content(m, c, clen, 4);
        case OMCI_MT_GET_ALL_ALARMS:
            if (clen < 1)
                return OMCI_E_BADLEN;
            m->result = c[0];   /* alarm retrieval mode */
            return take_content(m, c, clen, 1);
        case OMCI_MT_GET_ALL_ALARMS_NEXT:
        case OMCI_MT_MIB_UPLOAD_NEXT:
            if (clen < 2)
                return OMCI_E_BADLEN;
            m->seq = be16(c);
            return take_content(m, c, clen, 2);
        case OMCI_MT_MIB_UPLOAD:
        case OMCI_MT_MIB_RESET:
        case OMCI_MT_REBOOT:
            return take_content(m, c, clen, 0);
        case OMCI_MT_TEST:
        case OMCI_MT_START_SW_DOWNLOAD:
        case OMCI_MT_END_SW_DOWNLOAD:
        case OMCI_MT_ACTIVATE_SW:
        case OMCI_MT_COMMIT_SW:
        case OMCI_MT_SYNC_TIME:
        default:
            return take_content(m, c, clen, 0);
        }

    case OMCI_K_RSP:
        switch (omci_mt(m)) {
        case OMCI_MT_CREATE:
        case OMCI_MT_SET_BY_REF:
        case OMCI_MT_DELETE:
        case OMCI_MT_SET:
        case OMCI_MT_TEST:
        case OMCI_MT_MIB_RESET:
        case OMCI_MT_START_SW_DOWNLOAD:
        case OMCI_MT_DOWNLOAD_SECTION:
        case OMCI_MT_END_SW_DOWNLOAD:
        case OMCI_MT_ACTIVATE_SW:
        case OMCI_MT_COMMIT_SW:
        case OMCI_MT_SYNC_TIME:
        case OMCI_MT_REBOOT:
            if (clen < 1)
                return OMCI_E_BADLEN;
            m->result = c[0];
            return take_content(m, c, clen, 1);
        case OMCI_MT_GET:
        case OMCI_MT_GET_CURRENT_DATA:
            if (clen < 3)
                return OMCI_E_BADLEN;
            m->result = c[0];
            m->attr_mask = be16(c + 1);
            return take_content(m, c, clen, 3);
        case OMCI_MT_GET_NEXT:
            if (clen < 3)
                return OMCI_E_BADLEN;
            m->result = c[0];
            m->attr_mask = be16(c + 1);
            return take_content(m, c, clen, 3);
        case OMCI_MT_MIB_UPLOAD:
        case OMCI_MT_GET_ALL_ALARMS:
            if (clen < 2)
                return OMCI_E_BADLEN;
            m->num_cmds = be16(c);
            return take_content(m, c, clen, 2);
        case OMCI_MT_MIB_UPLOAD_NEXT:
            if (clen < 6)
                return OMCI_E_BADLEN;
            m->reported_class = be16(c);
            m->reported_instance = be16(c + 2);
            m->attr_mask = be16(c + 4);
            return take_content(m, c, clen, 6);
        case OMCI_MT_GET_ALL_ALARMS_NEXT:
            if (clen < 4)
                return OMCI_E_BADLEN;
            m->reported_class = be16(c);
            m->reported_instance = be16(c + 2);
            return take_content(m, c, clen, 4);
        default:
            return take_content(m, c, clen, 0);
        }

    case OMCI_K_AUTONOMOUS:
        switch (omci_mt(m)) {
        case OMCI_MT_ALARM:
            if (clen < OMCI_ALARM_BITMAP_LEN + 4)
                return OMCI_E_BADLEN;
            m->seq = c[OMCI_ALARM_BITMAP_LEN + 3];
            /* bitmap only; 3 pad bytes dropped */
            m->content_len = OMCI_ALARM_BITMAP_LEN;
            memcpy(m->content, c, OMCI_ALARM_BITMAP_LEN);
            return OMCI_OK;
        case OMCI_MT_AVC:
            if (clen < 2)
                return OMCI_E_BADLEN;
            m->attr_mask = be16(c);
            return take_content(m, c, clen, 2);
        default:    /* test result */
            return take_content(m, c, clen, 0);
        }

    default:        /* OMCI_K_OTHER: download section etc. */
        rc = take_content(m, c, clen, 0);
        return rc;
    }
}

/* ---- Encode ------------------------------------------------------------- */

static void put16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
}

int omci_encode(const struct omci_msg *m, uint8_t *buf, size_t cap)
{
    uint8_t *c;
    size_t off = 0;
    uint32_t crc;
    size_t content_len = m->content_len;

    if (cap < OMCI_FRAME_BASELINE_LEN)
        return -1;
    if (content_len > OMCI_CONTENT_MAX)
        return -1;

    memset(buf, 0, OMCI_FRAME_BASELINE_LEN);
    put16(buf, m->tc);
    buf[2] = m->msg_type;
    buf[3] = OMCI_DEV_BASELINE;
    put16(buf + 4, m->me_class);
    put16(buf + 6, m->me_instance);
    c = buf + OMCI_HDR_LEN;

    switch (omci_kind(m)) {
    case OMCI_K_REQ:
        switch (omci_mt(m)) {
        case OMCI_MT_SET_BY_REF:
        case OMCI_MT_SET:
        case OMCI_MT_GET:
        case OMCI_MT_GET_CURRENT_DATA:
            put16(c, m->attr_mask);
            off = 2;
            break;
        case OMCI_MT_GET_NEXT:
            put16(c, m->attr_mask);
            put16(c + 2, m->seq);
            off = 4;
            break;
        case OMCI_MT_GET_ALL_ALARMS:
            c[0] = m->result;  /* alarm retrieval mode */
            off = 1;
            break;
        case OMCI_MT_GET_ALL_ALARMS_NEXT:
        case OMCI_MT_MIB_UPLOAD_NEXT:
            put16(c, m->seq);
            off = 2;
            break;
        default:
            break;
        }
        break;

    case OMCI_K_RSP:
        switch (omci_mt(m)) {
        case OMCI_MT_CREATE:
        case OMCI_MT_SET_BY_REF:
        case OMCI_MT_DELETE:
        case OMCI_MT_SET:
        case OMCI_MT_TEST:
        case OMCI_MT_MIB_RESET:
        case OMCI_MT_START_SW_DOWNLOAD:
        case OMCI_MT_DOWNLOAD_SECTION:
        case OMCI_MT_END_SW_DOWNLOAD:
        case OMCI_MT_ACTIVATE_SW:
        case OMCI_MT_COMMIT_SW:
        case OMCI_MT_SYNC_TIME:
        case OMCI_MT_REBOOT:
            c[0] = m->result;
            off = 1;
            break;
        case OMCI_MT_GET:
        case OMCI_MT_GET_CURRENT_DATA:
        case OMCI_MT_GET_NEXT:
            c[0] = m->result;
            put16(c + 1, m->attr_mask);
            off = 3;
            break;
        case OMCI_MT_MIB_UPLOAD:
        case OMCI_MT_GET_ALL_ALARMS:
            put16(c, m->num_cmds);
            off = 2;
            break;
        case OMCI_MT_MIB_UPLOAD_NEXT:
            put16(c, m->reported_class);
            put16(c + 2, m->reported_instance);
            put16(c + 4, m->attr_mask);
            off = 6;
            break;
        case OMCI_MT_GET_ALL_ALARMS_NEXT:
            put16(c, m->reported_class);
            put16(c + 2, m->reported_instance);
            off = 4;
            break;
        default:
            break;
        }
        break;

    case OMCI_K_AUTONOMOUS:
        switch (omci_mt(m)) {
        case OMCI_MT_ALARM:
            if (content_len > OMCI_ALARM_BITMAP_LEN)
                return -1;
            /* bitmap at contents[0..27], 3 pad bytes, sequence at [31] */
            c[OMCI_ALARM_BITMAP_LEN + 3] = (uint8_t)m->seq;
            content_len = OMCI_ALARM_BITMAP_LEN;
            off = 0;
            break;
        case OMCI_MT_AVC:
            put16(c, m->attr_mask);
            off = 2;
            break;
        default:
            break;
        }
        break;

    default:
        break;
    }

    if (off + content_len > OMCI_CONTENT_MAX)
        return -1;
    memcpy(c + off, m->content, content_len);

    /* trailer: message length 0x0028, 2 pad bytes; then CRC-32 */
    buf[40] = 0x00;
    buf[41] = 0x28;
    crc = omci_crc32(buf, OMCI_FRAME_BASELINE_LEN - OMCI_CRC_LEN);
    buf[44] = (uint8_t)(crc >> 24);
    buf[45] = (uint8_t)(crc >> 16);
    buf[46] = (uint8_t)(crc >> 8);
    buf[47] = (uint8_t)crc;

    return OMCI_FRAME_BASELINE_LEN;
}

/* ---- Names -------------------------------------------------------------- */

const char *omci_mt_name(unsigned mt)
{
    switch (mt) {
    case OMCI_MT_CREATE:               return "Create";
    case OMCI_MT_SET_BY_REF:           return "Set by reference";
    case OMCI_MT_DELETE:               return "Delete";
    case OMCI_MT_SET:                  return "Set";
    case OMCI_MT_GET:                  return "Get";
    case OMCI_MT_GET_ALL_ALARMS:       return "Get all alarms";
    case OMCI_MT_GET_ALL_ALARMS_NEXT:  return "Get all alarms next";
    case OMCI_MT_MIB_UPLOAD:           return "MIB upload";
    case OMCI_MT_MIB_UPLOAD_NEXT:      return "MIB upload next";
    case OMCI_MT_MIB_RESET:            return "MIB reset";
    case OMCI_MT_ALARM:                return "Alarm";
    case OMCI_MT_AVC:                  return "Attribute value change";
    case OMCI_MT_TEST:                 return "Test";
    case OMCI_MT_START_SW_DOWNLOAD:    return "Start software download";
    case OMCI_MT_DOWNLOAD_SECTION:     return "Download section";
    case OMCI_MT_END_SW_DOWNLOAD:      return "End software download";
    case OMCI_MT_ACTIVATE_SW:          return "Activate software";
    case OMCI_MT_COMMIT_SW:            return "Commit software";
    case OMCI_MT_SYNC_TIME:            return "Synchronize time";
    case OMCI_MT_REBOOT:               return "Reboot";
    case OMCI_MT_GET_NEXT:             return "Get next";
    case OMCI_MT_TEST_RESULT:          return "Test result";
    case OMCI_MT_GET_CURRENT_DATA:     return "Get current data";
    case OMCI_MT_SET_TABLE:            return "Set table";
    default:                           return "Reserved";
    }
}

const char *omci_result_name(unsigned result)
{
    switch (result) {
    case OMCI_RES_SUCCESS:          return "Command processed successfully";
    case OMCI_RES_PROC_ERROR:       return "Command processing error";
    case OMCI_RES_NOT_SUPPORTED:    return "Command not supported";
    case OMCI_RES_PARAM_ERROR:      return "Command parameter error";
    case OMCI_RES_UNKNOWN_ME:       return "Unknown managed entity";
    case OMCI_RES_UNKNOWN_INSTANCE: return "Unknown managed entity instance";
    case OMCI_RES_DEVICE_BUSY:      return "Device busy";
    case OMCI_RES_INSTANCE_EXISTS:  return "Instance exists";
    case OMCI_RES_ATTR_FAILED:      return "Attribute(s) failed or unknown";
    default:                        return "Reserved";
    }
}

/* ---- Hex helpers -------------------------------------------------------- */

static int hexval(int ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

int omci_hex_parse(const char *hex, uint8_t *buf, size_t cap)
{
    size_t nibbles = 0;
    size_t n = 0;
    int hi = -1;
    const char *p;

    /* pass 1: validate so the buffer is left untouched on failure */
    for (p = hex; *p; p++)
        if (hexval((unsigned char)*p) >= 0)
            nibbles++;
    if (nibbles & 1)
        return -1;      /* dangling nibble */
    if (nibbles / 2 > cap)
        return -1;

    /* pass 2: fill */
    for (p = hex; *p; p++) {
        int v = hexval((unsigned char)*p);
        if (v < 0)
            continue;   /* separators: space, ':', '-', etc. */
        if (hi < 0) {
            hi = v;
        } else {
            buf[n++] = (uint8_t)((hi << 4) | v);
            hi = -1;
        }
    }
    return (int)n;
}

void omci_hex_format(const uint8_t *buf, size_t len, char *dst)
{
    static const char digits[] = "0123456789abcdef";
    size_t i;

    for (i = 0; i < len; i++) {
        dst[2 * i]     = digits[buf[i] >> 4];
        dst[2 * i + 1] = digits[buf[i] & 0xF];
    }
    dst[2 * len] = '\0';
}
