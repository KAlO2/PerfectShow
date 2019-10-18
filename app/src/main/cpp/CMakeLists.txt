cmake_minimum_required(VERSION 3.4.1)


# TODO: ${OPENCV_MODULE_PATH}/src/main/cpp/FindOpenCV.cmake
# find_package(OpenCV REQUIRED)
set(OPENCV_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../../OpenCV)
set(OPENCV_DIR         ${OPENCV_MODULE_PATH}/src/main     CACHE STRING "" FORCE)
set(OPENCV_INCLUDE_DIR ${OPENCV_DIR}/cpp/include          CACHE STRING "" FORCE)
set(OPENCV_LIBDIR      ${OPENCV_DIR}/lib/${ANDROID_ABI}   CACHE STRING "" FORCE)

set(VERSION_FILE ${OPENCV_INCLUDE_DIR}/opencv2/core/version.hpp)
if(NOT EXISTS ${VERSION_FILE})
	message(FATAL_ERROR "OpenCV version file not found in " ${OPENCV_INCLUDE_DIR}/opencv2/core/version.hpp)
endif()
file(STRINGS ${VERSION_FILE} OPENCV_VERSIONS_TMP REGEX "^#define CV_VERSION_[A-Z]+[ \t]+[0-9]+$")

string(REGEX REPLACE ".*#define CV_VERSION_MAJOR[ \t]+([0-9]+).*"    "\\1" OPENCV_VERSION_MAJOR ${OPENCV_VERSIONS_TMP})
string(REGEX REPLACE ".*#define CV_VERSION_MINOR[ \t]+([0-9]+).*"    "\\1" OPENCV_VERSION_MINOR ${OPENCV_VERSIONS_TMP})
string(REGEX REPLACE ".*#define CV_VERSION_REVISION[ \t]+([0-9]+).*" "\\1" CV_VERSION_REVISION  ${OPENCV_VERSIONS_TMP})

set(OPENCV_VERSION  ${OPENCV_VERSION_MAJOR}.${OPENCV_VERSION_MINOR}.${CV_VERSION_REVISION} CACHE STRING "" FORCE)
set(VERSION_CODE   "${OPENCV_VERSION_MAJOR}${OPENCV_VERSION_MINOR}${CV_VERSION_REVISION}")
set(OPENCV_VERSION "${OPENCV_VERSION_MAJOR}.${OPENCV_VERSION_MINOR}.${CV_VERSION_REVISION}")

message(STATUS "OPENCV_DIR: ${OPENCV_DIR}")
message(STATUS "OpenCV version: ${OPENCV_VERSION}")
message(STATUS "OpenCV_INCLUDE_DIR: ${OPENCV_INCLUDE_DIR}")
message(STATUS "OPENCV_LIBDIR: " ${OPENCV_LIBDIR})

add_library(opencv_java3
		SHARED
		IMPORTED)

set_target_properties(opencv_java3
		PROPERTIES IMPORTED_LOCATION
		${OPENCV_LIBDIR}/libopencv_java3.so)

set(opencv_libs opencv_core opencv_imgproc opencv_objdetect opencv_photo opencv_imgcodecs)
foreach(lib ${opencv_libs})
	add_library(${lib}
			STATIC
			IMPORTED)
			
	set_target_properties(${lib}
			PROPERTIES IMPORTED_LOCATION
			${OPENCV_LIBDIR}/lib${lib}.a)
endforeach()


find_package(OpenMP)
if(OPENMP_FOUND)
	set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   ${OpenMP_C_FLAGS}  ")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()


if(MSVC)  # Visual C++ compiler
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()  # GCC or CLANG compiler
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3")
endif()

set(PLATFORM "UNKNOWN")
# Android platform
if(DEFINED ANDROID_ABI)
	set(PLATFORM android)
endif()

# windows platform
if(WIN32)
	set(PLATFORM x86)
endif()

file(GLOB_RECURSE STASM_HEADER    stasm/*.h)
file(GLOB         VENUS_HEADER   venus/*.cpp)
file(GLOB         PLATFORM_HREADER platform/${PLATFORM}/*.h)

file(GLOB_RECURSE STASM_SOURCE    stasm/*.cpp)
file(GLOB         VENUS_SOURCE    venus/*.cpp)
file(GLOB         PLATFORM_SOURCE platform/${PLATFORM}/*.cpp)

set(USE_INPAINTING None CACHE STRING None FORCE)
set_property(CACHE USE_INPAINTING PROPERTY STRINGS None OpenCV Criminisi)
# None: disable inpainting algorithm
# OpenCV: OpenCV's inpainting algorithm
# Criminisi: Criminisi's inpainting algorithm

if(${USE_INPAINTING} STREQUAL None)
	add_definitions(-DUSE_INPAINTING=0)
elseif(${USE_INPAINTING} STREQUAL OpenCV)
	add_definitions(-DUSE_INPAINTING=1)
elseif(${USE_INPAINTING} STREQUAL Criminisi)
	add_definitions(-DUSE_INPAINTING=2)
endif()

if(NOT ${USE_INPAINTING} STREQUAL Criminisi)
	list(REMOVE_ITEM VENUS_HEADER venus/inpaint.h)
	list(REMOVE_ITEM VENUS_SOURCE venus/inpaint.cpp)
endif()

if(MSVC)
	source_group("Stasm Header" FILES ${STASM_HEADER})
	source_group("Stasm Source" FILES ${STASM_SOURCE})

	source_group("Platform Header" FILES ${PLATFORM_HEADER})
	source_group("Platform Source" FILES ${PLATFORM_SOURCE})

	source_group("Venus Header" FILES ${VENUS_HEADER})
	source_group("Venus Source" FILES ${VENUS_SOURCE})
endif()

set(PROJECT_SOURCE
		${STASM_SOURCE}
		${VENUS_SOURCE}
		${PLATFORM_SOURCE}
		)

include_directories(
	${OPENCV_INCLUDE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
)

if(WIN32)
	add_executable(${PROJECT_NAME} ${PROJECT_SOURCE})
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
#			debug opencv_flann${VERSION_CODE}d     optimized opencv_flann${VERSION_CODE}
			debug opencv_hal${VERSION_CODE}d       optimized opencv_hal${VERSION_CODE}
			debug opencv_highgui${VERSION_CODE}d   optimized opencv_highgui${VERSION_CODE}
			debug opencv_imgproc${VERSION_CODE}d   optimized opencv_imgproc${VERSION_CODE}
			debug opencv_objdetect${VERSION_CODE}d optimized opencv_objdetect${VERSION_CODE}
			debug opencv_photo${VERSION_CODE}d     optimized opencv_photo${VERSION_CODE}  # for seamlessClone
			debug opencv_imgcodecs${VERSION_CODE}d optimized opencv_imgcodecs${VERSION_CODE}
)
else()
	add_library(${PROJECT_NAME} SHARED ${PROJECT_SOURCE})
	target_link_libraries(${PROJECT_NAME}
			opencv_java3
#[[
			opencv_core
			opencv_imgproc
			opencv_objdetect
			opencv_photo
			opencv_imgcodecs
#]]
			log
			jnigraphics
			)
endif()


