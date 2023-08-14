cmake_minimum_required(VERSION 3.26)
project(json_tracer)

set(DYNAMORIO_VERSION 9.93.19566)
set(LIBDWARF_VERSION 0.7.0)

set(SRCS 
    ${CMAKE_SOURCE_DIR}/src/args.c
    ${CMAKE_SOURCE_DIR}/src/client.c
    ${CMAKE_SOURCE_DIR}/src/filter.c
    ${CMAKE_SOURCE_DIR}/src/insert_instrumentation.c
    ${CMAKE_SOURCE_DIR}/src/instr_vals_buffer.c
    ${CMAKE_SOURCE_DIR}/src/json_generation.c
    ${CMAKE_SOURCE_DIR}/src/module_debug_info_init.c
    ${CMAKE_SOURCE_DIR}/src/module_query.c
    ${CMAKE_SOURCE_DIR}/src/module_set.c
    ${CMAKE_SOURCE_DIR}/src/query.c
    ${CMAKE_SOURCE_DIR}/src/trace_entry.c
    ${CMAKE_SOURCE_DIR}/src/trace_information.c
    ${CMAKE_SOURCE_DIR}/src/extra_debug_info.c
    ${CMAKE_SOURCE_DIR}/src/address_lookup.c
)    
     
set(EXTS drmgr drx drsyms)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

set(DynamoRIO_DIR "${CMAKE_SOURCE_DIR}/external/DynamoRIO-Linux-${DYNAMORIO_VERSION}/cmake")
find_package(DynamoRIO)
if (NOT DynamoRIO_FOUND)
    message(FATAL_ERROR "Could not find DynamoRIO - have you run"
                        "./configure.sh?")
endif (NOT DynamoRIO_FOUND)

set(LIBDWARF "${CMAKE_SOURCE_DIR}/external/libdwarf-${LIBDWARF_VERSION}/build/src/lib/libdwarf/libdwarf.a")
set(LIBDWARF_INCLUDE "${CMAKE_SOURCE_DIR}/external/libdwarf-${LIBDWARF_VERSION}/src/lib/libdwarf")

add_library(jsontracer SHARED ${SRCS})
target_include_directories(jsontracer PRIVATE ${CMAKE_SOURCE_DIR}/include
                                              ${LIBDWARF_INCLUDE})
target_link_libraries(jsontracer ${LIBDWARF})
target_compile_options(jsontracer PRIVATE -fPIC -Wl,--whole-archive)

configure_DynamoRIO_client(jsontracer)
foreach(EXT ${EXTS})
    use_DynamoRIO_extension(jsontracer "${EXT}")
endforeach (EXT ${EXTS})

include(CTest)
enable_testing()
add_subdirectory(test)
