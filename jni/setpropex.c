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
    unsigned start;
    unsigned end;
    char perm[8];
    char name[1024];
};

// 6f000000-6f01e000 rwxp 00000000 00:0c 16389419   /android/lib/libcomposer.so
// 012345678901234567890123456789012345678901234567890123456789
// 0         1         2         3         4         5

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

    mi->start = strtoul(line, 0, 16);
    mi->end = strtoul(line + 9, 0, 16);
    strncpy(mi->perm, line + 18, 4);

    if(len < 50) {
        LOGE("read_mapinfo: line too short! (%d bytes) skipping", len);
        //mi->name[0] = '\0';
	    goto skip;
    } else {
        strcpy(mi->name, line + 49);
    }

    return 0;
}

static mapinfo *search_maps(int pid, const char* perm, const char* name)
{
    char tmp[128];
    FILE *fp;
    mapinfo *mi;
    
    sprintf(tmp, "/proc/%d/maps", pid);
    fp = fopen(tmp, "r");
    if(fp == 0) {
        LOGE("search_maps: unable to open maps file: %s", strerror(errno));
        return 0;
    }
    
    mi = calloc(1, sizeof(mapinfo) + 16);

    while(!read_mapinfo(fp, mi)) {
        //LOGD("%08x %s %s", mi->start, mi->perm, mi->name);
        if(!strcmp(mi->perm, perm) && !strcmp(mi->name, name)){
            fclose(fp);
            mi->pid = pid;
            return mi;
        }
    }

    LOGE("search_maps: map \"%s\" not found!", name);
    fclose(fp);
    free(mi);
    return 0;
}

static void dump_region(int fd, unsigned start, unsigned end, char* mem)
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

static void update_prop_info(prop_info *pi, const char *value, unsigned len)
{
    pi->serial = pi->serial | 1;
    memcpy(pi->value, value, len + 1);
    pi->serial = (len << 24) | ((pi->serial + 1) & 0xffffff);
}

/* first, set it in our memory, then in init'd memory */
static int property_set_ex(const char *name, const char *value, int mem, mapinfo *mi)
{
    prop_area *pa;
    prop_info *pi;

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

    pi = (prop_info*) __system_property_find(name);

    if(pi != 0) {
        LOGD("before: pi=%p name=%s value=%s", pi, pi->name, pi->value);
        pa = __system_property_area__;
        update_prop_info(pi, value, valuelen);
        LOGD("after: pi=%p name=%s value=%s", pi, pi->name, pi->value);
        LOGD("write %08x", mi->start + ((char*)pi - (char*)pa));
#if 0
        lseek64(mem, mi->start + ((char*)pi - (char*)pa), SEEK_SET);
        int ret = write(mem, pi, sizeof(*pi));
        LOGD("write ret=%d", ret);
#else
        /* set it in init's memory */
        {
            unsigned *addr = (unsigned*)(mi->start + ((char*)pi - (char*)pa));
            unsigned *data = (unsigned*)pi;
            int ret;

            for(int i=0; i<sizeof(*pi)/4; i++){
                ret = ptrace(PTRACE_POKEDATA, mi->pid, (void*)addr, (void*)*data);
                if (ret != 0) {
                    LOGE("ptrace POKEDATA: %s", strerror(errno));
                    break;
                }
                addr++;
                data++;
            }
        }
#endif
        return 0;
    }
    else
        LOGE("didn't find the property!");
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

    /* try several different strategies to find the property area in init */
    mi = search_maps(init_pid, "rw-s", "/dev/__properties__");
    if(!mi)
        mi = search_maps(init_pid, "rw-s", "/dev/__properties__ (deleted)");
    if(!mi)
        mi = search_maps(init_pid, "rwxs", "/dev/__properties__ (deleted)");
    if(!mi)
        mi = search_maps(init_pid, "rwxs", "/dev/ashmem/system_properties (deleted)");
    if(!mi) {
        LOGE("didn't find mapinfo!");
        return 1;
    }
    LOGD("map found @ %08x %08x %s %s", mi->start, mi->end, mi->perm, mi->name);

    /* open it up, so we can read a copy */
    sprintf(tmp, "/proc/%d/mem", init_pid);
    int mem = open(tmp, O_RDONLY);
    if(mem == -1) {
        LOGE("unable to open init's mem: %s", strerror(errno));
        goto error2;
    }

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

error1:
    close(mem);
error2:
    free(mi);
    if(ret == 0){
        return 0;
    }

    if(property_set(argv[1], argv[2]))
        return 1;

    return 0;
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
