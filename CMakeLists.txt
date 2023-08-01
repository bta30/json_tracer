cmake_minimum_required(VERSION 3.26)
project(json_tracer)

set(DYNAMORIO_VERSION 9.93.19566)
set(LIBDWARF_VERSION 0.7.0)

set(SRCS 
    ${PROJECT_SOURCE_DIR}/src/args.c
    ${PROJECT_SOURCE_DIR}/src/client.c
    ${PROJECT_SOURCE_DIR}/src/filter.c
    ${PROJECT_SOURCE_DIR}/src/insert_instrumentation.c
    ${PROJECT_SOURCE_DIR}/src/instr_vals_buffer.c
    ${PROJECT_SOURCE_DIR}/src/json_generation.c
    ${PROJECT_SOURCE_DIR}/src/module_debug_info_init.c
    ${PROJECT_SOURCE_DIR}/src/module_query.c
    ${PROJECT_SOURCE_DIR}/src/module_set.c
    ${PROJECT_SOURCE_DIR}/src/query.c
    ${PROJECT_SOURCE_DIR}/src/trace_entry.c
    ${PROJECT_SOURCE_DIR}/src/trace_information.c
)    
     
set(EXTS drmgr drx drsyms)

set(DynamoRIO_DIR "${PROJECT_SOURCE_DIR}/external/DynamoRIO-Linux-${DYNAMORIO_VERSION}/cmake")
find_package(DynamoRIO)
if (NOT DynamoRIO_FOUND)
    message(FATAL_ERROR "Could not find DynamoRIO - have you run"
                        "./configure.sh?")
endif (NOT DynamoRIO_FOUND)

set(LIBDWARF "${PROJECT_SOURCE_DIR}/external/libdwarf-${LIBDWARF_VERSION}/build/src/lib/libdwarf/libdwarf.a")
set(LIBDWARF_INCLUDE "${PROJECT_SOURCE_DIR}/external/libdwarf-${LIBDWARF_VERSION}/src/lib/libdwarf")

add_library(jsontracer SHARED ${SRCS})
target_include_directories(jsontracer PRIVATE ${PROJECT_SOURCE_DIR}/include
                                              ${LIBDWARF_INCLUDE})
target_link_libraries(jsontracer ${LIBDWARF})
add_compile_options(-Wall -Wextra -Wpedantic -fPIC -Wl,--whole-archive)

configure_DynamoRIO_client(jsontracer)
foreach(EXT ${EXTS})
    use_DynamoRIO_extension(jsontracer "${EXT}")
endforeach (EXT ${EXTS})
