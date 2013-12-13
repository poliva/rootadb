/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _INCLUDE_SYS__SYSTEM_PROPERTIES_H
#define _INCLUDE_SYS__SYSTEM_PROPERTIES_H

#ifndef _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#error you should #include <sys/system_properties.h> instead
#else
#include <sys/system_properties.h>

#include <stdbool.h>
#include <sys/types.h>


/*
 * definitions for both versions
 */
#define PROP_AREA_MAGIC   0x504f5250
#define PROP_AREA_VERSION 0xfc6ed0ab
#define PROP_AREA_VERSION_COMPAT 0x45434f76

extern bool compat_mode;

extern size_t pa_size;
extern size_t pa_data_size;


/*
 * structures and definitions for >= kitkat
 */

struct prop_area {
    unsigned bytes_used;
    unsigned volatile serial;
    unsigned magic;
    unsigned version;
    unsigned reserved[28];
    char data[0];
};

typedef struct prop_area prop_area;

struct prop_info {
    unsigned volatile serial;
    char value[PROP_VALUE_MAX];
    char name[0];
};

typedef struct prop_info prop_info;


/*
 * structures and definitions for < kitkat
 */

struct prop_area_compat {
    unsigned volatile count;
    unsigned volatile serial;
    unsigned magic;
    unsigned version;
    unsigned toc[1];
};

typedef struct prop_area_compat prop_area_compat;

struct prop_area;
typedef struct prop_area prop_area;

struct prop_info_compat {
    char name[PROP_NAME_MAX];
    unsigned volatile serial;
    char value[PROP_VALUE_MAX];
};

typedef struct prop_info_compat prop_info_compat;

const prop_info *__system_property_find_compat(const char *name);

#endif

#endif

