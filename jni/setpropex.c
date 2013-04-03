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

#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include "_system_properties.h"

extern struct prop_area *__system_property_area__;

#define  LOG_TAG    "setpropex"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
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
    if(len < 1) return -1;
    line[--len] = 0;

    if(mi == 0) return -1;

    mi->start = strtoul(line, 0, 16);
    mi->end = strtoul(line + 9, 0, 16);
    strncpy(mi->perm, line + 18, 4);

    if(len < 50) {
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
    if(fp == 0) return 0;
    
    mi = calloc(1, sizeof(mapinfo) + 16);

    while(!read_mapinfo(fp, mi)) {
//        LOGD("%08x %s %s\n", mi->start, mi->perm, mi->name);
        if(!strcmp(mi->perm, perm) && !strcmp(mi->name, name)){
            fclose(fp);
            mi->pid = pid;
            return mi;
        }
    }

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
     if(len > 4096) len = 4096;
     rd = read(fd, mem, len);
//     write(1, mem, len);
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

static int property_set_ex(const char *name, const char *value, int mem, mapinfo *mi)
{
    prop_area *pa;
    prop_info *pi;

    int namelen = strlen(name);
    int valuelen = strlen(value);

    if(namelen >= PROP_NAME_MAX) return -1;
    if(valuelen >= PROP_VALUE_MAX) return -1;
    if(namelen < 1) return -1;

    pi = (prop_info*) __system_property_find(name);

    if(pi != 0) {
//        LOGD("pi=%p name=%s value=%s", pi, pi->name, pi->value);
        pa = __system_property_area__;
        update_prop_info(pi, value, valuelen);
//        LOGD("pi=%p name=%s value=%s", pi, pi->name, pi->value);
//        LOGD("write %08x", mi->start + ((char*)pi - (char*)pa));
#if 0
        lseek64(mem, mi->start + ((char*)pi - (char*)pa), SEEK_SET);
        int ret = write(mem, pi, sizeof(*pi));
        LOGD("write ret=%d", ret);
#else
        {
          unsigned *addr = (unsigned*)(mi->start + ((char*)pi - (char*)pa));
          unsigned *data = (unsigned*)pi;
          for(int i=0; i<sizeof(*pi)/4; i++){
            ptrace(PTRACE_POKEDATA, mi->pid, (void*)addr, (void*)*data);
            addr++;
            data++;
          }
        }
#endif
        return 0;
    }
    return -1;
}

static int setpropex(int init_pid, int argc, char *argv[])
{
  if(argc != 3) {
    fprintf(stderr,"usage: setpropex <key> <value>\n");
    return 1;
  }
  mapinfo *mi = search_maps(init_pid, "rw-s", "/dev/__properties__ (deleted)");
  if(!mi) {
    mi = search_maps(init_pid, "rwxs", "/dev/__properties__ (deleted)");
  }
  if(!mi) {
    mi = search_maps(init_pid, "rwxs", "/dev/ashmem/system_properties (deleted)");
  }
  if(!mi) return 1;

  int ret = -1;

  LOGD("%08x %08x %s %s\n", mi->start, mi->end, mi->perm, mi->name);

  char tmp[128];
  sprintf(tmp, "/proc/%d/mem", init_pid);
  int mem = open(tmp, O_RDONLY);
  if(mem == -1) goto error2;

  void *p = malloc(mi->end - mi->start);
  if(p == 0) goto error1;

  dump_region(mem, mi->start, mi->end, p);
  __system_property_area__ = p;

  ret = property_set_ex(argv[1], argv[2], mem, mi);
  free(p);

error1:
  close(mem);
error2:
  free(mi);
  if(ret == 0){
    return 0;
  }

  if(property_set(argv[1], argv[2])){
    fprintf(stderr,"could not set property\n");
    return 1;
  }
  return 0;
}

int main(int argc, char** argv)
{
  int init_pid = 1; //TODO: find init process
  if(ptrace(PTRACE_ATTACH, init_pid, NULL, NULL) == -1) {
      fprintf(stderr, "ptrace error: failed to attach to %d, %s\n", init_pid, strerror(errno));
      return 1;
  }
  int ret = setpropex(init_pid, argc, argv);
  ptrace(PTRACE_DETACH, 1, NULL, NULL);
  return ret;
}
