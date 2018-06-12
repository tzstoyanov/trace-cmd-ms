# Check if cmake has the required version
cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)

# Set the name and version of the project
project(kernelshark)

set(KS_VERSION_MAJOR 0)
set(KS_VERSION_MINOR 5)
set(KS_VERSION_PATCH 1)
set(KS_VERSION_STRING ${KS_VERSION_MAJOR}.${KS_VERSION_MINOR}.${KS_VERSION_PATCH})

message("\n project: Kernel Shark:  (version: ${KS_VERSION_STRING}) \n")

set(KS_DIR ${CMAKE_SOURCE_DIR})

# Make a directory to hold configuration files. To change this do:
# cmake .. -DKS_CONF_DIR=/your/preferred/path
set(KS_CONF_DIR "${KS_DIR}/.ksconf" CACHE STRING "Directory for configuration files.")
file(MAKE_DIRECTORY ${KS_CONF_DIR})

# Get the number of CPUs
include(ProcessorCount)
ProcessorCount(N_CPUS)

include(${KS_DIR}/build/FindTraceCmd.cmake)
include(${KS_DIR}/build/FindJSONC.cmake)

find_package(Qt5Widgets 5.7.1)
find_package(Qt5Network)
find_package(Qt5Concurrent)
if (Qt5Widgets_FOUND)
	message(STATUS "Found Qt5Widgets:  (version ${Qt5Widgets_VERSION})")
endif (Qt5Widgets_FOUND)

find_package(OpenGL)
find_package(GLUT)

find_package(PythonLibs)
include(${KS_DIR}/build/FindNumPy.cmake)

find_package(GTest)

set(LIBRARY_OUTPUT_PATH    "${KS_DIR}/lib")
set(EXECUTABLE_OUTPUT_PATH "${KS_DIR}/bin")

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -Wall -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -pthread")

include_directories(${KS_DIR}/src/
                    ${KS_DIR}/build/src/
                    ${TRACECMD_INCLUDE_DIR}
                    ${TRACEEVENT_INCLUDE_DIR}
                    ${JSONC_INCLUDE_DIR})

message("")
message(STATUS "C flags      : " ${CMAKE_C_FLAGS})
message(STATUS "CXX flags    : " ${CMAKE_CXX_FLAGS})
message(STATUS "Linker flags : " ${CMAKE_EXE_LINKER_FLAGS})

add_subdirectory(${KS_DIR}/src)

if (GTEST_FOUND)

#     add_subdirectory(${KS_DIR}/tests)

endif (GTEST_FOUND)

configure_file( ${KS_DIR}/build/ks.desktop.cmake
                ${KS_DIR}/kernelshark.desktop)

message("")
