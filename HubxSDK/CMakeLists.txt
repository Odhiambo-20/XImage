cmake_minimum_required(VERSION 3.10)
project(HubxSDK VERSION 2.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
# find_package(Qt5 COMPONENTS Core Network REQUIRED)

# Include directories
include_directories(include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/xlibdll)

# Source files
file(GLOB_RECURSE SOURCES
    src/core/*.cpp
    src/correction/*.cpp
    src/xlibdll_wrapper/*.cpp
    src/utils/*.cpp
)

# Header files
file(GLOB_RECURSE HEADERS
    include/*.h
)

# Create shared library
add_library(hubx SHARED ${SOURCES} ${HEADERS})

# Platform-specific settings
if(WIN32)
    set_target_properties(hubx PROPERTIES OUTPUT_NAME "hubx")
else()
    set_target_properties(hubx PROPERTIES OUTPUT_NAME "hubx")
endif()

# Link libraries
# target_link_libraries(hubx Qt5::Core Qt5::Network)

# Install targets
install(TARGETS hubx
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include/hubx)
