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

if(MSVC)
  target_compile_options(chip8_core PRIVATE /W4)
else()
  target_compile_options(chip8_core PRIVATE -Wall -Wextra)
endif()

# The frontend uses SDL 1.2 for the window, keyboard and beeper. On Windows, the copy
# under ./include and ./lib is used unless SDLDIR points elsewhere.
if(WIN32 AND NOT DEFINED ENV{SDLDIR})
  list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR})
endif()
find_package(SDL 1.2 REQUIRED)

add_executable(chip8
  src/main.cc
  src/frontend/emulator.cc
  src/frontend/graphic.cc
  src/frontend/input.cc
  src/frontend/sound.cc
)
target_include_directories(chip8 PRIVATE src ${SDL_INCLUDE_DIR})
target_link_libraries(chip8 PRIVATE chip8_core ${SDL_LIBRARY})

if(WIN32 AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/SDL.dll)
  add_custom_command(TARGET chip8 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_CURRENT_SOURCE_DIR}/SDL.dll $<TARGET_FILE_DIR:chip8>)
endif()

if(CHIP8_BUILD_TESTS)
  enable_testing()

  # use the system googletest if there is one, otherwise download it
  find_package(GTest 1.10 QUIET)
  if(NOT GTest_FOUND)
    include(FetchContent)
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
