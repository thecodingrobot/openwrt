/*
 * platform/ini.c — stick MIB seed (.ini) loader implementation
 *
 * omcid2 — clean-room OMCI daemon for Lantiq Falcon GPON sticks
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "ini.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define INI_LINE_MAX 1024

/* ---- tokenizer ----------------------------------------------------------- */

/* token types */
enum tok_kind { TOK_END, TOK_WORD };

struct tokenizer {
    const char *p;
};

/* Fetch the next attribute token into buf (unescaped, may contain
 * spaces or NUL bytes; *len receives the byte count). Returns 0 on
 * success, 1 at end of line, -1 on error. */
static int next_token(struct tokenizer *t, char *buf, size_t cap, size_t *len)
{
    const char *p = t->p;
    size_t n = 0;
    char quote = 0;

    while (*p == ' ' || *p == '\t')
        p++;

    if (*p == '\0') {
        t->p = p;
        *len = 0;
        return 1;
    }

    if (*p == '"' || *p == '\'') {
        quote = *p++;
    }

    while (*p) {
        char c = *p;

        if (quote) {
            if (c == quote) {
                p++;
                break;
            }
        } else if (c == ' ' || c == '\t') {
            break;
        }

        if (c == '\\') {
            p++;
            switch (*p) {
            case 's': c = ' ';  p++; break;
            case '0': c = '\0'; p++; break;
            case '\\': c = '\\'; p++; break;
            case 'x': {
                p++;
                if (isxdigit((unsigned char)p[0]) &&
                    isxdigit((unsigned char)p[1])) {
                    char hex[3] = { p[0], p[1], 0 };
                    c = (char)strtol(hex, NULL, 16);
                    p += 2;
                } else {
                    return -1;
                }
                break;
            }
            default:
                return -1;
            }
            if (n + 1 >= cap)
                return -1;
            buf[n++] = c;
            continue;       /* p already past the escape */
        }

        if (n + 1 >= cap)
            return -1;
        buf[n++] = c;
        p++;
    }

    buf[n] = '\0';
    t->p = p;
    *len = n;
    return 0;
}

/* parse a number token (decimal, 0x hex, 0 octal) */
static int parse_num(const char *tok, unsigned long *out)
{
    char *end;
    unsigned long v;

    if (!isdigit((unsigned char)tok[0]) && tok[0] != '+')
        return -1;
    v = strtoul(tok, &end, 0);
    if (*end != '\0')
        return -1;
    *out = v;
    return 0;
}

/* Convert one attribute token to bytes according to the attribute type.
 * Numeric attributes accept decimal/0x-hex/0-octal numbers or a raw
 * big-endian byte sequence of exactly the attribute size (\xHH escapes). */
static int token_to_bytes(const struct omci_attr_def *ad, const char *tok,
                          size_t tok_len, uint8_t *dst, int size)
{
    unsigned long v;
    int i;

    switch (ad->type) {
    case A_U8:
    case A_U16:
    case A_U32:
        if (parse_num(tok, &v) == 0) {
            for (i = 0; i < size; i++)
                dst[i] = (uint8_t)(v >> (8 * (size - 1 - i)));
            return 0;
        }
        /* fall through: raw bytes for \xHH-style tokens */
        if (tok_len != (size_t)size)
            return -1;
        memcpy(dst, tok, (size_t)size);
        return 0;
    case A_STR:
    case A_BYTES:
    case A_TABLE:
    default:
        /* literal bytes, right-padded with NUL */
        {
            size_t len = tok_len;
            size_t n = len < (size_t)size ? len : (size_t)size;
            if (ad->type == A_TABLE) {
                /* table token bytes must match the slot exactly */
                if (len != (size_t)size)
                    return -1;
                n = len;
            }
            memset(dst, 0, (size_t)size);
            memcpy(dst, tok, n);
            return 0;
        }
    }
}

/* ---- loader --------------------------------------------------------------- */

int ini_load_stream(FILE *f, struct mib *m,
                    void (*warn)(const char *msg, void *ud), void *ud)
{
    char line[INI_LINE_MAX];
    int lineno = 0;
    int created = 0;

    while (fgets(line, sizeof(line), f)) {
        struct tokenizer t;
        char tok[512];
        unsigned long cls, inst;
        const struct omci_class_def *cd;
        uint8_t area[MIB_ATTRS_MAX];
        int area_len;
        unsigned attr_no;
        int rc;
        char msg[128];

        lineno++;

        /* strip comment and line ending */
        {
            char *h = strchr(line, '#');
            if (h)
                *h = '\0';
            line[strcspn(line, "\r\n")] = '\0';
        }

        size_t tok_len = 0;

        t.p = line;
        if (next_token(&t, tok, sizeof(tok), &tok_len) != 0)
            continue;                       /* blank */
        if (parse_num(tok, &cls) != 0) {
            snprintf(msg, sizeof(msg), "%d: bad class id", lineno);
            if (warn) warn(msg, ud);
            continue;
        }
        if (next_token(&t, tok, sizeof(tok), &tok_len) != 0 ||
            parse_num(tok, &inst) != 0) {
            snprintf(msg, sizeof(msg), "%d: bad instance id", lineno);
            if (warn) warn(msg, ud);
            continue;
        }

        cd = omci_class_find((uint16_t)cls);
        if (!cd) {
            snprintf(msg, sizeof(msg), "%d: unsupported class %lu",
                     lineno, cls);
            if (warn) warn(msg, ud);
            continue;
        }
        memset(area, 0, sizeof(area));
        area_len = omci_class_attr_area_len(cd);
        if (area_len < 0 || area_len > MIB_ATTRS_MAX) {
            snprintf(msg, sizeof(msg), "%d: class %lu too large",
                     lineno, cls);
            if (warn) warn(msg, ud);
            continue;
        }

        /* attributes in file order == schema order */
        for (attr_no = 1; attr_no <= cd->num_attrs; attr_no++) {
            const struct omci_attr_def *ad = omci_attr(cd, attr_no);
            int sz = omci_attr_size(cd, attr_no);
            int at_off;

            rc = next_token(&t, tok, sizeof(tok), &tok_len);
            if (rc == 1) {
                /* rest of the attributes stay default (e.g. ME 7 lines) */
                break;
            }
            if (rc < 0) {
                snprintf(msg, sizeof(msg), "%d: bad token in class %lu",
                         lineno, cls);
                if (warn) warn(msg, ud);
                break;
            }
            if (sz <= 0)
                continue;                   /* unsized table: skip token */
            at_off = omci_attr_offset(cd, attr_no);
            if (at_off < 0 || at_off + sz > area_len)
                continue;
            if (token_to_bytes(ad, tok, tok_len, area + at_off, sz) != 0) {
                snprintf(msg, sizeof(msg),
                         "%d: bad value for class %lu attr %u",
                         lineno, cls, attr_no);
                if (warn) warn(msg, ud);
            }
        }

        rc = mib_create(m, (uint16_t)cls, (uint16_t)inst,
                        area, (uint16_t)area_len);
        if (rc == OMCI_RES_SUCCESS) {
            created++;
        } else {
            snprintf(msg, sizeof(msg), "%d: create class %lu inst %lu: %d",
                     lineno, cls, inst, rc);
            if (warn) warn(msg, ud);
        }
    }

    /* seeding is not an OLT-visible MIB change */
    mib_data_sync_set(m, 0);

    return created;
}

int ini_load_file(const char *path, struct mib *m,
                  void (*warn)(const char *msg, void *ud), void *ud)
{
    FILE *f = fopen(path, "r");
    int n;

    if (!f)
        return -1;
    n = ini_load_stream(f, m, warn, ud);
    fclose(f);
    return n;
}
