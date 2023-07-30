#ifndef MODULE_DEBUG_INFO_H
#define MODULE_DEBUG_INFO_H

#include <dwarf.h>
#include <libdwarf.h>

#include "dr_api.h"

#include "query.h"

typedef struct entry {
    Dwarf_Half tag;
    Dwarf_Off offset;
    Dwarf_Bool isInfo;

    char *name;

    bool hasType;
    size_t type;

    bool hasLowPC;
    void *lowPC;

    bool hasHighPC;
    void *highPC;

    bool hasFbregOffset;
    int fbregOffset;

    bool hasAddr;
    void *addr;

    bool hasByteSize;
    size_t byteSize;

    size_t *children;
    int sizeChildren, capacityChildren;
} entry_t;

typedef struct module_debug {
    char *path;
    void *start;

    char **strs;
    int sizeStrs, capacityStrs;

    entry_t *entries;
    int sizeEntries, capacityEntries;

    size_t *roots;
    int sizeRoots, capacityRoots;
} module_debug_t;

/**
 * Gets the debugging information for a module, given its path
 *
 * Returns: Whether successful
 */
bool get_module_debug(const char *path, module_debug_t *info);

/**
 * Deinitialises the given module debugging information
 */
void deinit_module_debug(module_debug_t info);

/**
 * Finds debugging information in a module's debugging information given an
 * address, appending results to those given
 *
 * Returns: Whether successful
 */
bool module_query(module_debug_t info, query_t query, query_results_t *res);

#endif
