cmake_minimum_required(VERSION 3.26)
project(jsontracer)

set(SRCS client.c thread.c insert_instrumentation.c trace_information.c
         json_generation.c trace_entry.c module_debug_info_init.c
         module_query.c query.c module_set.c args.c filter.c)
set(EXTS drmgr drx drsyms)

find_package(DynamoRIO)
if (NOT DynamoRIO_FOUND)
    message(FATAL_ERROR "Error: Could not find DynamoRIO - please set "
                        "DynamoRIO_DIR")
endif (NOT DynamoRIO_FOUND)

find_library(DWARF_PATH libdwarf.a HINTS ENV LIBDWARF_DIR)
if (NOT DWARF_PATH)
    message(FATAL_ERROR "Error: Unable to find libdwarf - please set "
                        "LIBDWARF_DIR to the path of libdwarf.a")
endif (NOT DWARF_PATH)
message(${DWARF_PATH})

find_path(DWARF_INCLUDE_DIR libdwarf.h HINTS ENV LIBDWARF_INCLUDES)
if (NOT DWARF_INCLUDE_DIR)
    message(FATAL_ERROR "Error: Could not find includes for libdwarf - please"
                        " set LIBDWARF_INCLUDES")
endif (NOT DWARF_INCLUDE_DIR)
message(${DWARF_INCLUDE_DIR})

add_library(jsontracer SHARED ${SRCS})

target_include_directories(jsontracer PRIVATE ${DWARF_INCLUDE_DIR})
target_link_libraries(jsontracer ${DWARF_PATH})
add_compile_options(-Wall -Wextra -Wpedantic -fPIC -Wl,--whole-archive)

configure_DynamoRIO_client(jsontracer)
foreach(EXT ${EXTS})
    use_DynamoRIO_extension(jsontracer "${EXT}")
endforeach (EXT ${EXTS})
