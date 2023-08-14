#ifndef DEBUG_QUERY_H
#define DEBUG_QUERY_H

#include <stdint.h>

#include "dr_api.h"

typedef struct query {
    void *addr, *pc, *sp, *moduleBase;
} query_t;

typedef enum type_compound {
    constType,
    packedType,
    pointerType,
    lvalReferenceType,
    restrictType,
    rvalReferenceType,
    sharedType,
    volatileType,
    arrayType
} type_compound_t;

typedef struct type {
    type_compound_t *compound;
    int compoundSize, compoundCapacity;

    const char *name;
} type_t;

typedef struct query_result {
    enum {
        function,
        variable
    } type;

    bool deallocName;
    char *name;
    void *address;
    bool isLocal;
    type_t valType;
} query_result_t;

typedef struct query_results {
    query_result_t *results;
    int sizeResults, capacityResults;
} query_results_t;

/**
 * Performs a query of an address for debug information across all modules
 *
 * Returns: Whether successful
 */
bool query_addr(void *addr, void *pc, void *bp, query_results_t *results);

/**
 * Deinitialises given debug query results
 */
void deinit_query_results(query_results_t results);

/**
 * Returns blank query results
 */
query_results_t blank_query_results(void);

/**
 * Gets file and line debug information for an instruction
 *
 * Returns: Whether successful
 */
bool query_line(void *pc, char **file, uint64_t *line, bool persistent);

/**
 * Gets module and source file information for an instruction
 *
 * Returns: Whether no error has occurred
 */
bool query_file(void *pc, char *modulePath, char *sourcePath, int bufSize);

/**
 * Gets function name information for a possible PC
 *
 * Returns: Whether successful
 */
bool query_function(void *pc, char **functionName);

/**
 * Returns the name for a type compound, or NULL on error
 */
const char *get_type_compound_name(type_compound_t compound);

#endif
