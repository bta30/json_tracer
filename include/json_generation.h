#ifndef JSON_GENERATION_H
#define JSON_GENERATION_H

#include "trace_entry.h"

#include "dr_api.h"

#define JSON_BUF_LEN ((uint64)1 << 30)

typedef struct json_file {
    file_t fd;
    FILE *file;
    bool firstLine;
    char *buf;
    int size;
} json_file_t;

/**
 * Open a unique file to output JSON with a given prefix
 *
 * Returns: The file opened
 */
json_file_t open_json_file(const char *prefix);

/**
 * Closes a given JSON file
 *
 * Returns: Whether successful
 */
void close_json_file(json_file_t jsonFile);

/**
 * Adds an entry to the JSON file
 *
 * Returns: Whether successful
 */
bool add_json_entry(json_file_t *jsonFile, trace_entry_t entry);

#endif
