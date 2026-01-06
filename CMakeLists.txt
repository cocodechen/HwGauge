# CMakeLists.txt: Modern CMake configuration with C23/C++20 support
cmake_minimum_required(VERSION 3.25)

# Set policy for MSVC debug information format
if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# Components Options
option(HWGAUGE_USE_INTEL_PCM "Enable collectors for Intel CPUs" ON)
option(HWGAUGE_USE_NVML "Enable collectors for Nvidia GPUs" ON)
option(HWGAUGE_USE_NPU "Enable collectors for Ascend NPUs" ON)

# Project declaration
project(HwGauge
    VERSION 1.0.0
    LANGUAGES C CXX
    DESCRIPTION "Hardware Gauge Application"
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Set C and C++ standards globally
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Compiler-specific flags and optimizations
if(MSVC)
    # MSVC specific flags
    add_compile_options(
        /W4                          # Warning level 4
        /permissive-                 # Standards conformance
        /Zc:__cplusplus             # Enable updated __cplusplus macro
        /utf-8                       # Set source and execution character sets to UTF-8
    )
    
    # Debug configuration
    add_compile_options($<$<CONFIG:Debug>:/Od>)      # No optimization
    add_compile_options($<$<CONFIG:Debug>:/Zi>)      # Full debug info
    add_compile_options($<$<CONFIG:Debug>:/RTC1>)    # Runtime checks
    add_compile_options($<$<CONFIG:Debug>:/MDd>)     # Multi-threaded Debug DLL
    
    # Release configuration
    add_compile_options($<$<CONFIG:Release>:/O2>)    # Maximum optimization
    add_compile_options($<$<CONFIG:Release>:/GL>)    # Whole program optimization
    add_compile_options($<$<CONFIG:Release>:/MD>)    # Multi-threaded Release DLL
    add_compile_options($<$<CONFIG:Release>:/DNDEBUG>) # Disable assertions
    
    # Linker flags for Release
    add_link_options($<$<CONFIG:Release>:/LTCG>)     # Link-time code generation
    add_link_options($<$<CONFIG:Release>:/OPT:REF>)  # Remove unreferenced functions
    add_link_options($<$<CONFIG:Release>:/OPT:ICF>)  # Enable COMDAT folding
    
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # GCC and Clang common flags
    add_compile_options(
        -Wall                        # Enable most warnings
        -Wextra                      # Enable extra warnings
        -Wpedantic                   # Strict ISO C/C++ compliance
    )
    
    # Debug configuration
    add_compile_options($<$<CONFIG:Debug>:-O0>)      # No optimization
    add_compile_options($<$<CONFIG:Debug>:-g3>)      # Maximum debug info
    add_compile_options($<$<CONFIG:Debug>:-ggdb>)    # GDB-specific debug info
    add_compile_options($<$<CONFIG:Debug>:-fno-omit-frame-pointer>) # Keep frame pointers
    
    # Enable sanitizers in Debug mode (optional, can be toggled)
    option(ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers in Debug mode" OFF)
    if(ENABLE_SANITIZERS)
        add_compile_options($<$<CONFIG:Debug>:-fsanitize=address,undefined>)
        add_link_options($<$<CONFIG:Debug>:-fsanitize=address,undefined>)
    endif()
    
    # Release configuration
    add_compile_options($<$<CONFIG:Release>:-O3>)    # Maximum optimization
    add_compile_options($<$<CONFIG:Release>:-march=native>) # Optimize for current CPU
    add_compile_options($<$<CONFIG:Release>:-DNDEBUG>) # Disable assertions
    add_compile_options($<$<CONFIG:Release>:-flto>)  # Link-time optimization
    add_link_options($<$<CONFIG:Release>:-flto>)
    
    # Additional Clang-specific flags
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-Wno-unused-command-line-argument)
    endif()
    
    # Additional GCC-specific flags
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options($<$<CONFIG:Release>:-fuse-linker-plugin>)
    endif()
endif()

# Platform-specific settings
if(WIN32)
    # Windows-specific settings
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX                     # Prevent Windows.h from defining min/max macros
        WIN32_LEAN_AND_MEAN         # Reduce Windows.h bloat
    )
elseif(UNIX AND NOT APPLE)
    # Linux-specific settings
    add_compile_options(-pthread)
    add_link_options(-pthread)
elseif(APPLE)
    # macOS-specific settings
    set(CMAKE_MACOSX_RPATH ON)
endif()

# Position Independent Code for shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Print build configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "C Standard: C${CMAKE_C_STANDARD}")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Architecture: ${CMAKE_SYSTEM_PROCESSOR}")

# Include sub-projects
add_subdirectory(HwGauge)
add_subdirectory(vendors/spdlog)
add_subdirectory(vendors/CLI11)

set(ENABLE_TESTING OFF CACHE BOOL "Disable prometheus-cpp testing" FORCE)
set(ENABLE_COMPRESSION OFF CACHE BOOL "Disable prometheus-cpp compression" FORCE)
set(ENABLE_PUSH OFF CACHE BOOL "Disable prometheus-cpp push" FORCE)
add_subdirectory(vendors/prometheus-cpp)

if(HWGAUGE_USE_INTEL_PCM)
    set(PCM_BUILD_EXECUTABLES OFF CACHE BOOL "Disable Intel PCM utilities" FORCE)
    add_subdirectory(vendors/pcm)
endif()