set(SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/args.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/summariser.cpp
)

add_executable(sum ${SRCS})
target_include_directories(sum PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(sum PRIVATE -O0 -g3 -gdwarf-4 -std=c++11)
