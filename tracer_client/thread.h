#ifndef THREAD_H
#define THREAD_H

#include "dr_api.h"

#include "trace_entry.h"
#include "json_generation.h"

/**
 * Registers a tls slot
 *
 * Returns: Whether successful
 */
bool register_tls_slot(void);

/**
 * Unregisters the tls slot
 *
 * Returns: Whether successful
 */
bool unregister_tls_slot(void);

/**
 * Initialises a new thread
 */
void thread_init(void *drcontext);

/**
 * Deinitialises a thread
 */
void thread_deinit(void *drcontext);

/**
 * Enqueues a new instrumentation entry for the current thread
 *
 * Returns: Whether successful
 */
bool thread_entry_enqueue(void *drcontext, instrument_vals_t entry);

/**
 * Dequeues a instrumentation entry for the current thread
 *
 * Returns: Whether successful
 */
bool thread_entry_dequeue(void *drcontext, instrument_vals_t *entry);

/**
 * Checks if the instrumentation buffer for the current thread is empty
 *
 * Returns: True if the buffer is empty, false otherwise
 */
bool thread_buffer_empty(void *drcontext);

/**
 * Checks if the instrumentation buffer for the current thread is full
 *
 * Returns: True if the buffer is full, false otherwise
 */
bool thread_buffer_full(void *drcontext);

/**
 *  Returns the JSON file for the current thread
 */
json_file_t *thread_get_json_file(void *drcontext);

#endif
