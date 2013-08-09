# crane sc3052 product config

$(call inherit-product, device/softwinner/crane-common/ProductCommon.mk)
#add overlay
DEVICE_PACKAGE_OVERLAYS := device/softwinner/crane-MID9742-sc3052/overlay

PRODUCT_COPY_FILES += \
	device/softwinner/crane-MID9742-sc3052/modules/modules/nand.ko:root/nand.ko

PRODUCT_PACKAGES += \
	sensors.exDroid

# updatelogic agent
PRODUCT_PACKAGES += \
	libutv-agent \
	uliSupervisor


PRODUCT_COPY_FILES += \
	device/softwinner/crane-MID9742-sc3052/kernel:kernel \
	device/softwinner/crane-MID9742-sc3052/recovery.fstab:recovery.fstab

PRODUCT_COPY_FILES += \
	device/softwinner/crane-MID9742-sc3052/ueventd.sun4i.rc:root/ueventd.sun4i.rc \
	device/softwinner/crane-MID9742-sc3052/init.sun4i.rc:root/init.sun4i.rc \
	device/softwinner/crane-MID9742-sc3052/init.sun4i.usb.rc:root/init.sun4i.usb.rc \
	device/softwinner/crane-MID9742-sc3052/media_profiles.xml:system/etc/media_profiles.xml \
	device/softwinner/crane-MID9742-sc3052/camera.cfg:system/etc/camera.cfg \
	frameworks/base/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
	frameworks/base/data/etc/android.hardware.camera.xml:system/etc/permissions/android.hardware.camera.xml \
	frameworks/base/data/etc/android.hardware.camera.autofocus.xml:system/etc/permissions/android.hardware.camera.autofocus.xml

#input device config
PRODUCT_COPY_FILES += \
	device/softwinner/crane-MID9742-sc3052/sun4i-keyboard.kl:system/usr/keylayout/sun4i-keyboard.kl \
	device/softwinner/crane-MID9742-sc3052/gt80x.idc:system/usr/idc/gt80x.idc

PRODUCT_COPY_FILES += \
	device/softwinner/crane-MID9742-sc3052/initlogo.rle:root/initlogo.rle

PRODUCT_COPY_FILES += \
	device/softwinner/crane-MID9742-sc3052/vold.fstab:system/etc/vold.fstab


PRODUCT_PROPERTY_OVERRIDES += \
	persist.sys.usb.config=mass_storage \
	ro.sf.hwrotation=180 \
 	persist.sys.language=en \
 	persist.sys.country=US \
	ro.ui.fullscreen=false \
	ro.product.firmware=4.0.3


$(call inherit-product-if-exists, device/softwinner/crane-MID9742-sc3052/modules/modules.mk)

PRODUCT_CHARACTERISTICS := tablet

# Overrides
PRODUCT_BRAND  := sc
PRODUCT_NAME   := crane_MID9742_sc3052
PRODUCT_DEVICE := crane-MID9742-sc3052
PRODUCT_MODEL  := MID9742

