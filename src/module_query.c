#include "module_debug_info.h"

#include <string.h>

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

/**
 * Queries a namespace
 *
 * Returns: Whether successful
 */
static bool ns_query(query_info_t info, size_t ns, query_results_t *results);

/**
 * Retrieves the full type for the given entry
 * 
 * Returns: Whether successful
 */
static bool type_query(query_info_t info, size_t entryIndex, type_t *type);

/**
 * Gets the type compound for a DW_TAG
 *
 * Returns: Whether successful
 */
static bool get_type_compound(Dwarf_Half tag, type_compound_t *compound);

/**
 * Appends a new compound for a type
 *
 * Returns: Whether successful
 */
static bool append_type_compound(type_compound_t compound, type_t *type);

/**
 * Adds namespace results, given the namespace's name, to the final results
 *
 * Returns: Whether successful
 */
static bool add_ns_res(char *name, query_results_t nsRes, query_results_t *res);

bool module_query(module_debug_t info, query_t query, query_results_t *res) {
    PRINT_DEBUG("Enter module query");

    if (!init_results(res)) {
        return false;
    }

    query.addr -= (size_t)query.moduleBase;
    query.pc -= (size_t)query.moduleBase;
    query.sp -= (size_t)query.moduleBase;

    query_info_t queryInfo;
    queryInfo.query = query;
    queryInfo.info = info;

    for (int i = 0; i < info.sizeRoots; i++) {
        if (!cu_query(queryInfo, info.roots[i], res)) {
            return false;
        }
    }

    if (res->sizeResults != 0) {
        PRINT_DEBUG("Exit module query early");
        return true;
    }

    char *funcName;
    void *addr = (size_t)query.addr + query.moduleBase;
    bool success = true;
    if (query_function(addr, &funcName) && funcName != NULL) {
        query_result_t result;
        result.type = function;
        result.deallocName = true;
        result.name = funcName;
        result.address = addr;

        success = add_result(res, result);
    }

    PRINT_DEBUG("Exit module query");
    return success;
}

static bool init_results(query_results_t *results) {
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
                         func_query(info, entry->children[i], results)) &&
                       (childTag != DW_TAG_namespace ||
                         ns_query(info, entry->children[i], results)) &&
                       (childTag != DW_TAG_class_type ||
                         ns_query(info, entry->children[i], results)) &&
                       (childTag != DW_TAG_structure_type ||
                         ns_query(info, entry->children[i], results));
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

    if (!entry->hasType) {
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
    result.deallocName = false;
    result.name = entry->name;
    result.address = info.query.moduleBase + (size_t)minAddr;
    result.isLocal = entry->hasFbregOffset;

    return type_query(info, entry->type, &result.valType) &&
           add_result(results, result);
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
        result.deallocName = false;
        result.name = entry->name;
        result.address = info.query.moduleBase + (size_t) entry->lowPC;

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

static bool ns_query(query_info_t info, size_t ns, query_results_t *results) {
    if (info.info.entries[ns].name == NULL) {
        return true;
    }

    query_results_t nsResults;

    return init_results(&nsResults) &&
           cu_query(info, ns, &nsResults) &&
           add_ns_res(info.info.entries[ns].name, nsResults, results);
}

static bool type_query(query_info_t info, size_t entryIndex, type_t *type) {
    type->compound = NULL;
    type->compoundSize = 0;
    type->compoundCapacity = 0;
    type->name = NULL;

    entry_t *entry = &info.info.entries[entryIndex];

    while (entry != NULL) {
        switch (entry->tag) {
            case DW_TAG_base_type:
            case DW_TAG_typedef:
            case DW_TAG_union_type:
            case DW_TAG_class_type:
            case DW_TAG_interface_type:
            case DW_TAG_enumeration_type:
                type->name = entry->name;
                entry = NULL;
                break;

            case DW_TAG_const_type:
            case DW_TAG_packed_type:
            case DW_TAG_pointer_type:
            case DW_TAG_reference_type:
            case DW_TAG_restrict_type:
            case DW_TAG_rvalue_reference_type:
            case DW_TAG_shared_type:
            case DW_TAG_volatile_type:
            case DW_TAG_array_type:
                type_compound_t compound;
                bool success = get_type_compound(entry->tag, &compound) &&
                               append_type_compound(compound, type);
                if (!success) {
                    return false;
                }
                if (entry->hasType) {
                    entry = &info.info.entries[entry->type];
                } else {
                    type->name = "void";
                    entry = NULL;
                }
                break;

            default:
                PRINT_ERROR("Could not determine type in type query");
                return false;
        }
    }

    return true;
}

static bool get_type_compound(Dwarf_Half tag, type_compound_t *compound) {
    switch (tag) {
        case DW_TAG_const_type:
            *compound = constType;
            return true;

        case DW_TAG_packed_type:
            *compound = packedType;
            return true;

        case DW_TAG_pointer_type:
            *compound = pointerType;
            return true;

        case DW_TAG_reference_type:
            *compound = lvalReferenceType;
            return true;

        case DW_TAG_restrict_type:
            *compound = restrictType;
            return true;

        case DW_TAG_rvalue_reference_type:
            *compound = rvalReferenceType;
            return true;

        case DW_TAG_shared_type:
            *compound = sharedType;
            return true;

        case DW_TAG_volatile_type:
            *compound = volatileType;
            return true;

        case DW_TAG_array_type:
            *compound = arrayType;
            return true;

        default:
            PRINT_ERROR("Given invalid compound type");
            return false;
    }
}

static bool append_type_compound(type_compound_t compound, type_t *type) {
    if (type->compound == NULL) {
        type->compound = malloc(MIN_CAPACITY * sizeof(type->compound[0]));
        type->compoundCapacity = MIN_CAPACITY;

        if (type->compound == NULL) {
            PRINT_ERROR("Could not allocate space for type compound");
            return false;
        }
    }

    if (type->compoundSize == type->compoundCapacity) {
        type->compoundCapacity *= 2;
        type->compound = reallocarray(type->compound, type->compoundCapacity,
                                      sizeof(type->compound[0]));
        if (type->compound == NULL) {
            PRINT_ERROR("Could not allocate further space for type compound");
            return false;
        }
    }

    type->compound[type->compoundSize++] = compound;
    return true;
}

static bool add_ns_res(char *name, query_results_t nsRes, query_results_t *res) {
    if (nsRes.results == NULL) {
        return true;
    }

    int nsNameLen = strlen(name);
    for (int i = 0; i < nsRes.sizeResults; i++) {
        query_result_t currRes = nsRes.results[i];

        int nameSize = nsNameLen + 2 + strlen(currRes.name) + 1;

        currRes.deallocName = true;
        
        char *newName = malloc(nameSize * sizeof(currRes.name[0]));
        if (newName == NULL) {
            PRINT_ERROR("Could not allocate space for new name\n");
        }

        strcpy(newName, name);
        strcat(newName, "::");
        strcat(newName, currRes.name);

        if (strcmp(newName, "std::mutex::lock") == 0)  {
            printf("FOUND LOCK!\n");
        }

        currRes.name = newName;

        if (!add_result(res, currRes)) {
            return false;
        }

        if (nsRes.results[i].deallocName) {
            free(nsRes.results[i].name);
        }
    }

    free(nsRes.results);
    return true;
}
