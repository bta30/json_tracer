#include "thread.h"

#include "dr_api.h"
#include "drmgr.h"

#include "trace_entry.h"
#include "trace_information.h"
#include "json_generation.h"
#include "error.h"

typedef struct thread_data {
    instrument_vals_t buf[BUF_LEN];
    int head;
    int size;
    json_file_t file;
} thread_data_t;

static int tlsSlot = -1;

/**
 * Sets the data for the current thread
 */
static bool set_thread_data(void *drcontext, thread_data_t *data);

/**
 * Gets the data for the current thread
 */
static thread_data_t *get_thread_data(void *drcontext);

bool register_tls_slot(void) {
    tlsSlot = drmgr_register_tls_field();
    if (tlsSlot == -1) {
        PRINT_ERROR("Could not register tls slot");
    }

    return tlsSlot != -1;
}

bool unregister_tls_slot(void) {
    if (!drmgr_unregister_tls_field(tlsSlot)) {
        PRINT_ERROR("Could not unregister tls slot");
        return false;
    }

    return true;
}


void thread_init(void *drcontext) {
    PRINT_DEBUG("Entered thread init");

    thread_data_t *data = dr_thread_alloc(drcontext, sizeof(thread_data_t));
    if (data == NULL) {
        PRINT_ERROR("Could not allocate thread data");
        EXIT_FAIL();
    }

    if (!set_thread_data(drcontext, data)) {
        EXIT_FAIL();
    }

    data->head = 0;
    data->size = 0;
    data->file = open_json_file();

    PRINT_DEBUG("Exited thread init");
}

void thread_deinit(void *drcontext) {
    PRINT_DEBUG("Entered thread deinit");

    flush_thread_buffer(drcontext);
    thread_data_t *data = get_thread_data(drcontext);
    close_json_file(data->file);
    dr_thread_free(drcontext, data, sizeof(thread_data_t));

    PRINT_DEBUG("Exited thread deinit");
}

bool thread_entry_enqueue(void *drcontext, instrument_vals_t entry) {
    thread_data_t *data = get_thread_data(drcontext);
    
    if (data->size == BUF_LEN) {
        PRINT_ERROR("Could not enqueue entry");
        return false;
    }

    int index = (data->head + data->size++) % BUF_LEN;
    data->buf[index] = entry;
    return true;
}

bool thread_entry_dequeue(void *drcontext, instrument_vals_t *entry) {
    thread_data_t *data = get_thread_data(drcontext);

    if (data->size == 0) {
        PRINT_ERROR("Could not dequeue entry");
        return false;
    }

    data->size--;
    *entry = data->buf[data->head++];
    data->head %= BUF_LEN;
    return true;
}

bool thread_buffer_empty(void *drcontext) {
    thread_data_t *data = get_thread_data(drcontext);
    return data->size == 0;
}

bool thread_buffer_full(void *drcontext) {
    thread_data_t *data = get_thread_data(drcontext);
    return data->size == BUF_LEN;
}

json_file_t *thread_get_json_file(void *drcontext) {
    thread_data_t *data = get_thread_data(drcontext);
    return &data->file;
}

static bool set_thread_data(void *drcontext, thread_data_t *data) {
    if (!drmgr_set_tls_field(drcontext, tlsSlot, data)) {
        PRINT_ERROR("Could not set thread data for given slot");
        return false;
    }

    return true;
}

static thread_data_t *get_thread_data(void *drcontext) {
    return drmgr_get_tls_field(drcontext, tlsSlot);
}
