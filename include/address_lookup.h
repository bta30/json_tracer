#ifndef ADDRESS_LOOKUP_H
#define ADDRESS_LOOKUP_H

#include "dr_api.h"
#include "drsyms.h"

/**
 * Looks up file/function name information for a given address
 *
 * Returns: True if more buffer space if required, otherwise false
 */
bool lookup_address(void *addr, drsym_info_t *info, bool persistent);

/**
 * Initialises the lookup hashtable
 */
void init_lookup(void);

/**
 * Deinitialises the lookup hashtable
 */
void deinit_lookup(void);

#endif
