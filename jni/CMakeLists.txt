cmake_minimum_required(VERSION 2.8.10)

project(venus)

# refer to ${OPENCV_DIR}/OpenCVConfig.cmake
if(CMAKE_CL_64)
	message(STATUS "64 bit compiler detected.")
	set(OpenCV_ARCH x64)
	set(ARCH x86_64)
else()
	message(STATUS "32 bit compiler detected.")
	set(OpenCV_ARCH x86)
	set(ARCH x86)
endif()

if(MSVC)
	if(MSVC_VERSION EQUAL 1400)
		set(OpenCV_RUNTIME vc8)
	elseif(MSVC_VERSION EQUAL 1500)
		set(OpenCV_RUNTIME vc9)
	elseif(MSVC_VERSION EQUAL 1600)
		set(OpenCV_RUNTIME vc10)
	elseif(MSVC_VERSION EQUAL 1700)
		set(OpenCV_RUNTIME vc11)
	elseif(MSVC_VERSION EQUAL 1800)
		set(OpenCV_RUNTIME vc12)
	elseif(MSVC_VERSION EQUAL 1900)
		set(OpenCV_RUNTIME vc14)
	endif()
endif()

if(MSVC)
	set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} /MTd" CACHE STRING "Debug builds CMAKE CXX flags"   FORCE)
	SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_DEBUG} /MT"  CACHE STRING "Release builds CMAKE CXX flags" FORCE)
endif()

option(ENABLE_FAST_MATH "Enable fast math optimizations" OFF)
if(ENABLE_FAST_MATH)
	if(CMAKE_COMPILER_IS_GNUCXX)
		add_definitions(-ffast-math)
	endif()

	if(MSVC)
		add_definitions(/fp:fast)	
	endif()
elseif()
	if(CMAKE_COMPILER_IS_GNUCXX)
		add_definitions(-fno-fast-math)
	endif()
	
	if(MSVC)
		add_definitions(/fp:precise)	
	endif()
endif()

find_package(Java)

find_package(OpenMP)
if(OPENMP_FOUND)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

# if cmake cannot find OpenCV, comment this line and configure OPENCV_DIR above manually.
#find_package(OpenCV REQUIRED)

set(OPENCV_DIR "G:/opencv/build" CACHE STRING "" FORCE)
set(OpenCV_INCLUDE_DIR ${OPENCV_DIR}/include/ CACHE STRING "" FORCE)
set(OPENCV_LIBDIR ${OPENCV_DIR}/${OpenCV_ARCH}/${OpenCV_RUNTIME}/staticlib/ CACHE STRING "" FORCE)

set(VERSION_FILE ${OpenCV_INCLUDE_DIR}/opencv2/core/version.hpp)
if(NOT EXISTS ${VERSION_FILE})
	message(FATAL_ERROR "OpenCV version file not found")
endif()
file(STRINGS ${VERSION_FILE} OpenCV_VERSIONS_TMP REGEX "^#define CV_VERSION_[A-Z]+[ \t]+[0-9]+$")

string(REGEX REPLACE ".*#define CV_VERSION_MAJOR[ \t]+([0-9]+).*" "\\1" OpenCV_VERSION_MAJOR ${OpenCV_VERSIONS_TMP})
string(REGEX REPLACE ".*#define CV_VERSION_MINOR[ \t]+([0-9]+).*" "\\1" OpenCV_VERSION_MINOR ${OpenCV_VERSIONS_TMP})
string(REGEX REPLACE ".*#define CV_VERSION_REVISION[ \t]+([0-9]+).*" "\\1" CV_VERSION_REVISION ${OpenCV_VERSIONS_TMP})
set(OpenCV_VERSION ${OpenCV_VERSION_MAJOR}.${OpenCV_VERSION_MINOR}.${CV_VERSION_REVISION} CACHE STRING "" FORCE)
set(VERSION_CODE "${OpenCV_VERSION_MAJOR}${OpenCV_VERSION_MINOR}${CV_VERSION_REVISION}")
set(OpenCV_VERSION "${OpenCV_VERSION_MAJOR}.${OpenCV_VERSION_MINOR}.${CV_VERSION_REVISION}")

message(STATUS "OPENCV_DIR: ${OPENCV_DIR}")
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV_INCLUDE_DIR: ${OpenCV_INCLUDE_DIR}")
message(STATUS "OPENCV_LIBDIR: " ${OPENCV_LIBDIR})



# get_filename_component(STASM_HOME jni/stasm/ REALPATH)

file(GLOB EXAMPLE_HEADER  example/*.h)
file(GLOB EXAMPLE_SOURCE  example/*.cpp)

file(GLOB PLATFORM_HEADER platform/jni_bridge.h)#platform/*.h)
file(GLOB PLATFORM_SOURCE platform/jni_bridge.cpp)#platform/*.cpp)

file(GLOB_RECURSE STASM_HEADER stasm/*.h)
file(GLOB_RECURSE STASM_SOURCE stasm/*.cpp)

file(GLOB VENUS_HEADER venus/*.h)
file(GLOB VENUS_SOURCE venus/*.cpp)

source_group("Example" FILES ${EXAMPLE_HEADER})
source_group("Example" FILES ${EXAMPLE_SOURCE})

source_group("Platform Header" FILES ${PLATFORM_HEADER})
source_group("Platform Source" FILES ${PLATFORM_SOURCE})

source_group("Stasm Header" FILES ${STASM_HEADER})
source_group("Stasm Source" FILES ${STASM_SOURCE})
source_group("Venus Header" FILES ${VENUS_HEADER})
source_group("Venus Source" FILES ${VENUS_SOURCE})

#file(GLOB PLATFORM_SOURCE PLATFORM/*.cpp)

if(NOT MSVC)
	add_definitions(-std=c++11)
endif()

include_directories(${OpenCV_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR})

# Java's header file is not hierarchical
if(Java_FOUND)
	include_directories($ENV{JAVA_HOME}/include/)
	if(WIN32)  # damn, why not #include "win32/jni_md.h"
		include_directories($ENV{JAVA_HOME}/include/win32/)
	endif()
endif()

# Android NDK https://developer.android.com/ndk/guides/application_mk.html
find_path(NDK_HOME
	names ndk-build ndk-build.cmd
	hints $ENV{ANDROID_NDK} $ENV{ANDROID_ROOT} $ENV{NDK_ROOT} $ENV{NDK_HOME}
	NO_DEFAULT_PATH
)
message(STATUS "NDK_HOME: ${NDK_HOME}")
if(false)#(NDK_HOME)  # file cdefs.h conflicts fatal error C1189: #error :  "No function renaming possible"
#	if(CMAKE_HOST_WIN32)
		set(APP_ABI x86 CACHE STRING "" FORCE) # x86_64
		set(APP_PLATFORM android-14 CACHE STRING "" FORCE)
#	endif()

	set(ANDROID_INCLUDE "${NDK_HOME}/platforms/${APP_PLATFORM}/arch-${ARCH}/usr/include/")
	if(EXISTS ${ANDROID_INCLUDE})
		message(STATUS "add Android include dir ${ANDROID_INCLUDE}")
		include_directories(${ANDROID_INCLUDE})
	else()
		message(WARNING "missing Android include dir ${ANDROID_INCLUDE}")
	endif()
endif()

# https://cmake.org/cmake/help/v3.0/command/link_directories.html
# Note that link_directories command is rarely necessary.
# The LINK_DIRECTORIES has to be before the ADD_EXECUTABLE. http://public.kitware.com/pipermail/cmake/2006-March/008599.html
link_directories(${OPENCV_LIBDIR})

add_executable(${PROJECT_NAME}
	${EXAMPLE_HEADER} ${EXAMPLE_SOURCE}

	${PLATFORM_HEADER} ${PLATFORM_SOURCE}
	${STASM_HEADER} ${STASM_SOURCE}
	${VENUS_HEADER} ${VENUS_SOURCE}
)

target_link_libraries(${PROJECT_NAME}
	debug zlibd      optimized zlib
	debug IlmImfd    optimized IlmImf
	debug ippicvmt   optimized ippicvmt
	debug libjasperd optimized libjasper
	debug libjpegd   optimized libjpeg
	debug libpngd    optimized libpng
	debug libtiffd   optimized libtiff
	debug libwebpd   optimized libwebp
	
	debug opencv_core${VERSION_CODE}d      optimized opencv_core${VERSION_CODE}
	debug opencv_hal${VERSION_CODE}d       optimized opencv_hal${VERSION_CODE}
	debug opencv_highgui${VERSION_CODE}d   optimized opencv_highgui${VERSION_CODE}
	debug opencv_imgproc${VERSION_CODE}d   optimized opencv_imgproc${VERSION_CODE}
	debug opencv_objdetect${VERSION_CODE}d optimized opencv_objdetect${VERSION_CODE}
	debug opencv_photo${VERSION_CODE}d     optimized opencv_photo${VERSION_CODE}  # for seamlessClone
	debug opencv_imgcodecs${VERSION_CODE}d optimized opencv_imgcodecs${VERSION_CODE}
)

