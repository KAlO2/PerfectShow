LOCAL_PATH := $(call my-dir)
THIS_PATH  := $(LOCAL_PATH)
include $(CLEAR_VARS)

# http://docs.opencv.org/2.4/doc/tutorials/introduction/android_binary_package/dev_with_OCV_on_Android.html#application-development-with-static-initialization
OPENCV_CAMERA_MODULES   := off
OPENCV_INSTALL_MODULES  := off
OPENCV_LIB_TYPE         := STATIC

# change the paths below to meet your own project
OPENCV_ANDROID_SDK_HOME := G:/OpenCV-android-sdk

include $(OPENCV_ANDROID_SDK_HOME)/sdk/native/jni/OpenCV.mk
OPENCV_INCLUDE_DIR        := $(OPENCV_ANDROID_SDK_HOME)/sdk/native/jni/include
OPENCV_LIBS_DIR           := $(OPENCV_ANDROID_SDK_HOME)/sdk/native/libs/$(TARGET_ARCH_ABI)
OPENCV_3RD_PARTY_LIBS_DIR := $(OPENCV_ANDROID_SDK_HOME)/sdk/native/3rdparty/libs/$(TARGET_ARCH_ABI)



EXAMPLE_HEADER := $(wildcard example/*.h)
EXAMPLE_SOURCE := $(wildcard example/*.cpp)

PLATFORM_HEADER := $(wildcard platform/*.h)
PLATFORM_SOURCE := $(wildcard platform/*.cpp)

STASM_HEADER := $(wildcard stasm/*.h)   $(wildcard stasm/MOD_1/*.h)
STASM_SOURCE := $(wildcard stasm/*.cpp) $(wildcard stasm/MOD_1/*.cpp)

VENUS_HEADER := $(wildcard venus/*.h)
VENUS_SOURCE := $(wildcard venus/*.cpp)

LOCAL_MODULE    := venus
LOCAL_CPPFLAGS  := -fopenmp -DUSE_BGRA_LAYOUT=0
LOCAL_SRC_FILES := $(PLATFORM_SOURCE) $(STASM_SOURCE) $(VENUS_SOURCE)

LOCAL_LDFLAGS := -llog -ljnigraphics -L$(OPENCV_LIBS_DIR)

# -fopenmp flag will causes link error on *nix platforms: cannot find -lrt
# There is no separate libpthread, libresolv, or librt on Android, the functionality is all in libc.
ifeq ($(OS),Windows_NT)
    LOCAL_LDFLAGS += -fopenmp
else
    LOCAL_STATIC_LIBRARIES += -lgomp
endif

LOCAL_SHARED_LIBRARIES += opencv_java3_prebuilt
#LOCAL_WHOLE_STATIC_LIBRARIES +=
#LOCAL_STATIC_LIBRARIES += \
	-lopencv_core         \
	-lopencv_highgui      \
	-lopencv_objdetect    \
	-lopencv_imgproc      \
	-lopencv_imgcodecs    \
	-lopencv_photo
	
include $(BUILD_SHARED_LIBRARY)


LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE    := gpuimage
LOCAL_SRC_FILES += $(THIS_PATH)/platform/com_cloudream_ishow_gpuimage_GPUImageNativeLibrary.c
LOCAL_LDLIBS    += -llog
include $(BUILD_SHARED_LIBRARY)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
# <android-sdk>/docs/ndk/guides/prebuilts.html
LOCAL_MODULE    := opencv_java3_prebuilt
LOCAL_SRC_FILES := $(OPENCV_LIBS_DIR)/libopencv_java3.so
include $(PREBUILT_SHARED_LIBRARY)
