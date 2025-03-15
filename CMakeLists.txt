cmake_minimum_required(VERSION 3.10)
project(ElevatorControlSim VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Add main source files (excluding client file)
file(GLOB_RECURSE SOURCES "src/*.cpp")
list(FILTER SOURCES EXCLUDE REGEX "elevator_client.cpp$")

# Create main executable
add_executable(elevator_sim ${SOURCES})

# Create client executable
add_executable(elevator_client src/elevator_client.cpp)

# Link libraries
target_link_libraries(elevator_sim 
    ${CMAKE_THREAD_LIBS_INIT}
)

target_link_libraries(elevator_client
    ${CMAKE_THREAD_LIBS_INIT}
)

# Enable testing
enable_testing()
add_subdirectory(tests)

# Installation
install(TARGETS elevator_sim elevator_client DESTINATION bin)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ 
        DESTINATION include/elevator_sim
        FILES_MATCHING PATTERN "*.h")

# Documentation
option(BUILD_DOCS "Build documentation" OFF)
if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        add_subdirectory(docs)
    else()
        message(STATUS "Doxygen not found, documentation will not be built")
    endif()
endif()

# Find PostgreSQL
find_package(PostgreSQL REQUIRED)
include_directories(${PostgreSQL_INCLUDE_DIRS})

# Find libpqxx (PostgreSQL C++ client library)
find_path(PQXX_INCLUDE_DIR pqxx/pqxx
    PATHS
    /usr/local/include
    /usr/include
    /opt/homebrew/include
)

find_library(PQXX_LIBRARY pqxx
    PATHS
    /usr/local/lib
    /usr/lib
    /opt/homebrew/lib
)

if(NOT PQXX_INCLUDE_DIR OR NOT PQXX_LIBRARY)
    message(FATAL_ERROR "libpqxx not found. Please install libpqxx.")
endif()

include_directories(${PQXX_INCLUDE_DIR})

# Link libraries for main executable
target_link_libraries(elevator_sim 
    ${CMAKE_THREAD_LIBS_INIT}
    ${PostgreSQL_LIBRARIES}
    ${PQXX_LIBRARY}
)