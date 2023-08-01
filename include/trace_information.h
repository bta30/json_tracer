#ifndef TRACE_INFORMATION_H
#define TRACE_INFORMATION_H

#include "dr_api.h"

#include "instr_vals_buffer.h"

/**
 * Flushes the given buffer to its JSON file
 *
 * Returns: Whether successful
 */
bool flush_vals_buffer(vals_buf_t *buf);

#endif
