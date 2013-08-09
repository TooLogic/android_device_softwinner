#
# DianXinOS Prebuilt package release directory
#

LOCAL_PATH := $(call my-dir)

DIANXINOS_RELEASE_TEMPLATE_DIR := $(LOCAL_PATH)

#
# DianXinOS SDK Jar package
#
# stored as $(LOCAL_PATH)/framework/dianxinos_classes.jar
#
DIANXINOS_JAR_FILE_NAME := dianxinos_classes.jar


# include sub dir makefile for ODM
#
ifneq ($(OSSUITE_BUILD), true)
$(info Include Dianxinos Files for ODM build)
include $(call all-subdir-makefiles)
else
-include vendor/dianxinos/core/ossuite-release.mk
endif

