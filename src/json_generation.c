#include "json_generation.h"

#include <stdio.h>
#include <string.h>

#include "dr_api.h"
#include "drx.h"

#include "error.h"

/**
 * Opens a unique file
 *
 * Returns: The descriptor of the opened file
 */
static file_t open_unique_file();

/**
 * Writes the first line of the JSON file
 *
 * Returns: Whether successful
 */
static bool write_first_line(json_file_t *jsonFile);

/**
 * Writes the last line of the JSON file
 *
 * Returns: Whether successful
 */
static bool write_last_line(json_file_t *jsonFile);

/**
 * Terminates the previous JSON line
 *
 * Returns: Whether successful
 */
static bool terminate_json_line(json_file_t *jsonFile);

/**
 * Writes an entry to the JSON file
 *
 * Returns: Whether successful
 */
static bool write_entry(json_file_t *jsonFile, trace_entry_t entry);

/**
 * Writes line information to the JSON file if it exists
 *
 * Returns: Whether successful
 */
static bool write_line_info(json_file_t *jsonFile, trace_entry_t entry);

/**
 * Writes an operand to the JSON file
 *
 * Returns: Whether successful
 */
static bool write_opnd(json_file_t *jsonFile, opnd_info_t *opndInfo);

/**
 * Write operand debug information to the JSON file
 *
 * Returns: Whether successful
 */
static bool write_debug_info(json_file_t *jsonFile, query_results_t info);

/**
 * Writes the information of a type to the JSON file
 * 
 * Returns: Whether successful
 */
static bool write_type_info(json_file_t *jsonFile, type_t type);

/**
 * Flushes buffer of JSON file
 *
 * Returns: Whether successful
 */
static bool flush_buffer(json_file_t *jsonFile);

/**
 * Appends a string to the buffer of a JSON file
 *
 * Returns: Whether successful
 */
static bool append_buffer(json_file_t *jsonFile, const char *str);

json_file_t open_json_file(void) {
    PRINT_DEBUG("Entered open JSON file");

    json_file_t jsonFile;
    jsonFile.fd = open_unique_file();
    jsonFile.file = fdopen(jsonFile.fd, "w");
    jsonFile.firstLine = true;
    jsonFile.size = 0;

    if (!write_first_line(&jsonFile)) {
        EXIT_FAIL();
    }

    PRINT_DEBUG("Exited open JSON file");
    return jsonFile;
}

void close_json_file(json_file_t jsonFile) {
    PRINT_DEBUG("Entered close JSON file");

    if (!write_last_line(&jsonFile)) {
        EXIT_FAIL();
    }

    if (!flush_buffer(&jsonFile)) {
        EXIT_FAIL();
    }

    if (fclose(jsonFile.file) == EOF) {
        PRINT_ERROR("Could not close JSON file");
        EXIT_FAIL();
    }

    PRINT_DEBUG("Exited close JSON file");
}

bool add_json_entry(json_file_t *jsonFile, trace_entry_t entry) {
    PRINT_DEBUG("Entered add JSON entry");

    bool success = terminate_json_line(jsonFile) &&
                   write_entry(jsonFile, entry);
    if (!success) {
        PRINT_ERROR("Could not write entry to file");
        EXIT_FAIL();
    }

    PRINT_DEBUG("Exited add JSON entry");
}

static file_t open_unique_file() {
    file_t fd = drx_open_unique_file("./", "trace", "log",
                                         DR_FILE_ALLOW_LARGE, NULL, 0);

    if (fd == INVALID_FILE) {
        PRINT_ERROR("Unable to open unique file");
        EXIT_FAIL();
    } 

    return fd;
}

static bool write_first_line(json_file_t *jsonFile) {
    if (!append_buffer(jsonFile, "[\n")) {
        PRINT_ERROR("Could not write first line to JSON output file");
        return false;
    }

    return true;
}

static bool write_last_line(json_file_t *jsonFile)  {
    if (!append_buffer(jsonFile, "\n]")) {
        PRINT_ERROR("Could not write last line to JSON output file");
        return false;
    }

    return true;
}

static bool terminate_json_line(json_file_t *jsonFile) {
    if (!jsonFile->firstLine && !append_buffer(jsonFile, ",\n")) {
        PRINT_ERROR("Could not terminate line");
        return false;
    }

    jsonFile->firstLine = false;
    return true;
}

static bool write_entry(json_file_t *jsonFile, trace_entry_t entry) {
    char buf[128];
    sprintf(buf,
            "{ \"time\": \"%s\", \"tid\": %i, \"pc\": \"%p\", "
            "\"opcode\": { \"name\": \"%s\", \"value\": %i }, ",
            entry.time, entry.tid, entry.pc,
            entry.opcodeName, entry.opcodeValue);

    bool success = append_buffer(jsonFile, buf) &&
                   write_line_info(jsonFile, entry) &&
                   append_buffer(jsonFile, "\"srcs\": [ ");

    if (!success) {
        return false;
    }

    for (int i = 0; i < entry.sizeSrcs; i++) {
        if (i != 0 && !append_buffer(jsonFile, ", ")) {
            return false;
        }

        write_opnd(jsonFile, &entry.srcs[i]);
    }

    if (!append_buffer(jsonFile, " ], \"dsts\": [ ")) {
        return false;
    }
    
    for (int i = 0; i < entry.sizeDsts; i++) {
        if (i != 0 && !append_buffer(jsonFile, ", ")) {
            return false;
        }

        write_opnd(jsonFile, &entry.dsts[i]);
    }

    return append_buffer(jsonFile, " ] }");
}

static bool write_line_info(json_file_t *jsonFile, trace_entry_t entry) {
    if (entry.file == NULL) {
        return true;
    }

    int bufLen = strlen(entry.file) +  64;
    char buf[bufLen];
    sprintf(buf, "\"lineInfo\": { \"file\": \"%s\", \"line\": %lu }, ",
            entry.file, entry.line);
    
    free(entry.file);
    return append_buffer(jsonFile, buf);
}

static bool write_opnd(json_file_t *jsonFile, opnd_info_t *opndInfo) {
    PRINT_DEBUG("Enter write operand");

    char buf[512];
    sprintf(buf,
            "{ \"type\": \"%s\", \"size\": %i, ",
            opndInfo->typeName, opndInfo->size);

    if (!append_buffer(jsonFile, buf)) {
        return false;
    }

    switch (opndInfo->type) {
    case immedInt:
        sprintf(buf, "\"value\": \"0x%lx\"", opndInfo->info.immedInt);
        break;

    case immedFloat:
        sprintf(buf, "\"value\": %f", opndInfo->info.immedFloat);
        break;

    case pc:
        sprintf(buf, "\"value\": %p", opndInfo->info.pc);
        break;

    case reg:
        sprintf(buf,
                "\"register\": { \"name\": \"%s\", \"value\": \"0x%s\" }",
                opndInfo->info.reg.reg, opndInfo->info.reg.val);
        break;
    
    case memRef:
        mem_ref_info_t *memRef = &opndInfo->info.memRef;
        sprintf(buf,
                "\"reference\": { \"type\": \"%s\", \"isFar\": %s, "
                "\"addr\": \"%p\"",
                memRef->type, memRef->isFar, memRef->addr);

        if (memRef->outputVal) {
            sprintf(buf + strlen(buf), ", \"value\": \"0x%lx\"", memRef->val);
        }

        if (memRef->outputBaseDisp) {
            sprintf(buf + strlen(buf),
                    ", \"base\": { \"name\": \"%s\", \"value\": \"0x%lx\" }"
                    ", \"index\": { \"name\": \"%s\", \"value\": \"0x%lx\" }"
                    ", \"scale\": %i, \"disp\": %i",
                    memRef->base, memRef->baseVal, memRef->index,
                    memRef->indexVal, memRef->scale, memRef->disp);
        }
        break;
    }

    if (opndInfo->debugInfo.sizeResults == 0) {
        strcat(buf, " }");
        return append_buffer(jsonFile, buf);
    }

    strcat(buf, ", \"debugInfo\": ");
    bool success = append_buffer(jsonFile, buf) &&
                   write_debug_info(jsonFile, opndInfo->debugInfo);
    deinit_query_results(opndInfo->debugInfo);
    if (!success) {
        return false;
    }

    PRINT_DEBUG("Exit write operand");
    return append_buffer(jsonFile, " }");
}

static bool write_debug_info(json_file_t *jsonFile, query_results_t info) {
    PRINT_DEBUG("Enter write debug info");

    if (!append_buffer(jsonFile, "[ ")) {
        return false;
    }

    for (int i = 0; i < info.sizeResults; i++) {
        if (i != 0 && !append_buffer(jsonFile, ", ")) {
            return false;
        }

        size_t bufLen = strlen(info.results[i].name) + 64;
        char buf[bufLen];

        switch (info.results[i].type) {
        case function:
            sprintf(buf,
                    "{ \"type\": \"function\", \"name\": \"%s\"",
                    info.results[i].name);
            break;

        case variable:
            sprintf(buf,
                    "{ \"type\": \"variable\", \"name\": \"%s\", "
                    "\"isLocal\": %s",
                    info.results[i].name,
                    info.results[i].isLocal ? "true" : "false");
            break;
        }

        bool success = append_buffer(jsonFile, buf) &&
                       (info.results[i].type == function ||
                       (append_buffer(jsonFile, ", \"valueType\": ") &&
                        write_type_info(jsonFile, info.results[i].valType))) &&
                       append_buffer(jsonFile, " }");

        if (!success) {
            PRINT_ERROR("Could not append debug information to entry");
            return false;
        }
    }

    bool success = append_buffer(jsonFile, " ]");

    PRINT_DEBUG("Exit write debug info");
    return success;
}

static bool write_type_info(json_file_t *jsonFile, type_t type) {
    bool success = append_buffer(jsonFile, "{ \"name\": \"") &&
                   append_buffer(jsonFile, type.name) &&
                   append_buffer(jsonFile, "\", \"compound\": [ ");

    if (!success) {
        PRINT_ERROR("Could not write type info name to entry");
        return false;
    }

    for (int i = 0; i < type.compoundSize; i++) {
        type_compound_t compound = type.compound[i];
        success = (i == 0 || append_buffer(jsonFile, ", ")) &&
                  append_buffer(jsonFile, "\"") &&
                  append_buffer(jsonFile, get_type_compound_name(compound)) &&
                  append_buffer(jsonFile, "\"");

        if (!success) {
            PRINT_ERROR("Could not write type info compound to entry");
            return false;
        }
    }

    success = append_buffer(jsonFile, " ] }");
    if (!success) {
        PRINT_ERROR("Could not terminate type info in entry");
    }

    return success;
}

static bool flush_buffer(json_file_t *jsonFile) {
    PRINT_DEBUG("Entering flush JSON file buffer");

    size_t res = fwrite(jsonFile->buf, sizeof(jsonFile->buf[0]),
                        jsonFile->size, jsonFile->file);
    if (res != jsonFile->size) {
        PRINT_ERROR("Could not flush JSON file buffer");
        return false;
    }

    jsonFile->size = 0;

    PRINT_DEBUG("Exiting flush JSON file buffer");
    return true;
}

static bool append_buffer(json_file_t *jsonFile, const char *str) {
    size_t len = strnlen(str, BUF_LEN);

    bool success = len != BUF_LEN && (jsonFile->size + len < BUF_LEN ||
                                      flush_buffer(jsonFile));
    if (!success) {
        PRINT_ERROR("Could not append to JSON file buffer");
        return false;
    }

    memcpy(jsonFile->buf + jsonFile->size, str, len);
    jsonFile->size += len;
    return true;
}
