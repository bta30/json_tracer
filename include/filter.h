#ifndef FILTER_H
#define FILTER_H

#include "dr_api.h"

typedef struct filter_entry {
    bool include;  // True for include, false for exclude
    bool matchNot; // True for match all except option, false for match option

    enum {
        module,
        file,
        instruction
    } type;

    char *value;  // NULL for any value
} filter_entry_t;

/**
 * Initialises the stored filter space, with a flag to include/exclude all
 * modules by default
 *
 * Returns: Whether successful
 */
bool init_filter(bool include);

/**
 * Deinitialises the stored filter space
 *
 * Returns: Whether successful
 */
bool deinit_filter(void);

/**
 * Adds filters given program arguments
 * 
 * Note: Newer filter entries supersede older entries in the case of a conflict
 *
 * Returns: Whether successful
 */
bool add_filter_entry(filter_entry_t entry);

/**
 * Find whether an entry for a given instruction should be included or excluded
 * in the final trace
 *
 * Returns: Whether to include
 */
bool filter_include_instr(instr_t *instr);

#endif
