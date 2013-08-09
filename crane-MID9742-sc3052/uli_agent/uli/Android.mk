LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

BASEMAKEDIR = .
PROJECT = generic
CEM = C09C92-0001
PLATFORM_OS = android
#VERSION = RELEASE
VERSION = DEBUG

#PLATFORM_LIVE_SUPPORT=1
#PLATFORM_LIVE_SUPPORT_VIDEO=1
THIRD_PARTY_DIR = vendor

#COMMAND_LINE_ONLY_TEST_AGENT=1

# development build by default
#PLATFORM_PRODUCTION_BUILD=1

#include external/uli/config.dev
include $(LOCAL_PATH)/config.dev

XDEFINES += TARGET_BUILD_VARIANT='"$(TARGET_BUILD_VARIANT)"'

XLIBS += -llog
XLIBS += -lz
XLIBS += -lc
XLIBS += -lm
    
XDEFINES += THREADS_DEBUG


ifdef COMMAND_LINE_ONLY_TEST_AGENT
    XDEFINES += COMMAND_LINE_ONLY_TEST_AGENT
    SOURCEFILES += test/test-agent-project.c
endif

ifdef PLATFORM_LIVE_SUPPORT
    #for gsoap
    XDEFINES += __GLIBC__
    #XDEFINES += HAVE_STDINT_H
    #for uuid
    XDEFINES += _UUID_STDINT_H
    XDEFINES += HAVE_GETDTABLESIZE
    XDEFINES += WITH_OPENSSL
    XDEFINES += WITH_ZLIB
endif

INCDIR += $(XINCDIR)
INCDIR += include/
#INCDIR += external/uli/inc/
INCDIR += $(LOCAL_PATH)/inc/
INCDIR += .

INCDIR += $(LOCAL_PATH)/
INCDIR += $(LOCAL_PATH)/inc/
INCDIR += $(LOCAL_PATH)/platform/
INCDIR += $(LOCAL_PATH)/platform/os/linux/
INCDIR += $(LOCAL_PATH)/project/
INCDIR += $(LOCAL_PATH)/cem/$(CEM)/
INCDIR += $(LOCAL_PATH)/platform/arch/32bit/

INCDIR += external/openssl/include/
INCDIR += frameworks/base/core/jni

CFLAGS += $(addprefix -D, $(XDEFINES)) $(addprefix -I, $(INCDIR))

SOURCEFILES += jni-interface.c


##UUID
ifdef PLATFORM_LIVE_SUPPORT
    SOURCEFILES += uuid/clear.c
    SOURCEFILES += uuid/compare.c
    SOURCEFILES += uuid/copy.c
    SOURCEFILES += uuid/gen_uuid.c
    SOURCEFILES += uuid/isnull.c
    SOURCEFILES += uuid/pack.c
    SOURCEFILES += uuid/parse.c
    SOURCEFILES += uuid/unpack.c
    SOURCEFILES += uuid/unparse.c
    SOURCEFILES += uuid/uuid_time.c
endif

LOCAL_STATIC_LIBRARIES += libutvprivate

ifdef COMMAND_LINE_ONLY_TEST_AGENT
    LOCAL_MODULE    := utv-agent
else
LOCAL_MODULE    := libutv-agent
endif


LOCAL_SRC_FILES += $(SOURCEFILES)
LOCAL_C_INCLUDES := $(INCDIR)
LOCAL_CFLAGS := $(CFLAGS)
LOCAL_CPPFLAGS:= $(CFLAGS)
LOCAL_LDLIBS := $(XLIBS)


LOCAL_SHARED_LIBRARIES += libssl
LOCAL_SHARED_LIBRARIES += libcrypto
LOCAL_SHARED_LIBRARIES += libcutils


LOCAL_PRELINK_MODULE := false

ifdef PLATFORM_LIVE_SUPPORT
    LOCAL_SHARED_LIBRARIES += libglib-2.0
    LOCAL_SHARED_LIBRARIES += libgmodule-2.0
    LOCAL_SHARED_LIBRARIES += libgobject-2.0

    LOCAL_SHARED_LIBRARIES += libgstbase-0.10
    LOCAL_SHARED_LIBRARIES += libgstcontroller-0.10
    LOCAL_SHARED_LIBRARIES += libgstdataprotocol-0.10
    LOCAL_SHARED_LIBRARIES += libgstnet-0.10
    LOCAL_SHARED_LIBRARIES += libgstreamer-0.10
    LOCAL_SHARED_LIBRARIES += libgthread-2.0

    LOCAL_SHARED_LIBRARIES += liboil

    LOCAL_SHARED_LIBRARIES += libgstapp-0.10
endif

ifdef COMMAND_LINE_ONLY_TEST_AGENT
    include $(BUILD_EXECUTABLE)
else
include $(BUILD_SHARED_LIBRARY)
endif

# comment out when creating private lib
include $(LOCAL_PATH)/private.mk
# uncomment out to create private lib
#include $(LOCAL_PATH)/privateIntern.mk
#include $(LOCAL_PATH)/secureStore.mk





