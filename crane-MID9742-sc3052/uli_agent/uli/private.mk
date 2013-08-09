LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := libutvprivate

$(warning LOCAL_PATH=$(LOCAL_PATH))
LOCAL_PREBUILT_LIBS += lib/android/$(VERSION)/libutvprivate.a

include $(BUILD_MULTI_PREBUILT)


