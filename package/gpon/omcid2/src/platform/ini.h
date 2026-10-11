/*
 * platform/ini.h — stick MIB seed (.ini) loader
 *
 * omcid2 — clean-room OMCI daemon for Lantiq Falcon GPON sticks
 *
 * File format (see specs/interface-b.md section 4):
 *   <class id> <instance id> <attribute 1> ... <attribute N>
 * one ME instance per line; '#' comments and blank lines ignored;
 * string tokens may be quoted and use \\s, \\0 and \\xHH escapes;
 * numbers are decimal, 0x-prefixed hex or 0-prefixed octal.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef OMCI2_INI_H
#define OMCI2_INI_H

#include <stdio.h>

#include "omci/mib.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Load a seed MIB from an already-open stream into m (instances are
 * appended; usually called on an empty db). Returns the number of
 * instances created, or -1 on hard I/O error. Individual line errors
 * are reported via warn (may be NULL). */
int ini_load_stream(FILE *f, struct mib *m,
                    void (*warn)(const char *msg, void *ud), void *ud);

/* Convenience wrapper. */
int ini_load_file(const char *path, struct mib *m,
                  void (*warn)(const char *msg, void *ud), void *ud);

#ifdef __cplusplus
}
#endif
#endif /* OMCI2_INI_H */
