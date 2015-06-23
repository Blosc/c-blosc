# sources
set(SOURCES bench.c)


# targets
add_executable(bench ${SOURCES})
if(UNIX AND NOT APPLE)
  # cmake is complaining about LINK_PRIVATE in original PR
  # and removing it does not seem to hurt, so be it.
  # target_link_libraries(bench LINK_PRIVATE rt)
  target_link_libraries(bench rt)
endif(UNIX AND NOT APPLE)
target_link_libraries(bench blosc_shared)

# have to copy blosc dlls on Windows
if(MSVC)
    add_custom_command(
        TARGET      bench
        POST_BUILD
        COMMAND     ${CMAKE_COMMAND}
        ARGS        -E copy_if_different
                    "${PROJECT_BINARY_DIR}/blosc/\$\(Configuration\)/blosc.dll"
                    "${CMAKE_CURRENT_BINARY_DIR}/\$\(Configuration\)/blosc.dll")
elseif(MINGW)
    add_custom_command(
        TARGET      bench
        POST_BUILD
        COMMAND     ${CMAKE_COMMAND}
        ARGS        -E copy_if_different
                    "${PROJECT_BINARY_DIR}/blosc/libblosc.dll"
                    "${CMAKE_CURRENT_BINARY_DIR}/libblosc.dll")
endif()

# tests
if(BUILD_TESTS)

    # The commented tests below take too much time to complete
    option(TEST_INCLUDE_BENCH_SINGLE_1 "Include bench single (1 thread) in the tests" ON)
    if(TEST_INCLUDE_BENCH_SINGLE_1)
        add_test(test_blosclz_1 bench blosclz byteshuf single 1)
        if (HAVE_LZ4)
            add_test(test_lz4_1     bench lz4 byteshuf single 1)
            # add_test(test_lz4hc_1   bench lz4hc byteshuf single 1)
        endif (HAVE_LZ4)
        if (HAVE_SNAPPY)
            add_test(test_snappy_1  bench snappy byteshuf single 1)
        endif (HAVE_SNAPPY)
        if (HAVE_ZLIB)
            # add_test(test_zlib_1    bench zlib byteshuf single 1)
        endif (HAVE_ZLIB)
    endif(TEST_INCLUDE_BENCH_SINGLE_1)

    option(TEST_INCLUDE_BENCH_BITSHUF_1 "Include bench bitshuf (1 thread) in the tests" ON)
    if(TEST_INCLUDE_BENCH_BITSHUF_1)
        add_test(test_blosclz_bitshuf_1    bench blosclz bitshuf single 1)
        if (HAVE_LZ4)
            add_test(test_lz4_bitshuf_1    bench lz4 bitshuf single 1)
            # add_test(test_lz4hc_bitshuf_1  bench lz4hc bitshuf single 1)
        endif (HAVE_LZ4)
        if (HAVE_SNAPPY)
            add_test(test_snappy_bitshuf_1 bench snappy bitshuf single 1)
        endif (HAVE_SNAPPY)
        if (HAVE_ZLIB)
            # add_test(test_zlib_bitshuf_1   bench zlib bitshuf single 1)
        endif (HAVE_ZLIB)
    endif(TEST_INCLUDE_BENCH_BITSHUF_1)

    option(TEST_INCLUDE_BENCH_N "Include bench byteshuf (multithread) in the tests" ON)
    if(TEST_INCLUDE_BENCH_N)
        add_test(test_blosclz_n bench blosclz byteshuf single)
        if (HAVE_LZ4)
            add_test(test_lz4_n     bench lz4 byteshuf single)
            add_test(test_lz4hc_n   bench lz4hc byteshuf single)
        endif (HAVE_LZ4)
        if (HAVE_SNAPPY)
            add_test(test_snappy_n  bench snappy byteshuf single)
        endif (HAVE_SNAPPY)
        if (HAVE_ZLIB)
            add_test(test_zlib_n    bench zlib byteshuf single)
        endif (HAVE_ZLIB)
    endif(TEST_INCLUDE_BENCH_N)

    option(TEST_INCLUDE_BENCH_BITSHUF_N "Include bench bitshuf (multithread) in the tests" ON)
    if(TEST_INCLUDE_BENCH_BITSHUF_N)
        add_test(test_blosclz_bitshuf_n bench blosclz bitshuf single)
        if (HAVE_LZ4)
            add_test(test_lz4_bitshuf_n     bench lz4 bitshuf single)
            # add_test(test_lz4hc_bitshuf_n   bench lz4hc bitshuf single)
        endif (HAVE_LZ4)
        if (HAVE_SNAPPY)
            add_test(test_snappy_bitshuf_n  bench snappy bitshuf single)
        endif (HAVE_SNAPPY)
        if (HAVE_ZLIB)
            # add_test(test_zlib_bitshuf_n    bench zlib bitshuf single)
        endif (HAVE_ZLIB)
    endif(TEST_INCLUDE_BENCH_BITSHUF_N)

    option(TEST_INCLUDE_BENCH_SUITE "Include bench suite in the tests" OFF)
    if(TEST_INCLUDE_BENCH_SUITE)
        add_test(test_hardsuite blosc blosclz byteshuf suite)
    endif(TEST_INCLUDE_BENCH_SUITE)

    option(TEST_INCLUDE_BENCH_DEBUGSUITE "Include bench debugsuite in the tests" OFF)
    if(TEST_INCLUDE_BENCH_DEBUGSUITE)
        add_test(test_debugsuite bench blosclz byteshuf debugsuite)
    endif(TEST_INCLUDE_BENCH_DEBUGSUITE)
endif(BUILD_TESTS)
