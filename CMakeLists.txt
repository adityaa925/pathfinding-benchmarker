cmake_minimum_required(VERSION 3.16)
project(pathfinding_benchmarker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Benchmarks are meaningless without optimisation, so default to Release.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Header-only library: include/pf/*.hpp
add_library(pf INTERFACE)
target_include_directories(pf INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(MSVC)
  target_compile_options(pf INTERFACE /W4)
else()
  target_compile_options(pf INTERFACE -Wall -Wextra -Wpedantic)
endif()

add_executable(pfbench bench/main.cpp bench/mem_tracker.cpp)
target_link_libraries(pfbench PRIVATE pf)

enable_testing()
add_executable(pf_tests tests/test_main.cpp)
target_link_libraries(pf_tests PRIVATE pf)
add_test(NAME pf_tests COMMAND pf_tests)

# Optional: AddressSanitizer + UBSan build of the tests (GCC/Clang):  cmake -DPF_SANITIZE=ON ..
option(PF_SANITIZE "Build tests with ASan+UBSan" OFF)
if(PF_SANITIZE AND NOT MSVC)
  target_compile_options(pf_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer -g)
  target_link_options(pf_tests PRIVATE -fsanitize=address,undefined)
endif()
