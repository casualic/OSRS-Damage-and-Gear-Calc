cmake_minimum_required(VERSION 3.10)
project(OSRSCalc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)
include_directories(external/cpp-httplib)
include_directories(external) 

# Find dependencies
find_package(CURL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)

# Source files
set(SOURCES
    src/main.cpp
    src/player.cpp
    src/monster.cpp
    src/item.cpp
    src/battle.cpp
    src/upgrade_advisor.cpp
)

# Executable
add_executable(osrscalc ${SOURCES})

# Link libraries
target_link_libraries(osrscalc PRIVATE CURL::libcurl Boost::system Boost::thread)

if(APPLE)
    # OpenSSL is often needed for Boost Beast / generic SSL on Mac
    find_package(OpenSSL REQUIRED)
    target_link_libraries(osrscalc PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()
