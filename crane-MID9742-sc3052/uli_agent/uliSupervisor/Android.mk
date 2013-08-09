LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-subdir-java-files)

LOCAL_PACKAGE_NAME          := uliSupervisor

LOCAL_CERTIFICATE           := platform

LOCAL_PROGUARD_ENABLED      := disabled

include $(BUILD_PACKAGE)
# ============================================================

# Also build all of the sub-targets under this one: the shared library.
include $(call all-makefiles-under,$(LOCAL_PATH))

# ============================================================

