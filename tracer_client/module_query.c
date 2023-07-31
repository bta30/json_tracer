#include "module_debug_info.h"

#include "error.h"

#define MIN_CAPACITY 4

typedef struct query_info {
    query_t query;
    module_debug_t info;
} query_info_t;

/**
 * Initialises empty debug results
 *
 * Returns: Whether successful
 */
static bool init_results(query_results_t *results);

/**
 * Adds a result to the stored results
 *
 * Returns: Whether successful
 */
static bool add_result(query_results_t *results, query_result_t result);

/**
 * Queries an entry_t for a compilation unit root, appending any results
 *
 * Returns: Whether successful
 */
static bool cu_query(query_info_t info, size_t cu, query_results_t *results);

/**
 * Queries a variable entry, assuming it is currently in scope
 *
 * Returns: Whether successful
 */
static bool var_query(query_info_t info, size_t var, query_results_t *results);

/**
 * Queries a function entry
 *
 * Returns: Whether successful
 */
static bool func_query(query_info_t info, size_t func, query_results_t *results);

bool module_query(module_debug_t info, query_t query, query_results_t *res) {
    PRINT_DEBUG("Enter module query");

    query.addr -= (size_t)query.moduleBase;
    query.pc -= (size_t)query.moduleBase;
    query.sp -= (size_t)query.moduleBase;

    query_info_t queryInfo;
    queryInfo.query = query;
    queryInfo.info = info;

    if (!init_results(res)) {
        return false;
    }

    for (int i = 0; i < info.sizeRoots; i++) {
        if (!cu_query(queryInfo, info.roots[i], res)) {
            return false;
        }
    }

    PRINT_DEBUG("Exit module query");
    return true;
}

static bool init_results(query_results_t *results) {
    if (results->results != NULL) {
        return true;
    }

    results->results = malloc(sizeof(results->results[0]) * MIN_CAPACITY);
    results->sizeResults = 0;
    results->capacityResults = MIN_CAPACITY;

    if (results->results == NULL) {
        PRINT_ERROR("Could not allocate space for module query results\n");
        return false;
    }

    return true;
}

static bool add_result(query_results_t *results, query_result_t result) {
    if (results->sizeResults == results->capacityResults) {
        results->capacityResults *= 2;
        results->results = reallocarray(results->results,
                                        results->capacityResults,
                                        sizeof(results->results[0]));

        if (results->results == NULL) {
            PRINT_ERROR("Could not further allocate space for module query "
                        "results");
            return false;
        }
    }

    results->results[results->sizeResults++] = result;
    return true;
}

static bool cu_query(query_info_t info, size_t cu, query_results_t *results) {
    entry_t *entry = &info.info.entries[cu];

    for (int i = 0; i < entry->sizeChildren; i++) {
        Dwarf_Half childTag = info.info.entries[entry->children[i]].tag;
        bool success = (childTag != DW_TAG_variable || 
                         var_query(info, entry->children[i], results)) &&
                       (childTag != DW_TAG_subprogram ||
                         func_query(info, entry->children[i], results));
        if (!success) {
            return false;
        }
    }

    return true;
}

static bool var_query(query_info_t info, size_t var, query_results_t *results) {
    entry_t *entry = &info.info.entries[var];

    if (entry->name == NULL) {
        return true;
    }

    if (entry->type == -1) {
        return true;
    }

    size_t size = info.info.entries[entry->type].byteSize;

    if (!entry->hasAddr && !entry->hasFbregOffset) {
        // External variable
        return true;
    }

    void *minAddr = entry->hasAddr ? entry->addr :
                                     info.query.sp + entry->fbregOffset;
    void *maxAddr = minAddr + size;

    query_result_t result;
    if (info.query.addr < minAddr || info.query.addr >= maxAddr) {
        return true;
    }

    result.type = variable;
    result.name = entry->name;
    result.isLocal = entry->hasFbregOffset;

    return add_result(results, result);
}

static bool func_query(query_info_t info, size_t func, query_results_t *results) {
    entry_t *entry = &info.info.entries[func];

    if (entry->name == NULL) {
        return true;
    }

    if (!entry->hasLowPC) {
        return true;
    }

    if (!entry->hasHighPC) {
        return true;
    }

    if (info.query.addr == entry->lowPC) {
        query_result_t result;
        result.type = function;
        result.name = entry->name;

        if (!add_result(results, result)) {
            return false;
        }
    }

    if (info.query.pc < entry->lowPC || info.query.pc > entry->highPC) {
        return true;
    }

    for (int i = 0; i < entry->sizeChildren; i++) {
        Dwarf_Half childTag = info.info.entries[entry->children[i]].tag;
        
        bool validTag = childTag == DW_TAG_variable ||
                        childTag == DW_TAG_formal_parameter;
        if (!validTag) {
            continue;
        }

        if (!var_query(info, entry->children[i], results)) {
            PRINT_ERROR("Failed local variable query");
            return false;
        }
    }
    
    return true;
}
