# Check if cmake has the required version
cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)

# Set the name and version of the project
project(kernelshark)
MESSAGE("\n project: Kernelshark \n")

set(KS_VERSION_MAJOR 0)
set(KS_VERSION_MINOR 3)
set(KS_VERSION_PATCH 0)
set(KS_VERSION_STRING ${KS_VERSION_MAJOR}.${KS_VERSION_MINOR}.${KS_VERSION_PATCH})

set(KS_DIR ${CMAKE_SOURCE_DIR})

# Get the number of CPUs
include(ProcessorCount)
ProcessorCount(N_CPUS)
# MESSAGE("\n CPUs: ${N_CPUS} \n")

find_package (Qt5Widgets REQUIRED)
find_package(GTK2 REQUIRED)
find_package(PythonLibs REQUIRED)


include(${CMAKE_BINARY_DIR}/FindTraceCmd.cmake)

configure_file( ${CMAKE_BINARY_DIR}/deff.h.cmake
                ${CMAKE_SOURCE_DIR}/src/KsDeff.h)

set(LIBRARY_OUTPUT_PATH    "${CMAKE_SOURCE_DIR}/lib")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/bin")

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3 -std=c++11 -pthread")
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -Wall -O3")
set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl")

include_directories( ${CMAKE_SOURCE_DIR}/src/
                     ${CMAKE_BINARY_DIR}/src/
                     ${TRACECMD_INCLUDE_DIR}
                     ${GTK2_INCLUDE_DIRS})

message(STATUS "C flags      : " ${CMAKE_C_FLAGS})
message(STATUS "CXX flags    : " ${CMAKE_CXX_FLAGS})
message(STATUS "Linker flags : " ${CMAKE_EXE_LINKER_FLAGS}\n)

add_subdirectory(${CMAKE_SOURCE_DIR}/src)

