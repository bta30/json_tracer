#include "extra_debug_info.h"

#include <string.h>

#include "drsyms.h"

#include "error.h"

#define MIN_CAPACITY 128

typedef struct source_debug_info {
    const char *sourcePath;
    uint64 *lines;
    int sizeLines, capacityLines;
} source_debug_info_t;

typedef struct module_debug_info {
    bool success;
    const char *path;
    source_debug_info_t *sources;
    int sizeSources, capacitySources;
} module_debug_info_t;

/**
 * Callback when enumerating over lines in a module for writing a new line to
 * the module's debugging information
 *
 * Returns: Whether to continue enumerating
 */
static bool save_line(drsym_line_info_t *line, module_debug_info_t *module);

/**
 * Initialises a module_debug_info_t
 *
 * Returns: Whether successful
 */
static bool init_module_debug_info(module_debug_info_t *info);

/**
 * Deinitialises a module_debug_info_t
 */
static void deinit_module_debug_info(module_debug_info_t info);

/**
 * Initialises a source_debug_info_t
 *
 * Returns: Whether successful
 */
static bool init_source_debug_info(source_debug_info_t *info);

/**
 * Deinitialises a source_debug_info_t
 */
static void deinit_source_debug_info(source_debug_info_t info);

/**
 * Finds a source file in a module_debug_info_t given its path
 *
 * Returns: A pointer to the source file's source_debug_info_t on success,
 *          otherwise NULL
 */
static source_debug_info_t *find_source(module_debug_info_t *info, const char *path);

/**
 * Writes a line for the given module to the output file
 */
static void write_module_debug_json(debug_file_t *file, module_debug_info_t info);

bool open_debug_file(const char *path, debug_file_t *file) {
    PRINT_DEBUG("Enter open debug file");

    file->file = fopen(path, "w");
    if (file->file == NULL) {
        return false;
    }

    file->firstLine = true;
    fprintf(file->file, "[\n");

    PRINT_DEBUG("Exit open debug file");
    return true;
}

void close_debug_file(debug_file_t file) {
    PRINT_DEBUG("Enter close debug file");

    fprintf(file.file, "\n]\n");
    fflush(file.file);

    if (fclose(file.file) == EOF) {
        PRINT_ERROR("Could not close debug file");
        EXIT_FAIL();
    }
    
    PRINT_DEBUG("Exit close debug file");
}

bool write_module_debug_info(debug_file_t *file, const char *modulePath) {
    PRINT_DEBUG("Enter write module debug info");

    if (drsym_module_has_symbols(modulePath) != DRSYM_SUCCESS) {
        PRINT_DEBUG("Exit early write module debug info");
        return true;
    }

    module_debug_info_t debugInfo;
    if (!init_module_debug_info(&debugInfo)) {
        PRINT_ERROR("Could not initialise module debug info");
        return false;
    }
    debugInfo.path = modulePath;
    
    drsym_error_t err = drsym_enumerate_lines(modulePath,
                                              (drsym_enumerate_lines_cb)save_line,
                                              &debugInfo);

    if (err != DRSYM_SUCCESS || !debugInfo.success) {
        deinit_module_debug_info(debugInfo);
        return true;
    }
    
    write_module_debug_json(file, debugInfo);
    deinit_module_debug_info(debugInfo);

    PRINT_DEBUG("Exit write module debug info");
    return true;
}

static bool save_line(drsym_line_info_t *line, module_debug_info_t *module) {
    PRINT_DEBUG("Entering save line");

    if (line->file == NULL) {
        PRINT_DEBUG("Exit save line early");
        return true;
    }
    
    source_debug_info_t *source = find_source(module, line->file);
    if (source == NULL) {
        module->success = false;
        PRINT_ERROR("Could not get source for new line");
        return false;
    }

    for (int i = 0; i < source->sizeLines; i++) {
        if (source->lines[i] == line->line) {
            PRINT_DEBUG("Exit save line early from duplicate");
            return true;
        }
    }

    if (source->sizeLines == source->capacityLines) {
        source->capacityLines *= 2;
        uint64 *newLines = reallocarray(source->lines, source->capacityLines,
                                        sizeof(source->lines[0]));

        if (newLines == NULL) {
            module->success = false;
            PRINT_ERROR("Could not allocate further space for source_debug_info_t");
            return false;
        }

        source->lines = newLines;
    }

    source->lines[source->sizeLines++] = line->line;

    PRINT_DEBUG("Exiting save line");
    return true;
}

static bool init_module_debug_info(module_debug_info_t *info) {
    info->sources = malloc(MIN_CAPACITY * sizeof(info->sources[0]));
    if (info->sources == NULL) {
        PRINT_ERROR("Could not allocate space for module_debug_info_t");
        info->success = false;
        return false;
    }

    info->success = true;
    info->sizeSources = 0;
    info->capacitySources = MIN_CAPACITY;
}

static void deinit_module_debug_info(module_debug_info_t info) {
    if (info.sources == NULL) {
        return;
    }

    for (int i = 0; i < info.sizeSources; i++) {
        deinit_source_debug_info(info.sources[i]);
    }

    free(info.sources);
}

static bool init_source_debug_info(source_debug_info_t *info) {
    info->lines = malloc(MIN_CAPACITY * sizeof(info->lines[0]));
    if (info->lines == NULL) {
        PRINT_ERROR("Could not allocate space for source_debug_info_t");
        return false;
    }

    info->sizeLines = 0;
    info->capacityLines = MIN_CAPACITY;
    return true;
}

static void deinit_source_debug_info(source_debug_info_t info) {
    if (info.lines != NULL) {
        free(info.lines);
    }
}

static source_debug_info_t *find_source(module_debug_info_t *info, const char *path) {
    PRINT_DEBUG("Entering find source");

    for (int i = 0; i < info->sizeSources; i++) {
        if (strcmp(path, info->sources[i].sourcePath) == 0) {
            PRINT_DEBUG("Early exit find source");
            return &info->sources[i];
        }
    }

    if (info->sizeSources == info->capacitySources) {
        info->capacitySources *= 2;
        source_debug_info_t *sources = reallocarray(info->sources,
                                                    info->capacitySources,
                                                    sizeof(info->sources[0]));
        if (sources == NULL) {
            info->success = false;
            PRINT_ERROR("Could not allocate further space for module_debug_info_t");
            return NULL;
        }

        info->sources = sources;
    }

    source_debug_info_t *source = &info->sources[info->sizeSources];
    if (!init_source_debug_info(source)) {
        info->success = false;
        return NULL;
    }

    info->sizeSources++;
    source->sourcePath = path;

    PRINT_DEBUG("Exit find source");
    return source;
}

static void write_module_debug_json(debug_file_t *file, module_debug_info_t info) {
    if (info.sizeSources == 0) {
        return;
    }

    if (!file->firstLine) {
        fprintf(file->file, ",\n");
    }
    file->firstLine = false;

    fprintf(file->file, "{ \"path\": \"%s\", \"sourceFiles\": [ ", info.path);

    for (int i = 0; i < info.sizeSources; i++) {
        if (i != 0) {
            fprintf(file->file, ", ");
        }

        fprintf(file->file, "{ \"path\": \"%s\", \"lines\": [ ", info.sources[i].sourcePath);

        for (int j = 0; j < info.sources[i].sizeLines; j++) {
            if (j != 0) {
                fprintf(file->file, ", ");
            }

            fprintf(file->file, "%lu", info.sources[i].lines[j]);
        }

        fprintf(file->file, " ] }");
    }

    fprintf(file->file, " ] }");
}
