#
# Copyright (C) YuqiaoZhang(HanetakaChou)
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

# https://developer.android.com/ndk/guides/android_mk

LOCAL_PATH := $(realpath $(call my-dir))

# Android Application Library

include $(CLEAR_VARS)

LOCAL_MODULE := demo-android

LOCAL_SRC_FILES := \
	$(LOCAL_PATH)/../assets/cube.cpp \
	$(LOCAL_PATH)/../assets/lunarg_ppm.cpp \
	$(LOCAL_PATH)/../code/support/camera_controller.cpp \
	$(LOCAL_PATH)/../code/support/main.cpp \
	$(LOCAL_PATH)/../code/support/renderer.cpp \
	$(LOCAL_PATH)/../code/support/streaming.cpp \
	$(LOCAL_PATH)/../code/support/utils_align_up.cpp \
	$(LOCAL_PATH)/../code/support/vma.cpp \
	$(LOCAL_PATH)/../code/demo.cpp

LOCAL_CFLAGS :=
LOCAL_CFLAGS += -fdiagnostics-format=msvc
LOCAL_CFLAGS += -finput-charset=UTF-8 -fexec-charset=UTF-8
LOCAL_CFLAGS += -Werror=return-type
LOCAL_CFLAGS += -fvisibility=hidden
LOCAL_CFLAGS += -Wall

ifeq (arm, $(TARGET_ARCH))
LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true
endif

LOCAL_CPPFLAGS :=
LOCAL_CPPFLAGS += -std=c++11

LOCAL_C_INCLUDES :=
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../thirdparty/DirectXMath/Inc

LOCAL_LDFLAGS :=
LOCAL_LDFLAGS += -finput-charset=UTF-8 -fexec-charset=UTF-8
# the "dynamic linker" can't recognize the old dtags
LOCAL_LDFLAGS += -Wl,--enable-new-dtags
# the "chrpath" can only make path shorter
# LOCAL_LDFLAGS += -Wl,-rpath,XORIGIN 
LOCAL_LDFLAGS += -Wl,--version-script,$(LOCAL_PATH)/libdemo-android.map

LOCAL_LDLIBS :=
LOCAL_LDLIBS += -landroid
LOCAL_LDLIBS += -lvulkan

LOCAL_SHARED_LIBRARIES :=
ifeq (true, $(APP_DEBUG))
	LOCAL_LDLIBS += -llog
	LOCAL_SHARED_LIBRARIES += VkLayer_khronos_validation
endif

include $(BUILD_SHARED_LIBRARY)

ifeq (true, $(APP_DEBUG))

# VkLayer_khronos_validation

include $(CLEAR_VARS)

LOCAL_MODULE := VkLayer_khronos_validation

ifeq (x86_64,$(TARGET_ARCH_ABI))
	LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/vulkansdk/lib/android_x64/libVkLayer_khronos_validation$(TARGET_SONAME_EXTENSION)
else ifeq (x86,$(TARGET_ARCH_ABI))
	LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/vulkansdk/lib/android_x86/libVkLayer_khronos_validation$(TARGET_SONAME_EXTENSION)
else ifeq (arm64-v8a,$(TARGET_ARCH_ABI))
	LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/vulkansdk/lib/android_arm64/libVkLayer_khronos_validation$(TARGET_SONAME_EXTENSION)
else ifeq (armeabi-v7a,$(TARGET_ARCH_ABI))
	LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/vulkansdk/lib/android_arm/libVkLayer_khronos_validation$(TARGET_SONAME_EXTENSION)
else
	LOCAL_SRC_FILES := $(LOCAL_PATH)/../thirdparty/vulkansdk/lib/android_unknown/libVkLayer_khronos_validation$(TARGET_SONAME_EXTENSION)
endif

include $(PREBUILT_SHARED_LIBRARY)

endif
