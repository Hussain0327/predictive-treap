cmake_minimum_required(VERSION 3.21)
project(predictive_treap LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(pt INTERFACE)
target_include_directories(pt INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/include)

include(FetchContent)
# CMake 3.24+ warns unless the archive timestamp policy is chosen.
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(CTest)
include(GoogleTest)

# Warnings apply to our targets only, never to googletest.
function(pt_add_test name source)
  add_executable(${name} ${source})
  target_link_libraries(${name} PRIVATE pt GTest::gtest_main)
  target_compile_options(${name} PRIVATE
    -Wall -Wextra -Wpedantic -Werror)
  gtest_discover_tests(${name})
endfunction()

pt_add_test(smoke_test tests/smoke_test.cc)
