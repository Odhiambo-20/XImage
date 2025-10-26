# CMakeLists.txt for HubxSDK Core Components
# FXImage 2.1.0 Project
# Builds: xfactory, XAdaptor, XControl, xlibdll_proxy

cmake_minimum_required(VERSION 3.10)
project(HubxSDK VERSION 2.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")

# ============================================================================
# Compiler flags
# ============================================================================

if(MSVC)
    # Visual Studio flags
    add_compile_options(/W4)
    add_compile_options(/MP) # Multi-processor compilation
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(/O2 /Ob2)
    endif()
else()
    # GCC/Clang flags
    add_compile_options(-Wall -Wextra -pedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3)
    endif()
endif()

# ============================================================================
# Include directories
# ============================================================================

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/xlibdll_wrapper
    ${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/xlibdll
)

# ============================================================================
# Source files
# ============================================================================

# Core source files
set(CORE_SOURCES
    src/core/xfactory.cpp
    src/core/XAdaptor.cpp
    src/core/XControl.cpp
    src/core/XDetector.cpp
    src/core/XGrabber.cpp
    src/core/XFrame.cpp
    src/core/XImage.cpp
    src/core/XFile.cpp
)

# xlibdll wrapper
set(WRAPPER_SOURCES
    src/xlibdll_wrapper/xlibdll_proxy.cpp
)

# Utilities
set(UTIL_SOURCES
    src/utils/network_utils.cpp
    src/utils/error_handler.cpp
    src/utils/command_parser.cpp
)

# Correction algorithms (to be implemented)
set(CORRECTION_SOURCES
    src/correction/background_correction.cpp
    src/correction/gain_correction.cpp
    src/correction/baseline_correction.cpp
    src/correction/pdc_correction.cpp
    src/correction/dual_energy_fusion.cpp
    src/correction/xog_correct.cpp
    src/correction/xmg_correct.cpp
    src/correction/xmog_correct.cpp
)

# All sources
set(ALL_SOURCES
    ${CORE_SOURCES}
    ${WRAPPER_SOURCES}
    ${UTIL_SOURCES}
)

# Header files
file(GLOB_RECURSE HEADER_FILES include/*.h)

# ============================================================================
# Library target - hubx shared library
# ============================================================================

add_library(hubx SHARED ${ALL_SOURCES} ${HEADER_FILES})

# Set library properties
set_target_properties(hubx PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 2
    PUBLIC_HEADER "${HEADER_FILES}"
)

# Platform-specific settings
if(WIN32)
    set_target_properties(hubx PROPERTIES
        OUTPUT_NAME "hubx"
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )
    
    # Link Windows network libraries
    target_link_libraries(hubx PRIVATE ws2_32)
    
    # Copy DLL to bin directory after build
    add_custom_command(TARGET hubx POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:hubx>
        ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/hubx.dll
    )
else()
    set_target_properties(hubx PROPERTIES
        OUTPUT_NAME "hubx"
    )
    
    # Link pthread for threading support
    target_link_libraries(hubx PRIVATE pthread dl)
endif()

# Export symbols for DLL
if(WIN32)
    target_compile_definitions(hubx PRIVATE HUBX_EXPORTS)
endif()

# ============================================================================
# Installation
# ============================================================================

install(TARGETS hubx
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/hubx
)

# Install xlibdll reference (not the actual file, just documentation)
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/xlibdll/README.md
    DESTINATION lib/xlibdll
)

# ============================================================================
# Example executables (optional)
# ============================================================================

option(BUILD_EXAMPLES "Build example programs" ON)

if(BUILD_EXAMPLES)
    # Core usage example
    add_executable(core_example
        demo/cpp_demo/core_usage_example.cpp
    )
    target_link_libraries(core_example PRIVATE hubx)
    
    # Simple acquisition example
    add_executable(simple_acquisition
        demo/cpp_demo/simple_acquisition.cpp
    )
    target_link_libraries(simple_acquisition PRIVATE hubx)
    
    # Calibration demo
    add_executable(calibration_demo
        demo/cpp_demo/calibration_demo.cpp
    )
    target_link_libraries(calibration_demo PRIVATE hubx)
    
    # Install examples
    install(TARGETS core_example simple_acquisition calibration_demo
        RUNTIME DESTINATION bin/examples
    )
endif()

# ============================================================================
# Testing
# ============================================================================

option(BUILD_TESTS "Build test suite" ON)

if(BUILD_TESTS)
    enable_testing()
    
    # Unit tests
    add_executable(test_xfactory
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/unit_tests/test_xfactory.cpp
    )
    target_link_libraries(test_xfactory PRIVATE hubx)
    add_test(NAME XFactoryTest COMMAND test_xfactory)
    
    add_executable(test_xadaptor
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/unit_tests/test_xadaptor.cpp
    )
    target_link_libraries(test_xadaptor PRIVATE hubx)
    add_test(NAME XAdaptorTest COMMAND test_xadaptor)
    
    add_executable(test_xcontrol
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/unit_tests/test_xcontrol.cpp
    )
    target_link_libraries(test_xcontrol PRIVATE hubx)
    add_test(NAME XControlTest COMMAND test_xcontrol)
endif()

# ============================================================================
# Documentation
# ============================================================================

option(BUILD_DOCS "Build documentation" OFF)

if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        
        add_custom_target(docs ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
    else()
        message(WARNING "Doxygen not found, cannot build documentation")
    endif()
endif()

# ============================================================================
# Package configuration
# ============================================================================

include(CMakePackageConfigHelpers)

# Create package config file
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/HubxSDKConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/HubxSDKConfig.cmake
    INSTALL_DESTINATION lib/cmake/HubxSDK
)

# Create package version file
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/HubxSDKConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install package files
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/HubxSDKConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/HubxSDKConfigVersion.cmake
    DESTINATION lib/cmake/HubxSDK
)

# ============================================================================
# Summary
# ============================================================================

message(STATUS "")
message(STATUS "========================================")
message(STATUS "HubxSDK ${PROJECT_VERSION} Configuration")
message(STATUS "========================================")
message(STATUS "Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "Install prefix:    ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Build examples:    ${BUILD_EXAMPLES}")
message(STATUS "Build tests:       ${BUILD_TESTS}")
message(STATUS "Build docs:        ${BUILD_DOCS}")
message(STATUS "C++ compiler:      ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "C++ standard:      C++${CMAKE_CXX_STANDARD}")
message(STATUS "")
message(STATUS "Components:")
message(STATUS "  - xfactory       Resource management")
message(STATUS "  - XAdaptor       Network discovery")
message(STATUS "  - XControl       Command interface")
message(STATUS "  - xlibdll_proxy  DLL encapsulation")
message(STATUS "========================================")
message(STATUS "")

# ============================================================================
# Platform-specific notes
# ============================================================================

if(WIN32)
    message(STATUS "Windows Build Notes:")
    message(STATUS "  - hubx.dll will be created in bin/")
    message(STATUS "  - Make sure xlibdll.dll is in system PATH or same directory")
    message(STATUS "  - Visual Studio users: Use 'Multi-threaded DLL' runtime")
elseif(UNIX AND NOT APPLE)
    message(STATUS "Linux Build Notes:")
    message(STATUS "  - libhubx.so will be created in lib/")
    message(STATUS "  - Make sure xlibdll.so is in LD_LIBRARY_PATH")
    message(STATUS "  - Run: export LD_LIBRARY_PATH=./lib:$LD_LIBRARY_PATH")
endif()

message(STATUS "")
message(STATUS "Build commands:")
message(STATUS "  mkdir build && cd build")
message(STATUS "  cmake ..")
message(STATUS "  make -j$(nproc)")
message(STATUS "  sudo make install")
message(STATUS "")