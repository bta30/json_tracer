#ifndef ARGS_H
#define ARGS_H

#include "dr_api.h"

/**
 * Processes client arguments
 *
 * Returns: Whether successful
 */
bool process_args(int argc, const char **argv);

/**
 * Deinitialises processed arguments, at end of tracer execution
 *
 * Returns: Whether successful
 */
bool deinit_args(void);

#endif
