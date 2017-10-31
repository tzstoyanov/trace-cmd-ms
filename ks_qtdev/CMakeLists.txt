# Check if cmake has the required version
cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)

# Set the name and version of the project
project(kernelshark)
MESSAGE("\n project: Kernel Shark")
set(KS_VERSION_MAJOR 0)
set(KS_VERSION_MINOR 3)
set(KS_VERSION_PATCH 0)
set(KS_VERSION_STRING ${KS_VERSION_MAJOR}.${KS_VERSION_MINOR}.${KS_VERSION_PATCH})
MESSAGE(STATUS "version: ${KS_VERSION_STRING} \n")

set(KS_DIR ${CMAKE_SOURCE_DIR})

# Get the number of CPUs
include(ProcessorCount)
ProcessorCount(N_CPUS)

find_package(Qt5Widgets 5.9.1 REQUIRED)
MESSAGE(STATUS "Found Qt5Widgets:   (found version " ${Qt5Widgets_VERSION} ")")

find_package(Qt5Charts REQUIRED)
find_package(PythonLibs REQUIRED)
find_package(GTest)

include(${KS_DIR}/build/FindTraceCmd.cmake)

configure_file( ${KS_DIR}/build/deff.h.cmake
                ${KS_DIR}/src/KsDeff.h)

set(LIBRARY_OUTPUT_PATH    "${KS_DIR}/lib")
set(EXECUTABLE_OUTPUT_PATH "${KS_DIR}/bin")

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -Wall -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3 -std=c++11 -pthread")
#set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl")
#set(CMAKE_C_LINK_EXECUTABLE "${CMAKE_C_LINK_EXECUTABLE} -ldl")

include_directories( ${KS_DIR}/src/
                     ${KS_DIR}/build/src/
                     ${TRACECMD_INCLUDE_DIR})

message(STATUS "C flags      : " ${CMAKE_C_FLAGS})
message(STATUS "CXX flags    : " ${CMAKE_CXX_FLAGS})
message(STATUS "Linker flags : " ${CMAKE_EXE_LINKER_FLAGS}\n)

add_subdirectory(${KS_DIR}/src)

IF (GTEST_FOUND)
	add_subdirectory(${KS_DIR}/tests)
ENDIF (GTEST_FOUND)
