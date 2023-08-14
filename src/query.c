#include "query.h"

#include <string.h>

#include "dr_api.h"
#include "drsyms.h"

#include "module_debug_info.h"
#include "module_set.h"
#include "address_lookup.h"
#include "error.h"

/**
 * Returns a blank dr_sym_info_t
 */
static drsym_info_t get_blank_info(void);

/**
 * Allocates space ever increasing space for the strings in a dr_sym_info_t
 *
 * Returns: Whether successful
 */
static bool alloc_info_strs(drsym_info_t *info);

bool query_addr(void *addr, void *pc, void *bp, query_results_t *results) {
    PRINT_DEBUG("Enter query address");
    
    module_debug_t info;
    if (!find_module(pc, &info)) {
        *results = blank_query_results();
        PRINT_DEBUG("Early exit query address");
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

    if (results.results == NULL) {
        return;
    }

    for (int i = 0; i < results.sizeResults; i++) {
        query_result_t *res = &results.results[i];
        if (res->type == variable && res->valType.compound != NULL) {
            free(res->valType.compound);
        }

        if (res->deallocName) {
            free(res->name);
        }
    }

    free(results.results);

    PRINT_DEBUG("Exit deinit query results");
}

query_results_t blank_query_results(void) {
    query_results_t results;
    results.results = NULL;
    results.sizeResults = 0;
    results.capacityResults = 0;

    return results;
}

bool query_line(void *pc, char **file, uint64_t *line, bool persistent) {
    PRINT_DEBUG("Enter query line");

    *file = NULL;
    *line = 0;

    module_data_t *module = dr_lookup_module(pc);
    if (module == NULL) {
        PRINT_ERROR("Could not open module in query line");
        return false;
    }
    size_t offset = pc - (void *)module->start;

    drsym_info_t lineInfo = get_blank_info();

    bool retry;
    do {
        retry = alloc_info_strs(&lineInfo) &&
                lookup_address(pc, &lineInfo, persistent);
    } while (retry);

    if (lineInfo.name != NULL) {
        free(lineInfo.name);
    }

    if (lineInfo.file_available_size == 0) {
        free(lineInfo.file);
        lineInfo.file = NULL;
    }

    *file = lineInfo.file;
    *line = lineInfo.line;

    dr_free_module_data(module);

    PRINT_DEBUG("Exit query line");
    return true;
}

bool query_file(void *pc, char *modulePath, char *sourcePath, int bufSize) {
    PRINT_DEBUG("Enter query file");

    module_data_t *module = dr_lookup_module(pc);
    if (module == NULL) {
        PRINT_ERROR("Could not open module in query file");
        return false;
    }
    strncpy(modulePath, module->full_path, bufSize);

    char *src;
    uint64_t line;
    bool success = query_line(pc, &src, &line, false);
    if (!success) {
        return false;
    }

    if (src == NULL) {
        sourcePath[0] = '\0';
    } else {
        strncpy(sourcePath, src, bufSize);
        free(src);
    }

    PRINT_DEBUG("Exit query file");
    return true;
}

bool query_function(void *pc, char **functionName) {
    PRINT_DEBUG("Enter query function");

    *functionName = NULL;

    module_data_t *module = dr_lookup_module(pc);
    if (module == NULL) {
        return true;
    }
    size_t offset = pc - (void *)module->start;

    drsym_info_t funcInfo = get_blank_info();

    bool retry ;
    do {
        retry = alloc_info_strs(&funcInfo) &&
                lookup_address(pc, &funcInfo, false);
    } while (retry);
    
    if (funcInfo.file != NULL) {
        free(funcInfo.file);
    }

    *functionName = funcInfo.name;

    dr_free_module_data(module);

    PRINT_DEBUG("Exit query function");
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

static bool alloc_info_strs(drsym_info_t *info) {
    if (info->file_size == 0) {
        info->file_size = 256 * sizeof(info->file[0]);
    }

    if (info->name_size == 0) {
        info->name_size = 256 * sizeof(info->name[0]);
    }

    info->file_size *= 2;
    if (info->file != NULL) {
        free(info->file);
    }

    info->name_size *= 2;
    if (info->name != NULL) {
        free(info->name);
    }

    info->file = malloc(info->file_size);
    info->name = malloc(info->name_size);
    if (info->file == NULL || info->name == NULL) {
        PRINT_ERROR("Unable to allocate new space for info buffers");
    }

    return info->file != NULL && info->name != NULL;
}

const char *get_type_compound_name(type_compound_t compound) {
    switch (compound) {
        case constType:
            return "const";

        case pointerType:
            return "pointer";

        case lvalReferenceType:
            return "lvalueReference";

        case restrictType:
            return "restrict";

        case rvalReferenceType:
            return "rvalueReference";

        case sharedType:
            return "shared";

        case volatileType:
            return "volatile";

        case arrayType:
            return "array";

        default:
            return NULL;
    }
}
