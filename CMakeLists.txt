cmake_minimum_required(VERSION 3.20)
project(Voxel3D LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Logic tests need no graphics library:  cmake -S . -B build-tests -DBUILD_TESTS=ON && cmake --build build-tests && ./build-tests/voxel_tests
option(BUILD_TESTS "Build only the logic tests" OFF)
if(BUILD_TESTS)
  add_executable(voxel_tests tests/test_logic.cpp)
  target_include_directories(voxel_tests PRIVATE ${CMAKE_SOURCE_DIR})
  enable_testing()
  add_test(NAME logic COMMAND voxel_tests)
  return()
endif()

include(FetchContent)
FetchContent_Declare(
  raylib
  URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
)
FetchContent_MakeAvailable(raylib)

add_executable(Voxel3D main.cpp game.h art.h)
target_include_directories(Voxel3D PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(Voxel3D PRIVATE raylib)

if (MSVC)
  target_compile_options(Voxel3D PRIVATE /W4)
else()
  target_compile_options(Voxel3D PRIVATE -Wall -Wextra)
endif()
