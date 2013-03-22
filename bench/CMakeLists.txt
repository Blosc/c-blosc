# sources
set(SOURCES bench.c)


# targets
add_executable(bench ${SOURCES})
target_link_libraries(bench blosc_shared)


# tests
if(BUILD_TESTS)
    set(BENCH_N_THREADS 4
        CACHE STRING "Number of threads to use for benchmarking")

    option(TEST_INCLUDE_BENCH_SINGLE "Include bench_single in the tests" OFF)
    if(TEST_INCLUDE_BENCH_SINGLE)
        add_test(bench_single bench single)
    endif(TEST_INCLUDE_BENCH_SINGLE)

    option(TEST_INCLUDE_BENCH_SUITE "Include bench_suite in the tests" ON)
    if(TEST_INCLUDE_BENCH_SUITE)
        add_test(bench_suite bench suite ${BENCH_N_THREADS})
    endif(TEST_INCLUDE_BENCH_SUITE)

    option(TEST_INCLUDE_BENCH_SUITE_PARALLEL
        "Include bench_suite_parallel in the tests" OFF)
    if(TEST_INCLUDE_BENCH_SUITE_PARALLEL)
        add_test(bench_suite_parallel bench suite_parallel ${BENCH_N_THREADS})
    endif(TEST_INCLUDE_BENCH_SUITE_PARALLEL)

    option(TEST_INCLUDE_BENCH_HARDSUITE
        "Include bench_hardsuite in the tests" OFF)
    if(TEST_INCLUDE_BENCH_HARDSUITE)
        add_test(bench_hardsuite bench hardsuite ${BENCH_N_THREADS})
    endif(TEST_INCLUDE_BENCH_HARDSUITE)

    option(TEST_INCLUDE_BENCH_EXTREMESUITE
        "Include bench_extremesuite in the tests" OFF)
    if(TEST_INCLUDE_BENCH_EXTREMESUITE)
        add_test(bench_extremesuite bench extremesuite ${BENCH_N_THREADS})
    endif(TEST_INCLUDE_BENCH_EXTREMESUITE)

    option(TEST_INCLUDE_BENCH_DEBUGSUITE
        "Include bench_debugsuite in the tests" OFF)
    if(TEST_INCLUDE_BENCH_DEBUGSUITE)
        add_test(bench_edebugsuite bench debugsuite)
    endif(TEST_INCLUDE_BENCH_DEBUGSUITE)
endif(BUILD_TESTS)
