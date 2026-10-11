/*
 * omci/mib.c — managed-entity database implementation
 *
 * omcid2 — clean-room OMCI daemon for Lantiq Falcon GPON sticks
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "mib.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* baseline MIB upload next contents budget: 32 - 6 (reported class,
 * instance, mask) = 26 bytes of attribute data per image */
#define UPLOAD_CONTENT_BUDGET 26

int mib_init(struct mib *m)
{
    memset(m, 0, sizeof(*m));
    return OMCI_RES_SUCCESS;
}

void mib_free(struct mib *m)
{
    struct mib_inst *p = m->head;
    while (p) {
        struct mib_inst *n = p->next;
        free(p);
        p = n;
    }
    m->head = NULL;
    m->count = 0;
}

struct mib_inst *mib_find(struct mib *m, uint16_t cls, uint16_t inst)
{
    struct mib_inst *p;
    for (p = m->head; p; p = p->next)
        if (p->cls == cls && p->inst == inst)
            return p;
    return NULL;
}

static void mib_data_sync_bump(struct mib *m)
{
    /* stored in ME 2 instance when present */
    struct mib_inst *onu_data = mib_find(m, 2, 0);
    if (onu_data) {
        uint8_t v = onu_data->attrs[0];
        v = (uint8_t)(v + 1);
        onu_data->attrs[0] = v;
    }
}

uint8_t mib_data_sync_get(const struct mib *m)
{
    const struct mib_inst *onu_data = mib_find((struct mib *)m, 2, 0);
    return onu_data ? onu_data->attrs[0] : 0;
}

void mib_data_sync_set(struct mib *m, uint8_t v)
{
    struct mib_inst *onu_data = mib_find(m, 2, 0);
    if (onu_data)
        onu_data->attrs[0] = v;
}

int mib_create(struct mib *m, uint16_t cls, uint16_t inst,
               const uint8_t *attr_bytes, uint16_t len)
{
    const struct omci_class_def *cd = omci_class_find(cls);
    struct mib_inst *e, *p, **pp;
    int area;

    if (!cd)
        return OMCI_RES_UNKNOWN_ME;
    if (mib_find(m, cls, inst))
        return OMCI_RES_INSTANCE_EXISTS;

    area = omci_class_attr_area_len(cd);
    if (area < 0 || (unsigned)area > MIB_ATTRS_MAX)
        return OMCI_RES_NOT_SUPPORTED;

    e = calloc(1, sizeof(*e));
    if (!e)
        return OMCI_RES_DEVICE_BUSY;
    e->cls = cls;
    e->inst = inst;
    e->attrs_len = (uint16_t)area;
    if (attr_bytes && len) {
        size_t n = len < (size_t)area ? len : (size_t)area;
        memcpy(e->attrs, attr_bytes, n);
    }

    /* insert ordered by (class, instance) */
    for (pp = &m->head; (p = *pp); pp = &p->next) {
        if (p->cls > cls || (p->cls == cls && p->inst > inst))
            break;
    }
    e->next = *pp;
    *pp = e;
    m->count++;

    mib_data_sync_bump(m);
    return OMCI_RES_SUCCESS;
}

int mib_delete(struct mib *m, uint16_t cls, uint16_t inst)
{
    struct mib_inst *p, **pp;

    for (pp = &m->head; (p = *pp); pp = &p->next) {
        if (p->cls == cls && p->inst == inst) {
            *pp = p->next;
            free(p);
            m->count--;
            mib_data_sync_bump(m);
            return OMCI_RES_SUCCESS;
        }
    }
    return OMCI_RES_UNKNOWN_INSTANCE;
}

/* pack attribute values selected by mask from an instance */
static int pack_attrs(const struct omci_class_def *cd,
                      const struct mib_inst *e, uint16_t mask,
                      uint8_t *out, size_t cap, size_t *out_len)
{
    size_t off = 0;
    unsigned n;

    for (n = 1; n <= cd->num_attrs; n++) {
        const struct omci_attr_def *ad = &cd->attrs[n - 1];
        int sz = omci_attr_size(cd, n);
        int src;

        if (!(mask & ad->mask))
            continue;
        if (sz <= 0)
            continue;           /* unsized table */
        src = omci_attr_offset(cd, n);
        if (src < 0)
            return OMCI_RES_PROC_ERROR;
        if (off + (size_t)sz > cap)
            return OMCI_RES_PARAM_ERROR;
        memcpy(out + off, e->attrs + src, (size_t)sz);
        off += (size_t)sz;
    }
    *out_len = off;
    return OMCI_RES_SUCCESS;
}

/* unpack packed values (mask order) into an instance */
static int unpack_attrs(const struct omci_class_def *cd,
                        struct mib_inst *e, uint16_t mask,
                        const uint8_t *val, uint16_t len)
{
    size_t off = 0;
    unsigned n;

    for (n = 1; n <= cd->num_attrs; n++) {
        const struct omci_attr_def *ad = &cd->attrs[n - 1];
        int sz = omci_attr_size(cd, n);
        int dst;

        if (!(mask & ad->mask))
            continue;
        if (sz <= 0)
            continue;
        if (off + (size_t)sz > len)
            return OMCI_RES_PARAM_ERROR;
        dst = omci_attr_offset(cd, n);
        if (dst < 0)
            return OMCI_RES_PROC_ERROR;
        memcpy(e->attrs + dst, val + off, (size_t)sz);
        off += (size_t)sz;
    }
    return OMCI_RES_SUCCESS;
}

int mib_set(struct mib *m, uint16_t cls, uint16_t inst, uint16_t mask,
            const uint8_t *val, uint16_t len)
{
    const struct omci_class_def *cd = omci_class_find(cls);
    struct mib_inst *e = mib_find(m, cls, inst);
    int rc;

    if (!cd)
        return OMCI_RES_UNKNOWN_ME;
    if (!e)
        return OMCI_RES_UNKNOWN_INSTANCE;

    rc = unpack_attrs(cd, e, mask, val, len);
    if (rc != OMCI_RES_SUCCESS)
        return rc;
    mib_data_sync_bump(m);
    return OMCI_RES_SUCCESS;
}

int mib_get(struct mib *m, uint16_t cls, uint16_t inst, uint16_t mask,
            uint8_t *out, size_t cap, size_t *out_len)
{
    const struct omci_class_def *cd = omci_class_find(cls);
    struct mib_inst *e = mib_find(m, cls, inst);

    if (!cd)
        return OMCI_RES_UNKNOWN_ME;
    if (!e)
        return OMCI_RES_UNKNOWN_INSTANCE;
    return pack_attrs(cd, e, mask, out, cap, out_len);
}

int mib_get_next(struct mib *m, uint16_t cls, uint16_t inst, uint16_t mask,
                 uint8_t *out, size_t cap, size_t *out_len,
                 uint16_t *next_inst)
{
    const struct omci_class_def *cd = omci_class_find(cls);
    struct mib_inst *p, *first = NULL, *cand = NULL;

    if (!cd)
        return OMCI_RES_UNKNOWN_ME;

    for (p = m->head; p; p = p->next) {
        if (p->cls != cls)
            continue;
        if (!first)
            first = p;
        if (p->inst > inst && (!cand || p->inst < cand->inst))
            cand = p;
    }
    if (!cand)
        cand = first;      /* wrap */
    if (!cand)
        return OMCI_RES_UNKNOWN_INSTANCE;

    *next_inst = cand->inst;
    return pack_attrs(cd, cand, mask, out, cap, out_len);
}

void mib_reset(struct mib *m)
{
    mib_free(m);
}

/* ---- MIB upload --------------------------------------------------------- */

/* Split the uploadable attributes of a class into consecutive runs that
 * fit the baseline 26-byte budget. Returns the chunk mask for image img
 * (0-based) and the total image count via *total. */
static uint16_t upload_chunk(const struct omci_class_def *cd,
                             uint16_t upmask, unsigned img,
                             unsigned *total)
{
    uint16_t chunk = 0;
    unsigned cur = 0, n;
    size_t run = 0;

    *total = 1;
    for (n = 1; n <= cd->num_attrs; n++) {
        const struct omci_attr_def *ad = &cd->attrs[n - 1];
        int sz;

        if (!(upmask & ad->mask))
            continue;
        sz = omci_attr_size(cd, n);
        if (sz <= 0)
            continue;
        if (run + (size_t)sz > UPLOAD_CONTENT_BUDGET) {
            cur++;
            run = 0;
        }
        if (cur == img)
            chunk |= ad->mask;
        run += (size_t)sz;
    }
    *total = cur + 1;
    return chunk;
}

unsigned mib_upload_num(struct mib *m)
{
    unsigned total = 0;
    struct mib_inst *p;

    for (p = m->head; p; p = p->next) {
        const struct omci_class_def *cd = omci_class_find(p->cls);
        unsigned images;
        uint16_t upmask;

        if (!cd)
            continue;
        upmask = omci_class_upload_mask(cd);
        if (!upmask)
            continue;
        upload_chunk(cd, upmask, 0, &images);
        total += images;
    }
    return total;
}

int mib_upload_next(struct mib *m, unsigned seq,
                    uint16_t *cls, uint16_t *inst, uint16_t *mask,
                    uint8_t *out, size_t cap, size_t *out_len)
{
    struct mib_inst *p;
    unsigned idx = 0;

    for (p = m->head; p; p = p->next) {
        const struct omci_class_def *cd = omci_class_find(p->cls);
        unsigned images, img;
        uint16_t upmask, chunk;

        if (!cd)
            continue;
        upmask = omci_class_upload_mask(cd);
        if (!upmask)
            continue;
        upload_chunk(cd, upmask, 0, &images);

        if (seq < idx || seq >= idx + images) {
            idx += images;
            continue;
        }
        img = seq - idx;
        chunk = upload_chunk(cd, upmask, img, &images);
        *cls = p->cls;
        *inst = p->inst;
        *mask = chunk;
        return pack_attrs(cd, p, chunk, out, cap, out_len);
    }
    return OMCI_RES_UNKNOWN_INSTANCE;   /* seq out of range */
}

/* ---- dumps --------------------------------------------------------------- */

static int emit_line(int (*emit)(const char *line, void *ud), void *ud,
                     const char *fmt, ...)
{
    char buf[512];
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n < 0)
        return -1;
    return emit(buf, ud);
}

#define DASHES "-------------------------------------------------------------------------------"

static int dump_instance(struct mib_inst *e,
                         int (*emit)(const char *line, void *ud), void *ud)
{
    const struct omci_class_def *cd = omci_class_find(e->cls);
    unsigned n;
    char flags[12];

    if (!cd)
        return OMCI_RES_UNKNOWN_ME;

    emit_line(emit, ud, "Class ID    = %u (%s)", e->cls, cd->name);
    emit_line(emit, ud, "Instance ID = %u", e->inst);
    emit_line(emit, ud, "Upload      = %s", "yes");
    emit_line(emit, ud, "Alarms      = %s", "-");
    emit_line(emit, ud, "%s", DASHES);

    for (n = 1; n <= cd->num_attrs; n++) {
        const struct omci_attr_def *ad = &cd->attrs[n - 1];
        int sz = omci_attr_size(cd, n);
        int off = 0;
        char hexline[3 * 40 + 1];
        char escline[4 * 16 + 1];
        size_t hl = 0, el = 0;
        int j;

        if (sz <= 0) {
            /* unsized table attribute: header only */
            omci_attr_flags_str(ad, flags);
            emit_line(emit, ud, " %2u %-29s%3db %-4s %s",
                      n, ad->name, 0, omci_attr_type_str(ad), flags);
            emit_line(emit, ud, "%s", DASHES);
            continue;
        }

        omci_attr_flags_str(ad, flags);
        emit_line(emit, ud, " %2u %-29s%3db %-4s %s",
                  n, ad->name, sz, omci_attr_type_str(ad), flags);

        if (ad->type == A_U8 || ad->type == A_U16 || ad->type == A_U32) {
            /* numeric: one line "0x.. (decimal)" */
            unsigned long v = 0;
            for (j = 0; j < sz; j++)
                v = (v << 8) | e->attrs[off + j];
            if (ad->type == A_U8)
                emit_line(emit, ud, "   0x%02lx (%lu)", v, v);
            else if (ad->type == A_U16)
                emit_line(emit, ud, "   0x%04lx (%lu)", v, v);
            else
                emit_line(emit, ud, "   0x%08lx (%lu)", v, v);
        } else {
            /* string/bytes: hex byte list + escaped ASCII line */
            for (j = 0; j < sz && hl + 4 < sizeof(hexline); j++)
                hl += (size_t)snprintf(hexline + hl, sizeof(hexline) - hl,
                                       "%s0x%02x", j ? " " : "",
                                       e->attrs[off + j]);
            emit_line(emit, ud, "   %s", hexline);
            for (j = 0; j < sz && el + 5 < sizeof(escline); j++) {
                uint8_t c = e->attrs[off + j];
                if (c >= 0x20 && c < 0x7f && c != '\\' && c != '%')
                    el += (size_t)snprintf(escline + el,
                                           sizeof(escline) - el,
                                           "%c", c);
                else
                    el += (size_t)snprintf(escline + el,
                                           sizeof(escline) - el,
                                           "\\x%02X", c);
            }
            emit_line(emit, ud, "   %s", escline);
        }
        emit_line(emit, ud, "%s", DASHES);
    }

    emit_line(emit, ud, "R - Readable          O - Not supported (optional)");
    emit_line(emit, ud, "W - Writable          E - Excluded from MIB upload (template)");
    emit_line(emit, ud, "S - set-by-create     T - Table");
    emit_line(emit, ud, "A - Send AVC          V - Volatile");
    emit_line(emit, ud, "U - No upload         P - No swap");
    emit_line(emit, ud, "N - Not supported     Y - Partly supported");
    return OMCI_RES_SUCCESS;
}

int mib_dump_me(struct mib *m, uint16_t cls, uint16_t inst,
                int (*emit)(const char *line, void *ud), void *ud)
{
    struct mib_inst *e = mib_find(m, cls, inst);
    int rc;

    if (!omci_class_find(cls))
        return OMCI_RES_UNKNOWN_ME;
    if (!e)
        return OMCI_RES_UNKNOWN_INSTANCE;
    rc = dump_instance(e, emit, ud);
    return rc;
}

int mib_dump_all(struct mib *m, uint16_t cls,
                 int (*emit)(const char *line, void *ud), void *ud)
{
    struct mib_inst *p;
    int rc = OMCI_RES_SUCCESS;

    for (p = m->head; p; p = p->next) {
        if (cls && p->cls != cls)
            continue;
        rc = dump_instance(p, emit, ud);
        if (rc != OMCI_RES_SUCCESS)
            break;
    }
    return rc;
}
