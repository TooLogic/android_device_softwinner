# crane aino product config

$(call inherit-product, device/softwinner/crane-common/ProductCommon.mk)

DEVICE_PACKAGE_OVERLAYS := device/softwinner/crane-3g/overlay

PRODUCT_COPY_FILES += \
	device/softwinner/crane-3g/modules/modules/nand.ko:root/nand.ko

PRODUCT_COPY_FILES += \
	device/softwinner/crane-3g/kernel:kernel \
	device/softwinner/crane-3g/recovery.fstab:recovery.fstab

PRODUCT_COPY_FILES += \
	device/softwinner/crane-3g/ueventd.sun4i.rc:root/ueventd.sun4i.rc \
	device/softwinner/crane-3g/init.sun4i.rc:root/init.sun4i.rc \
	device/softwinner/crane-3g/init.sun4i.usb.rc:root/init.sun4i.usb.rc \
	device/softwinner/crane-3g/media_profiles.xml:system/etc/media_profiles.xml \
	device/softwinner/crane-3g/camera.cfg:system/etc/camera.cfg \
	frameworks/base/data/etc/android.hardware.camera.xml:system/etc/permissions/android.hardware.camera.xml \
	device/softwinner/common/hardware/realtek/bluetooth/rtl8723as.bin:system/vendor/modules/rtl8723as.bin \
	device/softwinner/common/hardware/realtek/bluetooth/bt_conf_rtk8723.c:system/vendor/modules/bt_conf_rtk8723.c

#input device config
PRODUCT_COPY_FILES += \
	device/softwinner/crane-3g/sun4i-keyboard.kl:system/usr/keylayout/sun4i-keyboard.kl \
	device/softwinner/crane-3g/ft5x_ts.kl:system/usr/keylayout/ft5x_ts.kl \
	device/softwinner/crane-3g/ft5x_ts.idc:system/usr/idc/ft5x_ts.idc

PRODUCT_COPY_FILES += \
	device/softwinner/crane-3g/initlogo.rle:root/initlogo.rle

PRODUCT_COPY_FILES += \
	device/softwinner/crane-3g/vold.fstab:system/etc/vold.fstab

# 3g ril
PRODUCT_COPY_FILES += \
    device/softwinner/crane-3g/apns-conf.xml:system/etc/apns-conf.xml \
    device/softwinner/crane-3g/ril/huawei/libsoftwinner-ril-mu509.so:system/lib/libsoftwinner-ril-mu509.so \
    device/softwinner/crane-3g/ril/yuga/libsoftwinner-ril-yuga.so:system/lib/libsoftwinner-ril-yuga.so \
    device/softwinner/crane-3g/ril/yuga/ip-down-ppp0:system/etc/ppp/ip-down-ppp0 \
	device/softwinner/crane-3g/ril/yuga/ip-up-ppp0:system/etc/ppp/ip-up-ppp0 \
	device/softwinner/crane-3g/ril/yuga/init.gprs-pppd:system/etc/init.gprs-pppd

# googleservice
# PRODUCT_COPY_FILES += \
#    $(call find-copy-subdir-files,*.apk,$(LOCAL_PATH)/googleservice,system/app)
#PRODUCT_PACKAGES += \
#	TSCalibration2

PRODUCT_PROPERTY_OVERRIDES += \
	persist.sys.usb.config=mass_storage,adb \
	ro.property.tabletUI=true \
	ro.udisk.lable=crane-evb \
	ro.product.firmware=1.4rc2 \
	rild.libpath=libsoftwinner-ril-yuga.so \
	ro.ril.ecclist=110,119,120,112,114,911

$(call inherit-product-if-exists, device/softwinner/crane-3g/modules/modules.mk)

PRODUCT_CHARACTERISTICS := tablet

# Overrides
PRODUCT_BRAND  := softwinners
PRODUCT_NAME   := crane_3g
PRODUCT_DEVICE := crane-3g
PRODUCT_MODEL  := Softwiner3g

