# BoardConfig.mk
#
# Product-specific compile-time definitions.
#

include device/softwinner/crane-common/BoardConfigCommon.mk


# image related
TARGET_NO_BOOTLOADER := true
TARGET_NO_RECOVERY := false
TARGET_NO_KERNEL := false

INSTALLED_KERNEL_TARGET := kernel
BOARD_KERNEL_BASE := 0x40000000
BOARD_KERNEL_CMDLINE := console=ttyS0,115200 rw init=/init loglevel=8
TARGET_USERIMAGES_USE_EXT4 := true
BOARD_FLASH_BLOCK_SIZE := 4096
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 536870912
#BOARD_USERDATAIMAGE_PARTITION_SIZE := 1073741824

# recovery stuff
TARGET_RECOVERY_PIXEL_FORMAT := "BGRA_8888"
TARGET_RECOVERY_UI_LIB := librecovery_ui_crane_m1003h6
#TARGET_RECOVERY_UPDATER_LIBS :=

# Gsensor board config
# we can use this string : mma7660, mxc622x, bma250
SW_BOARD_USES_GSENSOR_TYPE := mma7660
SW_BOARD_GSENSOR_DIRECT_X := false
SW_BOARD_GSENSOR_DIRECT_Y := true
SW_BOARD_GSENSOR_DIRECT_Z := true
SW_BOARD_GSENSOR_XY_REVERT := true

# Wifi related defines
#BOARD_WPA_SUPPLICANT_DRIVER := WEXT
#WPA_SUPPLICANT_VERSION      := VER_0_8_X

# Wifi chipset select
BOARD_WIFI_VENDOR := realtek
ifeq ($(BOARD_WIFI_VENDOR), realtek)
    WPA_SUPPLICANT_VERSION := VER_0_8_X
    BOARD_WPA_SUPPLICANT_DRIVER := NL80211
    BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_rtl
    BOARD_HOSTAPD_DRIVER        := NL80211
    BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_rtl

    SW_BOARD_USR_WIFI := rtl8192cu
    BOARD_WLAN_DEVICE := rtl8192cu
  
endif
