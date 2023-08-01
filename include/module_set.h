#ifndef MODULE_SET_H
#define MODULE_SET_H

#include "dr_api.h"

#include "module_debug_info.h"

/**
 * Initialises the stored module set
 *
 * Returns: Whether successful
 */
bool init_module_set(void);

/**
 * Deinitialises the stored module set
 *
 * Returns: Whether successful
 */
bool deinit_module_set(void);

/**
 * Initialises information for a loaded module
 */
void init_module(void *drcontext, const module_data_t *info, bool loaded);

/**
 * Finds the debug information for a module containing the given address
 *
 * Returns: Whether found
 */
bool find_module(void *addr, module_debug_t *info);

#endif
