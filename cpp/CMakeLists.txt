cmake_minimum_required(VERSION 3.16)
project(chip8 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CHIP8_BUILD_TESTS "Build the unit tests" ON)

# The emulator core has no windowing or audio dependency, so it can be unit tested
# on its own.
add_library(chip8_core STATIC src/core/cpu.cc)
target_include_directories(chip8_core PUBLIC src/core)

function(chip8_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra)
  endif()
endfunction()
chip8_warnings(chip8_core)

# The frontend uses SDL3 for the window, keyboard and beeper: the system copy if
# there is one, otherwise it is downloaded and built with the emulator.
include(FetchContent)
find_package(SDL3 3.2 CONFIG QUIET)
if(NOT SDL3_FOUND)
  FetchContent_Declare(SDL3
    URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.16/SDL3-3.4.16.tar.gz
    URL_HASH SHA256=7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68
    DOWNLOAD_EXTRACT_TIMESTAMP ON)
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(SDL3)
endif()

add_executable(chip8
  src/main.cc
  src/frontend/emulator.cc
  src/frontend/graphic.cc
  src/frontend/input.cc
  src/frontend/sound.cc
)
target_include_directories(chip8 PRIVATE src)
target_link_libraries(chip8 PRIVATE chip8_core SDL3::SDL3)
chip8_warnings(chip8)

if(CHIP8_BUILD_TESTS)
  enable_testing()

  # use the system googletest if there is one, otherwise download it
  find_package(GTest 1.10 QUIET)
  if(NOT GTest_FOUND)
    FetchContent_Declare(googletest
      URL https://github.com/google/googletest/releases/download/v1.17.0/googletest-1.17.0.tar.gz
      DOWNLOAD_EXTRACT_TIMESTAMP ON)
    # link the C runtime the same way as the tests on Windows
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
  endif()

  add_executable(chip8_test test/cpu_test.cc test/stack_test.cc)
  target_link_libraries(chip8_test PRIVATE chip8_core GTest::gtest_main)

  include(GoogleTest)
  gtest_discover_tests(chip8_test)
endif()
