cmake_minimum_required(VERSION 3.16)
project(bit_analyze VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(bit_analyze
    src/hierarchical_memory.cpp
    src/adaptive_memory.cpp
)

target_include_directories(bit_analyze PUBLIC include)

add_executable(bit_analyze_demo examples/demo.cpp)
target_link_libraries(bit_analyze_demo PRIVATE bit_analyze)

enable_testing()

add_executable(bit_analyze_tests tests/test_hierarchical_memory.cpp)
target_link_libraries(bit_analyze_tests PRIVATE bit_analyze)
add_test(NAME hierarchical_memory COMMAND bit_analyze_tests)

add_executable(bit_analyze_adaptive_tests tests/test_adaptive_memory.cpp)
target_link_libraries(bit_analyze_adaptive_tests PRIVATE bit_analyze)
add_test(NAME adaptive_memory COMMAND bit_analyze_adaptive_tests)

add_executable(bit_analyze_generalization_tests tests/test_generalization.cpp)
target_link_libraries(bit_analyze_generalization_tests PRIVATE bit_analyze)
add_test(NAME generalization COMMAND bit_analyze_generalization_tests)

add_executable(bit_analyze_cost_benchmark tests/test_cost_benchmark.cpp)
target_link_libraries(bit_analyze_cost_benchmark PRIVATE bit_analyze)
add_test(NAME cost_benchmark COMMAND bit_analyze_cost_benchmark)
