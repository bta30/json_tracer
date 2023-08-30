#ifndef EXTRA_DEBUG_INFO_H
#define EXTRA_DEBUG_INFO_H

#include "dr_api.h"

#include "module_debug_info.h"

typedef struct debug_file {
    file_t fd;
    bool firstLine;
} debug_file_t;

/**
 * Opens a new file for writing extra debug information
 *
 * Returns: Whether successful
 */
bool open_debug_file(const char *path, debug_file_t *file);

/**
 * Closes an open debug_file_t
 */
void close_debug_file(debug_file_t file);

/**
 * Writes information to a debug_file_t given a module's debugging information
 *
 * Returns: Whether successful
 */
bool write_module_debug_info(debug_file_t *file, module_debug_t *info);

#endif
