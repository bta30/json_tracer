#ifndef TRACE_INFORMATION_H
#define TRACE_INFORMATION_H

#include "dr_api.h"

/**
 * Flushed the buffer for the current thread to the JSON file
 *
 * Returns: Whether successful
 */
bool flush_thread_buffer(void *drcontext);

#endif
