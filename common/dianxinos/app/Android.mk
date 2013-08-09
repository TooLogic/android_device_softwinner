
LOCAL_PATH :=$(call my-dir)

#----------------------------------------------------------------
#		DianXinOS apps
#----------------------------------------------------------------
all-dianxinos-apks := $(patsubst $(LOCAL_PATH)/%,%,$(wildcard $(LOCAL_PATH)/*.apk))
$(info All DianXinOS APK Files: $(all-dianxinos-apks))


$(foreach m, $(all-dianxinos-apks), \
	$(eval include $(CLEAR_VARS)) \
	$(eval m := $(strip $(notdir $(m)))) \
	$(eval LOCAL_MODULE := $(patsubst %.apk,%,$(m))) \
	$(eval LOCAL_MODULE_TAGS := optional) \
	$(eval LOCAL_MODULE_CLASS := APPS) \
	$(eval LOCAL_MODULE_SUFFIX := $(COMMON_ANDROID_PACKAGE_SUFFIX)) \
	$(eval LOCAL_CERTIFICATE := PRESIGNED) \
	$(eval LOCAL_SRC_FILES := $(m)) \
	$(eval $(ALL_PREBUILT):$(LOCAL_MODULE)) \
	$(eval include $(BUILD_PREBUILT)) \
	$(eval m := ) \
)
all-dianxinos-apks :=

