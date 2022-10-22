cmake_minimum_required (VERSION 2.8)

project (d-config)

###############################
# Detect compiler and version #
###############################
if(NOT CMAKE_COMPILER_IS_GNUCXX)
    message(FATAL_ERROR "Wrong compiler used, required g++ 4.8 or greater")
endif()

set(COMPILER_NAME "gcc")
execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE COMPILER_VERSION)
string(REGEX REPLACE "(\r?\n)+$" "" COMPILER_VERSION "${COMPILER_VERSION}")

if(COMPILER_VERSION VERSION_LESS "4.8")
    message(FATAL_ERROR "Wrong g++ version, required g++ 4.8 or greater")
endif()

#######################
# Detect architecture #
#######################
set(ARCH "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}-${COMPILER_NAME}${COMPILER_VERSION}")
message(STATUS "Build Architecture:  " ${ARCH})
execute_process(COMMAND "cat" "/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size"
                OUTPUT_VARIABLE FOUND_CACHE_LINE_SIZE)
string(REGEX REPLACE "\n" "" HOOLA_CACHE_LINE_SIZE ${FOUND_CACHE_LINE_SIZE})
message(STATUS "Cache line size: ${HOOLA_CACHE_LINE_SIZE}")

######################
# Setup dependencies #
######################

find_package(Boost 1.48 REQUIRED program_options)

if(DEFINED ENV{GMOCK_ROOT})
    set(GMOCK_ROOT "$ENV{GMOCK_ROOT}")
else()
    set(GMOCK_ROOT "/usr")
endif()

message(STATUS "Using gmock root: ${GMOCK_ROOT}")

set(GMOCK_INCLUDE_DIRS "${GMOCK_ROOT}/include")
set(GMOCK_LIBRARY "${GMOCK_ROOT}/lib/x86_64-linux-gnu/libgmock.a")
set(GMOCK_LIBRARY "${GMOCK_ROOT}/lib/x86_64-linux-gnu/libgtest.a")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib/")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib/")

#########################
# Common compiler flags #
#########################
add_definitions(-Werror -Wall -Wextra -pedantic)
if (CMAKE_BUILD_TYPE MATCHES Release)
    add_definitions(-std=c++17 -O2)
elseif (CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-std=c++17 -g -O0 -fno-inline)
endif()
add_definitions(-DHOOLA_CACHE_LINE_SIZE=${HOOLA_CACHE_LINE_SIZE})

######################
# Additional targets #
######################

add_custom_target(distclean
    rm -rf bin lib CMakeCache.txt CMakeFiles cmake_install.cmake Makefile
)

###############
# Rock & Roll #
###############
include_directories(SYSTEM
    ${Boost_INCLUDE_DIRS}
    ${GMOCK_INCLUDE_DIRS}
)

include_directories(
    src
    test
)

file(GLOB_RECURSE SOURCES src/*.cpp)
file(GLOB_RECURSE UNITS test/unit/*.cpp)
file(GLOB_RECURSE PERFS test/perf/*.cpp)
file(GLOB TESTF test/*.cpp)

add_library(dconfig
   ${SOURCES}
)

add_executable(unit
    ${TESTF}
    ${UNITS}
)

target_link_libraries(unit
    dconfig
    ${Boost_LIBRARIES}
    ${GMOCK_LIBRARY}
    pthread
)

add_executable(perf
    ${TESTF}
    ${PERFS}
)

target_link_libraries(perf
    dconfig
    ${Boost_LIBRARIES}
    ${GMOCK_LIBRARY}
    pthread
)

