# Find Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.11.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Get source files (excluding main.cpp)
file(GLOB_RECURSE SOURCES "../src/*.cpp")
list(FILTER SOURCES EXCLUDE REGEX "../src/main.cpp")

# Add definition to indicate we're in testing mode
add_definitions(-DELEVATOR_TESTING)

# Add the test executable
add_executable(elevator_tests
    test_controller.cpp
    test_elevator.cpp
    test_emergency.cpp
    ${SOURCES}
)

# Link against libraries
target_link_libraries(elevator_tests
    gtest
    gtest_main
    ${CMAKE_THREAD_LIBS_INIT}
)

# Add test to CTest
add_test(NAME elevator_tests COMMAND elevator_tests)

include(GoogleTest)
gtest_discover_tests(elevator_tests)

if(DEFINED ENV{CI})
  target_compile_definitions(elevator_tests PRIVATE CI=1)
endif()