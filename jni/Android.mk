LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= \
	setpropex.c system_properties.c system_properties_compat.c
LOCAL_MODULE := setpropex
LOCAL_CFLAGS += -std=c99 -I jni/inc
LOCAL_LDLIBS := -llog
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= \
	setpropex.c system_properties.c system_properties_compat.c
LOCAL_MODULE := setpropex-pie
LOCAL_CFLAGS += -std=c99 -I jni/inc
LOCAL_LDLIBS := -llog -pie -fPIE
include $(BUILD_EXECUTABLE)
