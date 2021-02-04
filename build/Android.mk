LOCAL_PATH := $(call my-dir)

AS_INCLUDE := ${LOCAL_PATH}/../inc/
AS_SOURCE  := ${LOCAL_PATH}/../src/

include $(CLEAR_VARS)
LOCAL_MODULE     := hello-jni
LOCAL_C_INCLUDES := $(AS_INCLUDE)
LOCAL_SRC_FILES  := ${AS_SOURCE}*.c  ${AS_SOURCE}*.cpp

include $(BUILD_SHARED_LIBRARY)
LOCAL_LDLIBS     +=  -ldl -lm -lpthread
