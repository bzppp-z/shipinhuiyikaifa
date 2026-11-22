cmake_minimum_required(VERSION 3.16)
project(meeting_cpp_unit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()

find_package(GTest REQUIRED)

add_executable(meeting_cpp_unit
    FrameCodecTest.cpp
    RoomManagerTest.cpp
    MediaRouterTest.cpp
    ../../Server/src/FrameCodec.cpp
    ../../Server/src/RoomManager.cpp
    ../../Server/src/MediaRouter.cpp
)

target_include_directories(meeting_cpp_unit PRIVATE ../../Server/include)
target_link_libraries(meeting_cpp_unit PRIVATE GTest::gtest_main pthread)

include(GoogleTest)
gtest_discover_tests(meeting_cpp_unit)
