LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)

libs := $(patsubst $(LOCAL_PATH)/%,%,$(wildcard $(LOCAL_PATH)/*.so))
libs += $(patsubst $(LOCAL_PATH)/%,%,$(wildcard $(LOCAL_PATH)/*.a))
$(foreach i,$(libs),\
    $(eval m := $(basename $(patsubst lib/%,%,$(i)))) \
    $(eval LOCAL_PREBUILT_LIBS += $(m):$(i)) \
    $(eval $(ALL_PREBUILT):$(m)) \
    $(eval m := ) \
)
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
