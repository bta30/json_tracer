#include "dr_api.h"
#include "drmgr.h"
#include "drsyms.h"

#include "args.h"
#include "thread.h"
#include "insert_instrumentation.h"
#include "module_set.h"
#include "error.h"

/**
 * Deinitialises the client
 */
static void client_deinit(void);

/**
 * Initialises DynamoRIO extensions
 *
 * Returns: Whether successful
 */
static bool exts_init(void);

/**
 * Deinitialises DynamoRIO extensions
 *
 * Returns: Whether successful
 */
static bool exts_deinit(void);

/**
 * Registers callbacks for application events
 *
 * Returns: Whether successful
 */
static bool register_callbacks(void);

/**
 * Unregisters callbacks for application events
 *
 * Returns: Whether successful
 */
static bool unregister_callbacks(void);

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char **argv) {
    PRINT_DEBUG("Entered initialise client");

    dr_set_client_name("JSON Tracer", "https://github.com/bta30/json_tracer");

    bool successful = exts_init() && register_callbacks()
                                  && register_tls_slot()
                                  && init_module_set()
                                  && process_args(argc, argv);
    if (!successful) {
        EXIT_FAIL();
    }

    PRINT_DEBUG("Exited initialise client");
}

static void client_deinit(void) {
    bool successful = deinit_args() && deinit_module_set()
                                    && unregister_tls_slot()
                                    && unregister_callbacks()
                                    && exts_deinit();
    if (!successful) {
        EXIT_FAIL();
    }
}

static bool exts_init(void) {
    if (!drmgr_init()) {
        PRINT_ERROR("Could not initialise drmgr");
        return false;
    }

    if (drsym_init(0) != DRSYM_SUCCESS) {
        PRINT_ERROR("Could not initialise drsyms");
        return false;
    }

    return true;
}

static bool exts_deinit(void) {
    if (drsym_exit() != DRSYM_SUCCESS) {
        PRINT_ERROR("Could not exit drsyms");
        return false;
    }

    drmgr_exit();
    
    return true;
}

static bool register_callbacks(void) {
    dr_register_exit_event(client_deinit);

    if (!drmgr_register_thread_init_event(thread_init)) {
        PRINT_ERROR("Could not register thread init event");
        return false;
    }

    if (!drmgr_register_thread_exit_event(thread_deinit)) {
        PRINT_ERROR("Could not register thread exit event");
        return false;
    }

    if (!drmgr_register_module_load_event(init_module)) {
        PRINT_ERROR("Could not register module load event");
    }

    bool success = drmgr_register_bb_instrumentation_event(NULL,
                                                insert_instrumentation, NULL);
    if (!success) {
        PRINT_ERROR("Could not register basic block instrumentation insertion");
        return false;
    }

    return true;
}

static bool unregister_callbacks(void) {
    if (!drmgr_unregister_bb_insertion_event(insert_instrumentation)) {
        PRINT_ERROR("Could not unregister basic block instrumentation insertion");
        return false;
    }

    if (!drmgr_unregister_thread_exit_event(thread_deinit)) {
        PRINT_ERROR("Could not unregister thread exit event");
        return false;
    }

    if (!drmgr_unregister_thread_init_event(thread_init)) {
        PRINT_ERROR("Could not unregister thread init event");
        return false;
    }

    if (!drmgr_unregister_module_load_event(init_module)) {
        PRINT_ERROR("Could not unregister module load event");
    }

    if (!dr_unregister_exit_event(client_deinit)) {
        PRINT_ERROR("Could not unregister thread exit event");
        return false;
    }

    return true;
}
