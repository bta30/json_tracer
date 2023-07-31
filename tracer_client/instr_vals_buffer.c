#include "instr_vals_buffer.h"

#include "dr_api.h"
#include "drmgr.h"

#include "trace_entry.h"
#include "trace_information.h"
#include "json_generation.h"
#include "error.h"

static struct {
    bool separate, interleaved;
    int tlsSlot;
    vals_buf_t interleavedBuf;
    void *bufLock;
} bufDetails;

/**
 * Initialises a new instrumented values buffer
 *
 * Returns: Whether successful
 */
static bool init_buf(vals_buf_t *buf);

/**
 * Deinitialises a given instrumented values buffer
 */
static void deinit_buf(vals_buf_t *buf);

/**
 * Enqueues a value onto the given buffer
 *
 * Returns: Whether successful
 */
static bool buf_enqueue(vals_buf_t *buf, instrument_vals_t entry);

/**
 * Sets the buffer for the current thread
 */
static bool set_thread_buffer(void *drcontext, vals_buf_t *buf);

/**
 * Gets the buffer for the current thread
 */
static vals_buf_t *get_thread_buffer(void *drcontext);

bool init_vals_buf(bool separate, bool interleaved) {
    bufDetails.separate = separate;
    bufDetails.interleaved = interleaved;

    bool success = true;

    if (bufDetails.separate) {
        bufDetails.tlsSlot = drmgr_register_tls_field();
        if (bufDetails.tlsSlot == -1) {
            PRINT_ERROR("Could not register tls slot");
            success = false;
        }
    }

    if (success && bufDetails.interleaved) {
        if (!init_buf(&bufDetails.interleavedBuf)) {
            PRINT_ERROR("Could not initialised interleaved buffer");
            success = false;
        }
        bufDetails.bufLock = dr_mutex_create();
    }

    return success;
}

bool deinit_vals_buf(void) {
    bool success = !bufDetails.separate ||
                   drmgr_unregister_tls_field(bufDetails.tlsSlot);
    if (!success) {
        PRINT_ERROR("Could not unregister tls slot");
        return false;
    }

    if (bufDetails.interleaved) {
        dr_mutex_destroy(bufDetails.bufLock);
        deinit_buf(&bufDetails.interleavedBuf);
    }

    return true;
}

void thread_init(void *drcontext) {
    PRINT_DEBUG("Entered thread init");

    if (!bufDetails.separate) {
        return;
    }

    vals_buf_t *buf = dr_thread_alloc(drcontext, sizeof(vals_buf_t));
    if (buf == NULL) {
        PRINT_ERROR("Could not allocate thread buffer");
        EXIT_FAIL();
    }

    if (!set_thread_buffer(drcontext, buf)) {
        EXIT_FAIL();
    }

    init_buf(buf);

    PRINT_DEBUG("Exited thread init");
}

void thread_deinit(void *drcontext) {
    PRINT_DEBUG("Entered thread deinit");

    if (!bufDetails.separate) {
        return;
    }

    vals_buf_t *buf = get_thread_buffer(drcontext);
    deinit_buf(buf);
    dr_thread_free(drcontext, buf, sizeof(vals_buf_t));

    PRINT_DEBUG("Exited thread deinit");
}

bool entry_enqueue(void *drcontext, instrument_vals_t entry) {
    bool success = true;

    if (bufDetails.separate) {
        vals_buf_t *buf = get_thread_buffer(drcontext);
        success = buf_enqueue(buf, entry);
    }

    if (success && bufDetails.interleaved) {
        dr_mutex_lock(bufDetails.bufLock);
        success = buf_enqueue(&bufDetails.interleavedBuf, entry);
        dr_mutex_unlock(bufDetails.bufLock);
    }
    
    return success;
}

bool entry_dequeue(vals_buf_t *buf, instrument_vals_t *entry) {
    if (buf->size == 0) {
        PRINT_ERROR("Could not dequeue entry");
        return false;
    }

    buf->size--;
    *entry = buf->buf[buf->head++];
    buf->head %= BUF_LEN;
    return true;
}

static bool init_buf(vals_buf_t *buf) {
    buf->head = 0;
    buf->size = 0;
    buf->file = open_json_file();

    return true;
}

static bool buf_enqueue(vals_buf_t *buf, instrument_vals_t entry) {
    if (buf->size == BUF_LEN && !flush_vals_buffer(buf)) {
        PRINT_ERROR("Could not enqueue/flush instrumented values buffer");
        return false;
    }

    int index = (buf->head + buf->size++) % BUF_LEN;
    buf->buf[index] = entry;
    return true;
}

static void deinit_buf(vals_buf_t *buf) {
    flush_vals_buffer(buf);
    close_json_file(buf->file);
}

static bool set_thread_buffer(void *drcontext, vals_buf_t *buf) {
    if (!drmgr_set_tls_field(drcontext, bufDetails.tlsSlot, buf)) {
        PRINT_ERROR("Could not set thread buffer for given slot");
        return false;
    }

    return true;
}

static vals_buf_t *get_thread_buffer(void *drcontext) {
    return drmgr_get_tls_field(drcontext, bufDetails.tlsSlot);
}
