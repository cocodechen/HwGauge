# HwGauge/CMakeLists.txt: CMake configuration for HwGauge with NVML support
cmake_minimum_required(VERSION 3.25)

# Collect source files
file(GLOB_RECURSE HWGAUGE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)

file(GLOB_RECURSE HWGAUGE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hxx"
)

# Create executable target
add_executable(HwGauge ${HWGAUGE_SOURCES} ${HWGAUGE_HEADERS})

# Set target properties
set_target_properties(HwGauge PROPERTIES
    C_STANDARD 17
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    OUTPUT_NAME "hwgauge"
)

# Include directories
target_include_directories(HwGauge PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add NVML library
if(HWGAUGE_USE_NVML)
    find_package(CUDAToolkit REQUIRED)
    target_link_libraries(HwGauge PRIVATE CUDA::nvml)
    target_compile_definitions(HwGauge PRIVATE HWGAUGE_USE_NVML=1)
endif()

# Add Intel PCM libraryc
if(HWGAUGE_USE_INTEL_PCM)
    target_link_libraries(HwGauge PRIVATE PCM_STATIC)
    target_include_directories(HwGauge PRIVATE ../vendors/pcm/src)
    target_compile_definitions(HwGauge PRIVATE HWGAUGE_USE_INTEL_PCM=1)

endif()

# Add Ascend NPU
if(HWGAUGE_USE_NPU)
    # 查找昇腾相关路径
    find_path(ASCEND_INCLUDE_DIR
        NAMES dcmi_interface_api.h
        PATHS
            ${CMAKE_PREFIX_PATH}/include
            /usr/local/Ascend/driver/include
            /usr/local/Ascend/include
            /opt/Ascend/driver/include
            /usr/include
        DOC "Path to Ascend NPU header files"
    )
    
    # 查找多个可能的库文件位置
    find_library(DCMI_LIB
        NAMES dcmi
        PATHS
            ${CMAKE_PREFIX_PATH}/lib64/driver
            ${CMAKE_PREFIX_PATH}/lib64
            /usr/local/Ascend/driver/lib64/driver
            /usr/local/Ascend/lib64
            /opt/Ascend/driver/lib64/driver
            /usr/lib64
        DOC "Path to Ascend DCMI library"
    )
    
    find_library(ASCEND_HAL_LIB
        NAMES ascend_hal
        PATHS
            ${CMAKE_PREFIX_PATH}/lib64/driver
            ${CMAKE_PREFIX_PATH}/lib64
            /usr/local/Ascend/driver/lib64/driver
            /usr/local/Ascend/lib64
            /opt/Ascend/driver/lib64/driver
            /usr/lib64
        DOC "Path to Ascend HAL library"
    )
    
    # 检查是否找到所有必要组件
    if(ASCEND_INCLUDE_DIR AND DCMI_LIB AND ASCEND_HAL_LIB)
        target_include_directories(HwGauge PUBLIC ${ASCEND_INCLUDE_DIR})
        target_link_libraries(HwGauge PRIVATE ${DCMI_LIB} ${ASCEND_HAL_LIB})
        target_compile_definitions(HwGauge PRIVATE HWGAUGE_USE_NPU=1)
        
        message(STATUS "Ascend NPU support enabled")
        message(STATUS "  Include dir: ${ASCEND_INCLUDE_DIR}")
        message(STATUS "  Libraries: ${DCMI_LIB}, ${ASCEND_HAL_LIB}")
    else()
        message(WARNING "Ascend NPU support requested but required components not found")
        message(WARNING "  ASCEND_INCLUDE_DIR: ${ASCEND_INCLUDE_DIR}")
        message(WARNING "  DCMI_LIB: ${DCMI_LIB}")
        message(WARNING "  ASCEND_HAL_LIB: ${ASCEND_HAL_LIB}")
        message(WARNING "Please check if Ascend driver is installed and set CMAKE_PREFIX_PATH if needed")
        set(HWGAUGE_USE_NPU OFF CACHE BOOL "Enable collectors for NPU (Ascend)" FORCE)
    endif()
endif()

# Add PostgreSQL library
# find_package(PostgreSQL REQUIRED)
# target_include_directories(HwGauge PRIVATE ${PostgreSQL_INCLUDE_DIRS})
# target_link_libraries(HwGauge PRIVATE ${PostgreSQL_LIBRARIES})
# target_compile_definitions(HwGauge PRIVATE HWGAUGE_USE_POSTGRESQL=1)
# message(STATUS "PostgreSQL support enabled")

# Add PostgreSQL library (conditional)
if(HWGAUGE_USE_POSTGRESQL)
    # Windows / Linux 都用 Module 模式
    find_package(PostgreSQL REQUIRED)
    target_include_directories(HwGauge PRIVATE
        ${PostgreSQL_INCLUDE_DIRS}
    )
    target_link_libraries(HwGauge PRIVATE
        ${PostgreSQL_LIBRARIES}
    )
    if(WIN32)
        target_link_libraries(HwGauge PRIVATE
            ws2_32
            secur32
        )
    endif()
    target_compile_definitions(HwGauge PRIVATE HWGAUGE_USE_POSTGRESQL=1)

    message(STATUS "PostgreSQL support enabled")
    message(STATUS "  Include dir: ${PostgreSQL_INCLUDE_DIRS}")
    message(STATUS "  Libraries: ${PostgreSQL_LIBRARIES}")

endif()


# Add spdlog library
target_link_libraries(HwGauge PRIVATE spdlog::spdlog)

# Add CLI11 library
target_link_libraries(HwGauge PRIVATE CLI11::CLI11)

# Add prometheus-cpp library
target_link_libraries(HwGauge PRIVATE prometheus-cpp::pull)