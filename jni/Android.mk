LOCAL_PATH:= $(call my-dir)

ifneq ($(TARGET_ARCH_ABI),arm64-v8a)
ifneq ($(TARGET_ARCH_ABI),x86_64)
ifneq ($(TARGET_ARCH_ABI),mips64)
include $(CLEAR_VARS)
LOCAL_SRC_FILES:= \
	setpropex.c system_properties.c system_properties_compat.c
LOCAL_MODULE := setpropex
LOCAL_CFLAGS += -std=c99 -I jni/inc
LOCAL_LDLIBS := -llog
include $(BUILD_EXECUTABLE)
endif
endif
endif

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= \
	setpropex.c system_properties.c system_properties_compat.c
LOCAL_MODULE := setpropex-pie
LOCAL_CFLAGS += -std=c99 -I jni/inc
LOCAL_LDLIBS := -llog -pie -fPIE
include $(BUILD_EXECUTABLE)
