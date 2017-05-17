cmake_minimum_required(VERSION 2.8.10)

project(venus)

# refere to ${OPENCV_DIR}/OpenCVConfig.cmake
if(CMAKE_CL_64)
	message(STATUS "64 bit compiler detected.")
	set(OpenCV_ARCH x64)
else()
	message(STATUS "32 bit compiler detected.")
	set(OpenCV_ARCH x86)
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
	set(CMAKE_CXX_FLAGS_DEBUG
	"${CMAKE_CXX_FLAGS_DEBUG} /MTd"
	CACHE STRING "Debug builds CMAKE CXX flags " FORCE)
endif()

file(GLOB HEADER_VENUS *.h)

SET(SOURCE_VENUS
	color_space.cpp
	operation.cpp
	jni_bridge.cpp
	com_cloudream_ishow_algorithm_Effect.cpp
)

include_directories(${VENUS_HOME}/)

add_executable(${PROJECT_NAME} ${HEADER_VENUS} ${SOURCE_VENUS})

target_link_libraries(${PROJECT_NAME}

)

