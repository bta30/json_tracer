#ifndef INSTR_VALS_BUFFER_H
#define INSTR_VALS_BUFFER_H

#include "dr_api.h"

#include "trace_entry.h"
#include "json_generation.h"

typedef struct vals_buf {
    instrument_vals_t buf[BUF_LEN];
    int head;
    int size;
    json_file_t file;
} vals_buf_t;

typedef struct vals_buf_opts {
    bool interleaved, separate;
    const char *prefix;
} vals_buf_opts_t;

/**
 * Initialises the values buffer
 *
 * Returns: Whether successful
 */
bool init_vals_buf(vals_buf_opts_t opts);

/**
 * Deinitialises the values buffer
 *
 * Returns: Whether successful
 */
bool deinit_vals_buf(void);

/**
 * Initialises a new thread to use the buffer
 */
void thread_init(void *drcontext);

/**
 * Deinitialises a thread from using the buffer
 */
void thread_deinit(void *drcontext);

/**
 * Enqueues a new instrumentation entry for the current thread
 *
 * Returns: Whether successful
 */
bool entry_enqueue(void *drcontext, instrument_vals_t entry);

/**
 * Dequeues an instrumentation entry from the given buffer
 *
 * Returns: Whether successful
 */
bool entry_dequeue(vals_buf_t *buf, instrument_vals_t *entry);

#endif
