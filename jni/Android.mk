LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= \
	setpropex.c system_properties.c
LOCAL_MODULE := setpropex
LOCAL_CFLAGS += -std=c99 -I jni/inc
LOCAL_LDLIBS    := -llog ../libcutils.so ../libc.so
include $(BUILD_EXECUTABLE)
