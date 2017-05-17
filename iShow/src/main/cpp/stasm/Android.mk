LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# http://docs.opencv.org/2.4/doc/tutorials/introduction/android_binary_package/dev_with_OCV_on_Android.html#application-development-with-static-initialization
OPENCV_CAMERA_MODULES   := off
OPENCV_INSTALL_MODULES  := off
OPENCV_LIB_TYPE         := STATIC

# change the paths below to meet your own project
OPENCV_ANDROID_SDK_HOME := G:/OpenCV-android-sdk

include $(OPENCV_ANDROID_SDK_HOME)/sdk/native/jni/OpenCV.mk
OPENCV_LIBS_DIR := $(OPENCV_ANDROID_SDK_HOME)/sdk/native/libs/$(TARGET_ARCH_ABI)

# stasm project 
# Active Shape Models with Stasm
# http://www.milbo.users.sonic.net/stasm
LOCAL_MODULE    := stasm
LOCAL_SRC_FILES :=        \
	MOD_1/facedet.cpp     \
	MOD_1/initasm.cpp     \
	asm.cpp               \
	classicdesc.cpp       \
	convshape.cpp         \
	err.cpp               \
	eyedet.cpp            \
	eyedist.cpp           \
	faceroi.cpp           \
	hat.cpp               \
	hatdesc.cpp           \
	landmarks.cpp         \
	misc.cpp              \
	pinstart.cpp          \
	print.cpp             \
	shape17.cpp           \
	shapehacks.cpp        \
	shapemod.cpp          \
	startshape.cpp        \
	stasm.cpp             \
	stasm_lib.cpp         \

LOCAL_LDFLAGS := -fopenmp -llog -ljnigraphics -L$(OPENCV_LIBS_DIR)
LOCAL_SHARED_LIBRARIES += -lopencv_java3
#LOCAL_WHOLE_STATIC_LIBRARIES +=
#LOCAL_STATIC_LIBRARIES += \
	-lopencv_core         \
	-lopencv_highgui      \
	-lopencv_objdetect    \
	-lopencv_imgproc      \
	-lopencv_imgcodecs    \
	-lopencv_photo
	
include $(BUILD_SHARED_LIBRARY)