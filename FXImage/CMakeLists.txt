cmake_minimum_required(VERSION 3.10)
project(FXImage VERSION 2.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt5
find_package(Qt5 COMPONENTS Core Gui Widgets Network REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Include directories
include_directories(src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../HubxSDK/include)

# Source files
file(GLOB_RECURSE SOURCES
    src/*.cpp
    src/pages/*.cpp
    src/widgets/*.cpp
    src/controllers/*.cpp
    src/utils/*.cpp
)

file(GLOB_RECURSE HEADERS
    src/*.h
    src/pages/*.h
    src/widgets/*.h
    src/controllers/*.h
    src/utils/*.h
)

file(GLOB_RECURSE UI_FILES
    src/*.ui
)

# Create executable
add_executable(FXImage ${SOURCES} ${HEADERS} ${UI_FILES})

# Link libraries
target_link_libraries(FXImage
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
    hubx
)

# Install
install(TARGETS FXImage DESTINATION bin)
