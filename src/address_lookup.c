#include "address_lookup.h"

#include <stdlib.h>
#include <string.h>

#include "error.h"

typedef struct address_results {
    char buf[256];
    char *file, *name;
    size_t fileSize, nameSize;
    uint line;
} address_results_t;

static void *drcontext = NULL;
static void *hashtable = NULL;

/**
 * Sets the given drsym_info_t to blank, freeing buffers
 */
static void set_blank(drsym_info_t *info);

/**
 * Allocates a hashtable entry
 *
 * Returns: A pointer to the entry on success, otherwise NULL
 */
static void *alloc_entry(drsym_info_t *info);

/**
 * Destroys a hashtable entry
 */
static void destroy_entry(void *drcontext, void *entry);

/**
 * Sets a drsym_info_t to information stored in a hashtable entry
 *
 * Returns: Whether to retry with a larger buffer size
 */
static bool save_entry(void *entry, drsym_info_t *info);

bool lookup_address(void *addr, drsym_info_t *info, bool persistent) {
    PRINT_DEBUG("Enter lookup address");
    persistent = false;

    if (persistent) {
        void *lookupAddress = dr_hashtable_lookup(drcontext, hashtable,
                                                  (ptr_uint_t)addr);
        if (lookupAddress != NULL) {
            PRINT_DEBUG("Exit lookup address at lookup in table");
            return save_entry(lookupAddress, info);
        }
    }

    module_data_t *module = dr_lookup_module(addr);
    size_t offset = addr - (void *)module->start;

    drsym_error_t err = drsym_lookup_address(module->full_path, offset, info,
                                             DRSYM_DEFAULT_FLAGS);

    dr_free_module_data(module);

    if (err != DRSYM_SUCCESS) {
        if (persistent) {
            dr_hashtable_add(drcontext, hashtable, (ptr_uint_t)addr, (void *)1);
        }
        set_blank(info);
        PRINT_DEBUG("Exit lookup address at fail lookup");
        return false;
    }

    bool retry = info->file_size == info->file_available_size + 1 ||
                 info->name_size == info->name_available_size + 1;

    if (!retry && persistent) {
        void *entry = alloc_entry(info);
        if (entry == NULL) {
            PRINT_DEBUG("Exit lookup address at null entry");
            return false;
        }

        dr_hashtable_add(drcontext, hashtable, (ptr_uint_t)addr, entry);
    }

    PRINT_DEBUG("Exit lookup address");
    return retry;
}

void init_lookup(void) {
    PRINT_DEBUG("Enter init lookup");

    //drcontext = dr_get_current_drcontext();
    //hashtable = dr_hashtable_create(drcontext, 16, 25, true, destroy_entry);

    PRINT_DEBUG("Exit init lookup");
}

void deinit_lookup(void) {
    PRINT_DEBUG("Enter destroy lookup");

    //dr_hashtable_destroy(drcontext, hashtable);
    //hashtable = NULL;
    //drcontext = NULL;

    PRINT_DEBUG("Exit destroy lookup");
}

static void set_blank(drsym_info_t *info) {
    if (info->file != NULL) {
        __wrap_free(info->file);
        info->file = NULL;
    }
    info->file_size = 0;
    info->file_available_size = 0;

    if (info->name != NULL) {
        __wrap_free(info->name);
        info->name = NULL;
    }
    info->name_size = 0;
    info->name_available_size = 0;

    info->line = 0;
}

static void *alloc_entry(drsym_info_t *info) {
    address_results_t *entry = dr_global_alloc(sizeof(address_results_t));
    if (entry == NULL) {
        return NULL;
    }

    entry->fileSize = info->file == NULL ? 0 : strlen(info->file);
    entry->nameSize = info->name == NULL ? 0 : strlen(info->name);

    if (entry->fileSize + entry->nameSize > sizeof(entry->buf)/sizeof(entry->buf[0])) {
        dr_global_free(entry, sizeof(*entry));
        return NULL;
    }

    entry->file = entry->buf;
    if (info->file != NULL && info->file_available_size > 0) {
        strcpy(entry->file, info->file);
    } else {
        entry->file[0] = '\0';
    }

    entry->name = entry->buf + entry->fileSize;
    if (info->name != NULL && info->name_available_size > 0) {
        strcpy(entry->name, info->name);
    } else {
        entry->name[0] = '\0';
    }

    entry->line = info->line;

    return (void *)entry;
}

static void destroy_entry(void *drcontext, void *entry) {
    if (entry != (void *)1) {
        dr_global_free(entry, sizeof(address_results_t));
    }
}

static bool save_entry(void *entry, drsym_info_t *info) {
    if (entry == (void *)1) {
        set_blank(info);
        return false;
    }

    address_results_t *res = (address_results_t *)entry;
    bool retry = (info->file_size < res->fileSize && res->fileSize != 0) ||
                 (info->name_size < res->nameSize && res->nameSize != 0);
    if (retry) {
        return true;
    }

    if (res->fileSize == 0 && info->file != NULL) {
        __wrap_free(info->file);
        info->file = NULL;
    } else if (res->fileSize > 0) {
        strcpy(info->file, res->file);
    }

    if (res->nameSize == 0 && info->name != NULL) {
        __wrap_free(info->name);
        info->name = NULL;
    } else if (res->nameSize > 0) {
        strcpy(info->name, res->name);
    }

    return false;
}
