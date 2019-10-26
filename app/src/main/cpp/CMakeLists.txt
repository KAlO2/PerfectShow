cmake_minimum_required(VERSION 3.4.1)

project(venus)

find_package(OpenCV 4.0 REQUIRED)

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
	${OpenCV_INCLUDE_DIRS}
	${CMAKE_CURRENT_SOURCE_DIR}
)

if(WIN32)
	add_executable(${PROJECT_NAME} ${PROJECT_SOURCE})
	target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBRARIES})
else()
	add_library(${PROJECT_NAME} SHARED ${PROJECT_SOURCE})
	target_link_libraries(${PROJECT_NAME}
			${OpenCV_LIBRARIES}
			c++_shared
			log
			jnigraphics
			)
endif()


