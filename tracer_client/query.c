#include "query.h"

#include "dr_api.h"
#include "drsyms.h"

#include "module_debug_info.h"
#include "module_set.h"
#include "error.h"

/**
 * Returns a blank dr_sym_info_t
 */
static drsym_info_t get_blank_info(void);

/**
 * Allocates space ever increasing space for a file name in a dr_sym_info_t
 *
 * Returns: Whether successful
 */
static bool alloc_file_str(drsym_info_t *info);

/**
 * Attempts to get the line information, indicating if a bigger file name
 * buffer is needed
 *
 * Returns: Whether to retry with a bigger buffer
 */
static bool get_line_info(char *path, size_t offset, drsym_info_t *info);

bool query_addr(void *addr, void *pc, void *bp, query_results_t *results) {
    PRINT_DEBUG("Enter query address");
    
    *results = blank_query_results();

    module_debug_t info;
    if (!find_module(pc, &info)) {
        return true;
    }

    query_t query;
    query.addr = addr;
    query.pc = pc;
    query.sp = bp + 16;
    query.moduleBase = info.start;

    bool success = module_query(info, query, results);

    PRINT_DEBUG("Exit query address");
    return success;
}

void deinit_query_results(query_results_t results) {
    PRINT_DEBUG("Enter deinit query results");

    if (results.results != NULL) {
        free(results.results);
    }

    PRINT_DEBUG("Exit deinit query results");
}

query_results_t blank_query_results(void) {
    query_results_t results;
    results.results = NULL;
    results.sizeResults = 0;
    results.capacityResults = 0;

    return results;
}

bool query_line(void *pc, char **file, uint64_t *line) {
    PRINT_DEBUG("Enter query line");

    *file = NULL;
    *line = 0;

    size_t offset;
    module_data_t *module = dr_lookup_module(pc);
    if (module == NULL) {
        PRINT_ERROR("Could not open module in query line");
        return false;
    }
    offset = pc - (void *)module->start;

    drsym_info_t lineInfo = get_blank_info();
    lineInfo.file_size = 32 * sizeof(lineInfo.file[0]);

    bool retry;
    do {
        retry = alloc_file_str(&lineInfo) &&
                get_line_info(module->full_path, offset, &lineInfo);
    } while (retry);

    *file = lineInfo.file;
    *line = lineInfo.line;

    dr_free_module_data(module);

    PRINT_DEBUG("Exit query line");
    return true;
}

static drsym_info_t get_blank_info(void) {
    drsym_info_t info;
    info.struct_size = sizeof(info);
    info.name = NULL;
    info.name_size = 0;
    info.file = NULL;
    info.file_size = 0;

    return info;
}

static bool alloc_file_str(drsym_info_t *info) {
    info->file_size *= 2;
    if (info->file != NULL) {
        free(info->file);
    }

    info->file = malloc(info->file_size);
    if (info->file == NULL) {
        PRINT_ERROR("Unable to allocate new space for file name buffer");
    }

    return info->file != NULL;
}

static bool get_line_info(char *path, size_t offset, drsym_info_t *info) {
    drsym_error_t err = drsym_lookup_address(path, offset, info,
                                             DRSYM_DEFAULT_FLAGS);

    if (err != DRSYM_SUCCESS) {
        free(info->file);
        info->file = NULL;
        info->line = 0;
        return false;
    }

    return info->file_size == info->file_available_size + 1;
}
