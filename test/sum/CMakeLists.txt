set(SRCS
    ${CMAKE_SOURCE_DIR}/test/sum/src/args.cpp
    ${CMAKE_SOURCE_DIR}/test/sum/src/main.cpp
    ${CMAKE_SOURCE_DIR}/test/sum/src/summariser.cpp
)

add_executable(sum ${SRCS})
target_include_directories(sum PRIVATE ${CMAKE_SOURCE_DIR}/test/sum/include)
target_compile_options(sum PRIVATE -O0 -g3 -gdwarf-4)
