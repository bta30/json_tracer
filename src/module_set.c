#include "module_set.h"

#include <string.h>

#include "error.h"

#define MIN_CAPACITY 4

struct {
    module_debug_t *info;
    int infoSize, infoCapacity;

    void *mutex;
} moduleSet;

/**
 * Adds a module to the module set
 *
 * Returns: Whether successful
 */
static bool add_module(module_debug_t info);

/**
 * Returns if a module should have debug information retrieved
 */
static bool should_get_info(const module_data_t *info);

bool init_module_set(void) {
    PRINT_DEBUG("Enter init module set");

    moduleSet.mutex = dr_mutex_create();
    if (moduleSet.mutex == NULL) {
        PRINT_ERROR("Could not create mutex for module set");
        return false;
    }

    moduleSet.info = malloc(MIN_CAPACITY * sizeof(moduleSet.info[0]));
    moduleSet.infoSize = 0;
    moduleSet.infoCapacity = MIN_CAPACITY;
    if (moduleSet.info == NULL) {
        PRINT_ERROR("Could not allocate space for module set information");
        return false;
    }

    PRINT_DEBUG("Exit init module set");
}

bool deinit_module_set(void) {
    PRINT_DEBUG("Enter deinit module set");
    
    if (moduleSet.mutex != NULL) {
        dr_mutex_destroy(moduleSet.mutex);
    }

    if (moduleSet.info == NULL) {
        return true;
    }

    for (int i = 0; i < moduleSet.infoSize; i++) {
        deinit_module_debug(moduleSet.info[i]);
    }

    free(moduleSet.info);

    PRINT_DEBUG("Exit deinit module set");
    return true;
}

void init_module(void *drcontext, const module_data_t *info, bool loaded) {
    PRINT_DEBUG("Enter init module event");

    if (!should_get_info(info)) {
        PRINT_DEBUG("Exit init module event early");
        return;
    }

    module_debug_t debugInfo;

    if (!get_module_debug(info->full_path, &debugInfo)) {
        PRINT_ERROR("Could not get module debug info");
        EXIT_FAIL();
    }

    debugInfo.start = info->start;
    add_module(debugInfo);

    PRINT_DEBUG("Exit init module event");
}

bool find_module(void *addr, module_debug_t *info) {
    PRINT_DEBUG("Enter find module");

    module_data_t *module = dr_lookup_module(addr);
    if (module == NULL) {
        return false;
    }

    dr_mutex_lock(moduleSet.mutex);

    bool found = false;
    for (int i = 0; i < moduleSet.infoSize; i++) {
        if (strcmp(module->full_path, moduleSet.info[i].path) == 0) {
            found = true;
            *info = moduleSet.info[i];
            break;
        }
    }

    dr_mutex_unlock(moduleSet.mutex);
    dr_free_module_data(module);

    PRINT_DEBUG("Exit find module");
    return found;
}

static bool add_module(module_debug_t info) {
    dr_mutex_lock(moduleSet.mutex);
    
    if (moduleSet.infoSize == moduleSet.infoCapacity) {
        moduleSet.infoCapacity *= 2;
        moduleSet.info = reallocarray(moduleSet.info, moduleSet.infoCapacity,
                                      sizeof(moduleSet.info[0]));
        if (moduleSet.info == NULL) {
            PRINT_ERROR("Could not allocate space for new module");
            return false;
        }
    }

    moduleSet.info[moduleSet.infoSize++] = info;

    dr_mutex_unlock(moduleSet.mutex);
}

static bool should_get_info(const module_data_t *info) {
    return info->full_path[0] == '/';
}
