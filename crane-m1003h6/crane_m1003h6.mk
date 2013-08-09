# crane m1003h6 product config

$(call inherit-product, device/softwinner/crane-common/ProductCommon.mk)

DEVICE_PACKAGE_OVERLAYS := device/softwinner/crane-m1003h6/overlay

PRODUCT_COPY_FILES += \
	device/softwinner/crane-m1003h6/modules/modules/nand.ko:root/nand.ko

PRODUCT_COPY_FILES += \
	device/softwinner/crane-m1003h6/kernel:kernel \
	device/softwinner/crane-m1003h6/recovery.fstab:recovery.fstab \
	frameworks/base/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml \
	frameworks/base/data/etc/android.hardware.camera.xml:system/etc/permissions/android.hardware.camera.xml \
	frameworks/base/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
	frameworks/base/data/etc/android.hardware.camera.autofocus.xml:system/etc/permissions/android.hardware.camera.autofocus.xml

PRODUCT_COPY_FILES += \
	device/softwinner/crane-m1003h6/ueventd.sun4i.rc:root/ueventd.sun4i.rc \
	device/softwinner/crane-m1003h6/init.sun4i.rc:root/init.sun4i.rc \
	device/softwinner/crane-m1003h6/init.sun4i.usb.rc:root/init.sun4i.usb.rc \
	device/softwinner/crane-m1003h6/media_profiles.xml:system/etc/media_profiles.xml \
	device/softwinner/crane-m1003h6/camera.cfg:system/etc/camera.cfg

#input device config
PRODUCT_COPY_FILES += \
	device/softwinner/crane-m1003h6/sun4i-keyboard.kl:system/usr/keylayout/sun4i-keyboard.kl \
	device/softwinner/crane-m1003h6/ft5x_ts.idc:system/usr/idc/ft5x_ts.idc

PRODUCT_COPY_FILES += \
	device/softwinner/crane-m1003h6/media/bootanimation.zip:system/media/bootanimation.zip \
	device/softwinner/crane-m1003h6/initlogo.rle:root/initlogo.rle


PRODUCT_COPY_FILES += \
	device/softwinner/crane-m1003h6/vold.fstab:system/etc/vold.fstab

# facelock
PRODUCT_COPY_FILES += \
	device/softwinner/common/facelock/app/FaceLock.apk:system/app/FaceLock.apk \
	device/softwinner/common/facelock/lib/libfacelock_jni.so:system/lib/libfacelock_jni.so \
	device/softwinner/common/facelock/vendor/pittpatt/models/recognition/face.face.y0-y0-22-b-N/full_model.bin:system/vendor/pittpatt/models/recognition/face.face.y0-y0-22-b-N/full_model.bin \
	device/softwinner/common/facelock/vendor/pittpatt/models/detection/yaw_roll_face_detectors.3/head-y0-yi45-p0-pi45-rp30-ri30.5/full_model.bin:system/vendor/pittpatt/models/detection/yaw_roll_face_detectors.3/head-y0-yi45-p0-pi45-rp30-ri30.5/full_model.bin \
	device/softwinner/common/facelock/vendor/pittpatt/models/detection/yaw_roll_face_detectors.3/head-y0-yi45-p0-pi45-rn30-ri30.5/full_model.bin:system/vendor/pittpatt/models/detection/yaw_roll_face_detectors.3/head-y0-yi45-p0-pi45-rn30-ri30.5/full_model.bin \
	device/softwinner/common/facelock/vendor/pittpatt/models/detection/yaw_roll_face_detectors.3/head-y0-yi45-p0-pi45-r0-ri30.4a/full_model.bin:system/vendor/pittpatt/models/detection/yaw_roll_face_detectors.3/head-y0-yi45-p0-pi45-r0-ri30.4a/full_model.bin \
	device/softwinner/common/facelock/vendor/pittpatt/models/detection/multi_pose_face_landmark_detectors.3/right_eye-y0-yi45-p0-pi45-rp7-ri20.2d_n2/full_model.bin:system/vendor/pittpatt/models/detection/multi_pose_face_landmark_detectors.3/right_eye-y0-yi45-p0-pi45-rp7-ri20.2d_n2/full_model.bin \
	device/softwinner/common/facelock/vendor/pittpatt/models/detection/multi_pose_face_landmark_detectors.3/right_eye-y0-yi45-p0-pi45-rn7-ri20.2d_n2/full_model.bin:system/vendor/pittpatt/models/detection/multi_pose_face_landmark_detectors.3/right_eye-y0-yi45-p0-pi45-rn7-ri20.2d_n2/full_model.bin \
	device/softwinner/common/facelock/vendor/pittpatt/models/detection/multi_pose_face_landmark_detectors.3/right_eye-y0-yi45-p0-pi45-r0-ri20.2d_n2/full_model.bin:system/vendor/pittpatt/models/detection/multi_pose_face_landmark_detectors.3/right_eye-y0-yi45-p0-pi45-r0-ri20.2d_n2/full_model.bin \
	device/softwinner/common/facelock/vendor/pittpatt/models/detection/multi_pose_face_landmark_detectors.3/nose_base-y0-yi45-p0-pi45-rp7-ri20.2d_n2/full_model.bin:system/vendor/pittpatt/models/detection/multi_pose_face_landmark_detectors.3/nose_base-y0-yi45-p0-pi45-rp7-ri20.2d_n2/full_model.bin \
	device/softwinner/common/facelock/vendor/pittpatt/models/detection/multi_pose_face_landmark_detectors.3/nose_base-y0-yi45-p0-pi45-rn7-ri20.2d_n2/full_model.bin:system/vendor/pittpatt/models/detection/multi_pose_face_landmark_detectors.3/nose_base-y0-yi45-p0-pi45-rn7-ri20.2d_n2/full_model.bin \
	device/softwinner/common/facelock/vendor/pittpatt/models/detection/multi_pose_face_landmark_detectors.3/nose_base-y0-yi45-p0-pi45-r0-ri20.2d_n2/full_model.bin:system/vendor/pittpatt/models/detection/multi_pose_face_landmark_detectors.3/nose_base-y0-yi45-p0-pi45-r0-ri20.2d_n2/full_model.bin  \
	device/softwinner/common/facelock/vendor/pittpatt/models/detection/multi_pose_face_landmark_detectors.3/left_eye-y0-yi45-p0-pi45-rp7-ri20.2d_n2/full_model.bin:system/vendor/pittpatt/models/detection/multi_pose_face_landmark_detectors.3/left_eye-y0-yi45-p0-pi45-rp7-ri20.2d_n2/full_model.bin  \
	device/softwinner/common/facelock/vendor/pittpatt/models/detection/multi_pose_face_landmark_detectors.3/left_eye-y0-yi45-p0-pi45-rn7-ri20.2d_n2/full_model.bin:system/vendor/pittpatt/models/detection/multi_pose_face_landmark_detectors.3/left_eye-y0-yi45-p0-pi45-rn7-ri20.2d_n2/full_model.bin  \
	device/softwinner/common/facelock/vendor/pittpatt/models/detection/multi_pose_face_landmark_detectors.3/left_eye-y0-yi45-p0-pi45-r0-ri20.2d_n2/full_model.bin:system/vendor/pittpatt/models/detection/multi_pose_face_landmark_detectors.3/left_eye-y0-yi45-p0-pi45-r0-ri20.2d_n2/full_model.bin

PRODUCT_PROPERTY_OVERRIDES += \
	persist.sys.usb.config=mass_storage,adb \
	persist.service.adb.enable=1 \
	ro.udisk.lable=ONDA_MID \
	ro.product.firmware=1.5rc5



$(call inherit-product-if-exists, device/softwinner/crane-m1003h6/modules/modules.mk)

PRODUCT_CHARACTERISTICS := tablet

# Overrides
PRODUCT_BRAND  := SoftWinnner
PRODUCT_NAME   := crane_m1003h6
PRODUCT_DEVICE := crane-m1003h6

PRODUCT_MODEL  := ONDA MID
