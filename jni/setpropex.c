#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ptrace.h>
#include <sys/mman.h>
#include <cutils/properties.h>
#include <jni.h>
#include <inttypes.h>

/* include the common system property implementation definitions */
#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include "_system_properties.h"

extern struct prop_area *__system_property_area__;

#define  LOG_TAG    "setpropex"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#ifndef NDEBUG
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#else
#define  LOGD(...)
#endif
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#include <android/log.h>

typedef struct mapinfo mapinfo;

struct mapinfo {
    mapinfo *next;
    int pid;
    uintptr_t start;
    uintptr_t end;
    char perm[8];
    char name[1024];
};

// 6f000000-6f01e000 rwxp 00000000 00:0c 16389419   /android/lib/libcomposer.so
// 7f8e960000-7f8e980000 rw-s 00000000 00:0e 2011                           /dev/__properties__
// 01234567890123456789012345678901234567890123456789012345678901234567890123456789
// 0         1         2         3         4         5         6         7

static int read_mapinfo(FILE *fp, mapinfo* mi)
{
    char line[1024];
    int len;

skip:
    if(fgets(line, 1024, fp) == 0) return -1;

    len = strlen(line);
    if(len < 1) {
        LOGE("read_mapinfo: short line!");
        return -1;
    }
    line[--len] = 0;

    if(mi == 0) {
        LOGE("read_mapinfo: NULL mapinfo!");
        return -1;
    }

    char* end = strchr(line, '-') + 1;
    char* perm = strchr(line, ' ') + 1;
    if ((end == NULL) || (perm == NULL)) {
        LOGD("read_mapinfo: end/perm == NULL");
        goto skip;
    }

#ifdef __LP64__
    mi->start = strtoull(line, 0, 16);
    mi->end = strtoull(end, 0, 16);
#else
    mi->start = strtoul(line, 0, 16);
    mi->end = strtoul(end, 0, 16);
#endif
    strncpy(mi->perm, perm, 4);

    if(len < 50) {
        LOGD("read_mapinfo: line too short! (%d bytes) skipping", len);
        //mi->name[0] = '\0';
        goto skip;
    } else {
#ifdef __LP64__
        strcpy(mi->name, line + 73);
#else
        strcpy(mi->name, line + 49);
#endif
    }

    return 0;
}

static mapinfo *load_maps(int pid)
{
    char tmp[128];
    FILE *fp;
    mapinfo *mi, *last, *ret;
    
    sprintf(tmp, "/proc/%d/maps", pid);
    fp = fopen(tmp, "r");
    if(fp == 0) {
        LOGE("load_maps: unable to open maps file: %s", strerror(errno));
        return NULL;
    }

    ret = NULL;
    last = NULL;
    mi = malloc(sizeof(mapinfo));
    memset(mi, 0, sizeof(mapinfo));
    while(!read_mapinfo(fp, mi)) {
        LOGD("load_maps: %"PRIxPTR" %s %s", mi->start, mi->perm, mi->name);
        mi->pid = pid;
        if (ret == NULL) ret = mi;
        if (last != NULL) last->next = mi;
        last = mi;
        mi = malloc(sizeof(mapinfo));
        memset(mi, 0, sizeof(mapinfo));
    }

    fclose(fp);
    free(mi);
    return ret;
}

static void dump_region(int fd, uintptr_t start, uintptr_t end, char* mem)
{
    lseek64(fd, start, SEEK_SET);
    while(start < end) {
        int rd;
        int len;

        len = end - start;
        if(len > 4096)
            len = 4096;
        rd = read(fd, mem, len);
        if (rd != len)
            LOGE("dump_region: short read!");
        //write(1, mem, len);
        start += 4096;
        mem += 4096;
    }
}


/* set it in init's memory */
static void update_init(mapinfo *mi, void *pa, void *pi, size_t len)
{
    uintptr_t offset = (pi - pa);
    uintptr_t *addr = (uintptr_t*)(mi->start + offset);
    uintptr_t *data = (uintptr_t*)pi;
    int ret;

    LOGD("write %"PRIxPTR, mi->start + offset);
    for (int i = 0;  i < len / 4; i++) {
        ret = ptrace(PTRACE_POKEDATA, mi->pid, (void*)(addr + i), (void*)data[i]);
        if (ret != 0) {
            LOGE("ptrace POKEDATA: %s", strerror(errno));
            break;
        }
    }
}


static void update_prop_info(mapinfo *mi, void *pa, prop_info *pi, const char *value, unsigned len)
{
    LOGD("new/before: pi=%p name=%s value=%s", pi, pi->name, pi->value);
    pi->serial = pi->serial | 1;
    memcpy(pi->value, value, len + 1);
    pi->serial = (len << 24) | ((pi->serial + 1) & 0xffffff);
    LOGD("new/after: pi=%p name=%s value=%s", pi, pi->name, pi->value);

    /* update init */
    update_init(mi, pa, pi, sizeof(*pi));
}

static void update_prop_info_compat(mapinfo *mi, void *pa, prop_info_compat *pi, const char *value, unsigned len)
{
    LOGD("old/before: pi=%p name=%s value=%s", pi, pi->name, pi->value);
    pi->serial = pi->serial | 1;
    memcpy(pi->value, value, len + 1);
    pi->serial = (len << 24) | ((pi->serial + 1) & 0xffffff);
    LOGD("old/after: pi=%p name=%s value=%s", pi, pi->name, pi->value);

    /* update init */
    update_init(mi, pa, pi, sizeof(*pi));
}

/* first, set it in our memory, then in init'd memory */
static int property_set_ex(const char *name, const char *value, int mem, mapinfo *mi)
{
    void *pi;

    int namelen = strlen(name);
    int valuelen = strlen(value);

    if(namelen >= PROP_NAME_MAX) {
        LOGE("name too long!");
        return -1;
    }
    if(valuelen >= PROP_VALUE_MAX) {
        LOGE("value too long!");
        return -1;
    }
    if(namelen < 1) {
        LOGE("name too short!");
        return -1;
    }

    pi = (void *)__system_property_find(name);

    if(pi != 0) {
        char *pa = (char *)__system_property_area__;

        if (compat_mode)
            update_prop_info_compat(mi, pa, pi, value, valuelen);
        else
            update_prop_info(mi, pa, pi, value, valuelen);

        return 0;
    }

    return -1;
}

static int setpropex(int init_pid, int argc, char *argv[])
{
    int ret = -1;
    char tmp[128];
    mapinfo *mi;

    if(argc != 3) {
        fprintf(stderr, "usage: setpropex <key> <value>\n");
        return 1;
    }

    /* open it up, so we can read a copy */
    sprintf(tmp, "/proc/%d/mem", init_pid);
    int mem = open(tmp, O_RDONLY);
    if(mem == -1) {
        LOGE("unable to open init's mem: %s", strerror(errno));
        goto error2;
    }

    mi = load_maps(init_pid);
    while(mi != NULL) {
        if (
                /* try several different strategies to find the property area in init */
                (!strcmp(mi->perm, "rw-s") && !strcmp(mi->name, "/dev/__properties__")) ||
                (!strcmp(mi->perm, "rw-s") && !strcmp(mi->name, "/dev/__properties__ (deleted)")) ||
                (!strcmp(mi->perm, "rwxs") && !strcmp(mi->name, "/dev/__properties__ (deleted)")) ||
                (!strcmp(mi->perm, "rwxs") && !strcmp(mi->name, "/dev/ashmem/system_properties (deleted)")) ||

                /* property spaces split per SELinux type */
                (!strcmp(mi->perm, "rw-s") && !strncmp(mi->name, "/dev/__properties__/u", strlen("/dev/__properties__/u")))
        ) {
            LOGD("map found @ %"PRIxPTR" %"PRIxPTR" %s %s", mi->start, mi->end, mi->perm, mi->name);

            pa_size = mi->end - mi->start;
            pa_data_size = pa_size - sizeof(prop_area);

            /* allocate memory for our copy */
            void *p = malloc(pa_size);
            if(p == 0) {
                LOGE("unable to allocate memory for our copy of the property area");
                goto error1;
            }

            /* got it, read the data and set it for the system_property code */
            dump_region(mem, mi->start, mi->end, p);
            __system_property_area__ = p;

            /* detect old versions of android and use the correct implementation
             * accordingly */
            if (__system_property_area__->version == PROP_AREA_VERSION_COMPAT)
                compat_mode = true;

            /* set the property */
            ret = property_set_ex(argv[1], argv[2], mem, mi);

            /* clean up */
            free(p);

            if (ret == 0)
                break;
        }

        mi = mi->next;
    }

    if(ret == -1){
        LOGE("didn't find the property!");
    }

error1:
    close(mem);
error2:
    if(ret == 0){
        return 0;
    }

    return 1;
}


int main(int argc, char** argv)
{
    int init_pid = 1; //TODO: find init process

    if(ptrace(PTRACE_ATTACH, init_pid, NULL, NULL) == -1) {
        LOGE("ptrace error: failed to attach to %d, %s", init_pid, strerror(errno));
        return 1;
    }

    int ret = setpropex(init_pid, argc, argv);

    ptrace(PTRACE_DETACH, 1, NULL, NULL);

    return ret;
}
