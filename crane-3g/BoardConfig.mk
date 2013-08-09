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
TARGET_RECOVERY_UI_LIB := librecovery_ui_crane_3g
#TARGET_RECOVERY_UPDATER_LIBS :=

# Gsensor board config
# we can use this string : mma7660, mxc622x , bma250
#SW_BOARD_USES_GSENSOR_TYPE := mxc622x
#SW_BOARD_GSENSOR_DIRECT_X := false
#SW_BOARD_GSENSOR_DIRECT_Y := true
#SW_BOARD_GSENSOR_DIRECT_Z := false
#SW_BOARD_GSENSOR_XY_REVERT := true
# Wifi related defines
BOARD_WPA_SUPPLICANT_DRIVER := WEXT
WPA_SUPPLICANT_VERSION      := VER_0_8_X

# Wifi chipset select
# usb: realtek "rtl8192cu" wext-sta, ralink "rt5370";
# sdio: "nanowifi"/"ar6302"/"usibcm4329"
#SW_BOARD_USR_WIFI := nanowifi
#SW_BOARD_USR_WIFI := rt5370
#SW_BOARD_USR_WIFI := hwmw269v2
#SW_BOARD_USR_WIFI := hwmw269v3
#SW_BOARD_USR_WIFI := usibm01a
#SW_BOARD_USR_WIFI := bcm40181
#SW_BOARD_USR_WIFI := bcm40183
#SW_BOARD_USR_WIFI := ar6003
#SW_BOARD_USR_WIFI := ar6302

#BOARD_HAVE_BLUETOOTH := true
#BOARD_HAVE_BLUETOOTH_BCM := true
#BOARD_HAVE_BLUETOOTH_CSR:= true
#SW_BOARD_HAVE_BLUETOOTH_RTK:= true
#SW_BOARD_HAVE_BLUETOOTH_NAME := hwmw269v2
#SW_BOARD_HAVE_BLUETOOTH_NAME := usibm01a
#SW_BOARD_HAVE_BLUETOOTH_NAME := bcm40183

# realtek rtl8723as combo(wifi+bt) configuration
# set BOARD_HAVE_BLUETOOTH := true;
# set SW_BOARD_HAVE_BLUETOOTH_RTK:= true
# set BOARD_WIFI_VENDOR := realtek
# add bluetooth feature in the xml to display settings
# copy bt firmware rlt8723a_chip_b_cut_bt40_fw_asic_rom_patch.bin
# use nl80211 instead of wext in wpa_supplicant service@init.sun4i.rc
#BOARD_WIFI_VENDOR := realtek
ifeq ($(BOARD_WIFI_VENDOR), realtek)
    WPA_SUPPLICANT_VERSION := VER_0_8_X
    BOARD_WPA_SUPPLICANT_DRIVER := NL80211
    BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_rtl
    #BOARD_HOSTAPD_DRIVER        := NL80211
    #BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_rtl

    SW_BOARD_USR_WIFI := rtl8723as
    BOARD_WLAN_DEVICE := rtl8723as

endif

SW_BOARD_HAVE_3G := true

