cmake_minimum_required(VERSION 3.16)
project(bit_analyze VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(bit_analyze
    src/hierarchical_memory.cpp
    src/adaptive_memory.cpp
    src/compiled_encoder.cpp
    src/integrity.cpp
    src/recovery.cpp
    src/trail_recovery.cpp
    src/interleaving.cpp
    src/protection_policy.cpp
)
target_include_directories(bit_analyze PUBLIC include)

add_executable(bit_analyze_demo examples/demo.cpp)
target_link_libraries(bit_analyze_demo PRIVATE bit_analyze)

enable_testing()

function(add_ba_test target test_name source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE bit_analyze)
    add_test(NAME ${test_name} COMMAND ${target})
endfunction()

add_ba_test(bit_analyze_tests hierarchical_memory tests/test_hierarchical_memory.cpp)
add_ba_test(bit_analyze_adaptive_tests adaptive_memory tests/test_adaptive_memory.cpp)
add_ba_test(bit_analyze_generalization_tests generalization tests/test_generalization.cpp)
add_ba_test(bit_analyze_cost_benchmark cost_benchmark tests/test_cost_benchmark.cpp)
add_ba_test(bit_analyze_online_tests online_learning tests/test_online_learning.cpp)
add_ba_test(bit_analyze_online_retention_stress online_retention_stress tests/test_online_retention_stress.cpp)
add_ba_test(bit_analyze_compiled_encoder_tests compiled_encoder tests/test_compiled_encoder.cpp)
add_ba_test(bit_analyze_integrity_tests integrity tests/test_integrity.cpp)
add_ba_test(bit_analyze_recovery_tests recovery tests/test_recovery.cpp)
add_ba_test(bit_analyze_trail_recovery_tests trail_recovery tests/test_trail_recovery.cpp)
add_ba_test(bit_analyze_interleaving_tests interleaving tests/test_interleaving.cpp)
add_ba_test(bit_analyze_protection_policy_tests protection_policy tests/test_protection_policy.cpp)

function(add_ba_benchmark target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE bit_analyze)
endfunction()

add_ba_benchmark(bit_analyze_scale_benchmark benchmarks/scale_benchmark.cpp)
add_ba_benchmark(bit_analyze_diversity_benchmark benchmarks/diversity_benchmark.cpp)
add_ba_benchmark(bit_analyze_incremental_benchmark benchmarks/incremental_benchmark.cpp)
add_ba_benchmark(bit_analyze_capacity_sweep benchmarks/capacity_sweep.cpp)
add_ba_benchmark(bit_analyze_order_retention_benchmark benchmarks/order_retention_benchmark.cpp)
add_ba_benchmark(bit_analyze_online_vs_global benchmarks/online_vs_global.cpp)
add_ba_benchmark(bit_analyze_consolidation_benchmark benchmarks/consolidation_benchmark.cpp)
add_ba_benchmark(bit_analyze_multicycle_consolidation_benchmark benchmarks/multicycle_consolidation_benchmark.cpp)
add_ba_benchmark(bit_analyze_real_file_benchmark benchmarks/real_file_benchmark.cpp)
add_ba_benchmark(bit_analyze_real_scale_benchmark benchmarks/real_scale_benchmark.cpp)
add_ba_benchmark(bit_analyze_compiled_encoder_benchmark benchmarks/compiled_encoder_benchmark.cpp)
add_ba_benchmark(bit_analyze_protection_overhead benchmarks/protection_overhead.cpp)
add_ba_benchmark(bit_analyze_burst_interleaving_benchmark benchmarks/burst_interleaving_benchmark.cpp)
add_ba_benchmark(bit_analyze_corruption_probability_benchmark benchmarks/corruption_probability_benchmark.cpp)
add_ba_benchmark(bit_analyze_protection_profile_benchmark benchmarks/protection_profile_benchmark.cpp)
add_ba_benchmark(bit_analyze_adaptive_protection_benchmark benchmarks/adaptive_protection_benchmark.cpp)
add_ba_benchmark(bit_analyze_criticality_failure_benchmark benchmarks/criticality_failure_benchmark.cpp)
add_ba_benchmark(bit_analyze_criticality_group_failure_benchmark benchmarks/criticality_group_failure_benchmark.cpp)
add_ba_benchmark(bit_analyze_protection_quantile_sweep benchmarks/protection_quantile_sweep.cpp)
