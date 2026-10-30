cmake_minimum_required(VERSION 3.16)
project(algorithm_crossover_atlas CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The study holds the compiler and optimisation level fixed: GCC -O2.
# -march=native is OFF by default so results are tied to the ISA baseline
# (x86-64 SSE2) rather than to whatever vector extensions the host has;
# turn it on deliberately for the "does SIMD move the crossover?" variant.
option(ATLAS_MARCH_NATIVE "Compile with -march=native" OFF)
option(ATLAS_O3 "Use -O3 instead of -O2" OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "")  # we set flags explicitly below

set(ATLAS_FLAGS -Wall -Wextra -g -fno-omit-frame-pointer)
if(ATLAS_O3)
  list(APPEND ATLAS_FLAGS -O3)
else()
  list(APPEND ATLAS_FLAGS -O2)
endif()
if(ATLAS_MARCH_NATIVE)
  list(APPEND ATLAS_FLAGS -march=native)
endif()

add_library(harness INTERFACE)
target_include_directories(harness INTERFACE ${CMAKE_SOURCE_DIR}/src)
target_compile_options(harness INTERFACE ${ATLAS_FLAGS})
find_package(Threads REQUIRED)
target_link_libraries(harness INTERFACE Threads::Threads)

foreach(pair sorting matmul bigint dijkstra)
  add_executable(bench_${pair} src/${pair}/bench_${pair}.cpp)
  target_link_libraries(bench_${pair} PRIVATE harness)
  add_executable(test_${pair} tests/test_${pair}.cpp)
  target_link_libraries(test_${pair} PRIVATE harness)
endforeach()

enable_testing()
foreach(pair sorting matmul bigint dijkstra)
  add_test(NAME ${pair} COMMAND test_${pair})
endforeach()

# Record the exact flags for the reproducibility package.
file(WRITE ${CMAKE_BINARY_DIR}/build_flags.txt
  "compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}\nflags: ${ATLAS_FLAGS}\n")
