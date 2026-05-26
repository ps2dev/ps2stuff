cmake_minimum_required(VERSION 3.13)

# Set the toolchain file before project()
if(NOT CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{PS2DEV})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{PS2DEV}/share/ps2dev.cmake" CACHE FILEPATH "Toolchain file")
    else()
        message(FATAL_ERROR "PS2DEV environment variable is not set. Please set it to your PS2DEV installation path.")
    endif()
endif()

project(ps2stuff VERSION 1.0.0 LANGUAGES CXX C)

# Options
option(BUILD_TESTS "Build test projects" OFF)
option(DEBUG "Enable debug build" OFF)
option(ENABLE_VU0_VECTORS "Enable VU0 vector code paths" ON)
option(ENABLE_ASM "Enable assembly optimizations" ON)

# Check if PS2SDK is set (should be done by toolchain file)
if(NOT DEFINED PS2SDK)
    message(FATAL_ERROR "PS2SDK is not defined. Make sure the toolchain file is loaded correctly.")
endif()

# Set output library name
set(EE_LIB "libps2stuff.a")

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${PS2SDK}/ports/include
)

# Link directories
link_directories(
    ${PS2SDK}/ports/lib
)

# Compiler flags
if(DEBUG)
    add_compile_definitions(_DEBUG)
endif()

# Warning flags (matching Makefile)
set(WARNING_FLAGS
    -Wno-strict-aliasing
    -Wno-conversion-null
)

if(NOT ENABLE_VU0_VECTORS)
    add_compile_definitions(NO_VU0_VECTORS)
endif()

if(NOT ENABLE_ASM)
    add_compile_definitions(NO_ASM)
endif()

add_compile_options(${WARNING_FLAGS})

# ============================================================================
# Source files
# ============================================================================
set(PS2STUFF_SOURCES
    src/core.cpp
    src/cpu_matrix.cpp
    src/displayenv.cpp
    src/drawenv.cpp
    src/eetimer.cpp
    src/gs.cpp
    src/gsmem.cpp
    src/imagepackets.cpp
    src/math.cpp
    src/matrix.cpp
    src/packet.cpp
    src/perfmon.cpp
    src/ps2stuff.cpp
    src/sprite.cpp
    src/texture.cpp
    src/timer.cpp
    src/utils.cpp
)

# ============================================================================
# Build the library
# ============================================================================
add_library(ps2stuff STATIC ${PS2STUFF_SOURCES})

set_target_properties(ps2stuff PROPERTIES
    OUTPUT_NAME "ps2stuff"
    ARCHIVE_OUTPUT_NAME "ps2stuff"
)

target_include_directories(ps2stuff PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# ============================================================================
# Install targets
# ============================================================================
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${EE_LIB}
    DESTINATION "${PS2SDK}/ports/lib"
)

install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/include/ps2s
    DESTINATION "${PS2SDK}/ports/include"
    FILES_MATCHING PATTERN "*.h"
)

# ============================================================================
# Include tests if enabled
# ============================================================================
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

# ============================================================================
# Print configuration summary
# ============================================================================
message(STATUS "")
message(STATUS "ps2stuff configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Debug build: ${DEBUG}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "  Output library: ${EE_LIB}")
message(STATUS "  Install prefix: ${PS2SDK}/ports")
message(STATUS "  VU0 vectors: ${ENABLE_VU0_VECTORS}")
message(STATUS "  ASM optimizations: ${ENABLE_ASM}")
message(STATUS "")
