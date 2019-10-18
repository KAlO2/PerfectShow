LOCAL_PATH := $(call my-dir)
THIS_PATH  := $(LOCAL_PATH)
include $(CLEAR_VARS)

# http://docs.opencv.org/2.4/doc/tutorials/introduction/android_binary_package/dev_with_OCV_on_Android.html#application-development-with-static-initialization
OPENCV_CAMERA_MODULES   := off
OPENCV_INSTALL_MODULES  := off
OPENCV_LIB_TYPE         := STATIC

# change the paths below to meet your own project
OPENCV_ANDROID_SDK_HOME := F:/OpenCV/OpenCV-3.2.0-android-sdk

include $(OPENCV_ANDROID_SDK_HOME)/sdk/native/jni/OpenCV.mk
OPENCV_INCLUDE_DIR        := $(OPENCV_ANDROID_SDK_HOME)/sdk/native/jni/include
OPENCV_LIBS_DIR           := $(OPENCV_ANDROID_SDK_HOME)/sdk/native/libs/$(TARGET_ARCH_ABI)
OPENCV_3RD_PARTY_LIBS_DIR := $(OPENCV_ANDROID_SDK_HOME)/sdk/native/3rdparty/libs/$(TARGET_ARCH_ABI)


# `$(realpath $(wildcard venus/*.cpp))` doesn't workd in Android Studio, use full path for now.
STASM_SOURCE := \
	$(THIS_PATH)/stasm/asm.cpp             \
	$(THIS_PATH)/stasm/classicdesc.cpp     \
	$(THIS_PATH)/stasm/convshape.cpp       \
	$(THIS_PATH)/stasm/err.cpp             \
	$(THIS_PATH)/stasm/eyedet.cpp          \
	$(THIS_PATH)/stasm/eyedist.cpp         \
	$(THIS_PATH)/stasm/faceroi.cpp         \
	$(THIS_PATH)/stasm/hat.cpp             \
	$(THIS_PATH)/stasm/hatdesc.cpp         \
	$(THIS_PATH)/stasm/landmarks.cpp       \
	$(THIS_PATH)/stasm/misc.cpp            \
	$(THIS_PATH)/stasm/pinstart.cpp        \
	$(THIS_PATH)/stasm/print.cpp           \
	$(THIS_PATH)/stasm/shape17.cpp         \
	$(THIS_PATH)/stasm/shapehacks.cpp      \
	$(THIS_PATH)/stasm/shapemod.cpp        \
	$(THIS_PATH)/stasm/startshape.cpp      \
	$(THIS_PATH)/stasm/stasm.cpp           \
	$(THIS_PATH)/stasm/stasm_lib.cpp       \
	$(THIS_PATH)/stasm/MOD_1/facedet.cpp   \
	$(THIS_PATH)/stasm/MOD_1/initasm.cpp   \

VENUS_SOURCE := \
	$(THIS_PATH)/venus/Beauty.cpp          \
	$(THIS_PATH)/venus/blend.cpp           \
	$(THIS_PATH)/venus/blur.cpp            \
	$(THIS_PATH)/venus/colorspace.cpp      \
	$(THIS_PATH)/venus/Effect.cpp          \
	$(THIS_PATH)/venus/Feature.cpp         \
	$(THIS_PATH)/venus/ImageWarp.cpp       \
	$(THIS_PATH)/venus/inpaint.cpp         \
	$(THIS_PATH)/venus/Makeup.cpp          \
	$(THIS_PATH)/venus/opencv_utility.cpp  \
	$(THIS_PATH)/venus/Region.cpp          \

PLATFORM_SOURCE := \
	$(THIS_PATH)/platform/android/Effect.cpp   \
	$(THIS_PATH)/platform/android/Feature.cpp  \
	$(THIS_PATH)/platform/android/Makeup.cpp   \
	$(THIS_PATH)/platform/jni_bridge.cpp       \


#RELATIVE_SOURCES := $(STASM_SOURCE) $(VENUS_SOURCE) $(PLATFORM_SOURCE)
#$(realpath $(RELATIVE_SOURCES))
#$(addprefix $(THIS_PATH)/, $(RELATIVE_SOURCES))

LOCAL_MODULE    := venus
LOCAL_CPPFLAGS  := -DUSE_BGRA_LAYOUT=0 -DUSE_INPAINTING=0
LOCAL_SRC_FILES += $(STASM_SOURCE)
LOCAL_SRC_FILES += $(VENUS_SOURCE) 
LOCAL_SRC_FILES += $(PLATFORM_SOURCE)

LOCAL_LDFLAGS := -llog -ljnigraphics -L$(OPENCV_LIBS_DIR)

# -fopenmp flag will causes link error on *nix platforms: cannot find -lrt
# There is no separate libpthread, libresolv, or librt on Android, the functionality is all in libc.
ifeq (0, 1)
ifeq ($(OS), Windows_NT)
	LOCAL_LDFLAGS += -fopenmp
else
	LOCAL_STATIC_LIBRARIES += -lgomp
endif
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
