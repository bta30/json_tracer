add_subdirectory(sum)

set(TESTSRCS
    ${CMAKE_SOURCE_DIR}/test/src/tests.c
)

add_executable(tests ${TESTSRCS})
target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/test/include)

add_test(test, ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests)
