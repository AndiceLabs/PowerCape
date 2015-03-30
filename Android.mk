#Android.mk
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS) 
LOCAL_C_INCLUDES := external/kernel-headers/original
#module name
LOCAL_MODULE    := ina219
#src
LOCAL_SRC_FILES := utils/ina219.c
#build executable
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES += external/kernel-headers/original 
#module name
LOCAL_MODULE    := powercape
#src
LOCAL_SRC_FILES := utils/powercape.c
#build executable
include $(BUILD_EXECUTABLE)
