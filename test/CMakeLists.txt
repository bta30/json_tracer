cmake_minimum_required(VERSION 3.20)
project(json_tracer_tests)

include(CTest)
enable_testing()

add_subdirectory(sum)

set(TESTSRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tests.c
)

add_executable(tests ${TESTSRCS})
target_include_directories(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_test(test, ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests)
