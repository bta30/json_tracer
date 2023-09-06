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

typedef struct ns_info {
    char *buf;
    int capacity;
    bool firstFunc;
    size_t entry;
} ns_info_t;

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
 *
 * Returns: Whether successful
 */
static bool write_module_debug_json(debug_file_t *file,
                                    module_debug_t *fullInfo,
                                    module_debug_info_t info);

/**
 * Writes function information to the output file for some module
 *
 * Returns: Whether successful
 */
static bool write_funcs(debug_file_t *file, module_debug_t *info);

/**
 * Uses a depth-first search over namespaces to write function names
 *
 * Returns: Whether successful
 */
static bool write_funcs_dfs(debug_file_t *file, module_debug_t *info, ns_info_t *ns);

bool open_debug_file(const char *path, debug_file_t *file) {
    PRINT_DEBUG("Enter open debug file");

    file->fd = dr_open_file(path, DR_FILE_ALLOW_LARGE);
    if (file->fd == INVALID_FILE) {
        return false;
    }

    file->firstLine = true;
    dr_fprintf(file->fd, "[\n");

    PRINT_DEBUG("Exit open debug file");
    return true;
}

void close_debug_file(debug_file_t file) {
    PRINT_DEBUG("Enter close debug file");

    dr_fprintf(file.fd, "\n]\n");
    dr_flush_file(file.fd);
    dr_close_file(file.fd);

    PRINT_DEBUG("Exit close debug file");
}

bool write_module_debug_info(debug_file_t *file, module_debug_t *info) {
    PRINT_DEBUG("Enter write module debug info");

    if (drsym_module_has_symbols(info->path) != DRSYM_SUCCESS) {
        PRINT_DEBUG("Exit early write module debug info");
        return true;
    }

    module_debug_info_t debugInfo;
    if (!init_module_debug_info(&debugInfo)) {
        PRINT_ERROR("Could not initialise module debug info");
        return false;
    }
    debugInfo.path = info->path;
    
    drsym_error_t err = drsym_enumerate_lines(info->path,
                                              (drsym_enumerate_lines_cb)save_line,
                                              &debugInfo);

    if (err != DRSYM_SUCCESS || !debugInfo.success) {
        deinit_module_debug_info(debugInfo);
        return true;
    }
    
    bool success = write_module_debug_json(file, info, debugInfo) &&
                   drsym_free_resources(info->path) == DRSYM_SUCCESS;
    deinit_module_debug_info(debugInfo);

    PRINT_DEBUG("Exit write module debug info");
    return success;
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
        uint64 *newLines = realloc(source->lines, source->capacityLines *
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
    return true;
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
        source_debug_info_t *sources = realloc(info->sources,
                                               info->capacitySources *
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

static bool write_module_debug_json(debug_file_t *file,
                                    module_debug_t *fullInfo,
                                    module_debug_info_t info)
{
    if (info.sizeSources == 0) {
        return true;
    }

    if (!file->firstLine) {
        dr_fprintf(file->fd, ",\n");
    }
    file->firstLine = false;

    dr_fprintf(file->fd, "{ \"path\": \"%s\", \"sourceFiles\": [ ", info.path);

    for (int i = 0; i < info.sizeSources; i++) {
        if (i != 0) {
            dr_fprintf(file->fd, ", ");
        }

        dr_fprintf(file->fd, "{ \"path\": \"%s\", \"lines\": [ ", info.sources[i].sourcePath);

        for (int j = 0; j < info.sources[i].sizeLines; j++) {
            if (j != 0) {
                dr_fprintf(file->fd, ", ");
            }

            dr_fprintf(file->fd, "%lu", info.sources[i].lines[j]);
        }

        dr_fprintf(file->fd, " ] }");
    }

    dr_fprintf(file->fd, " ], \"funcs\": ");

    if (!write_funcs(file, fullInfo)) {
        return false;
    }

    dr_fprintf(file->fd, " }");
    
    return true;
}

static bool write_funcs(debug_file_t *file, module_debug_t *info) {
    dr_fprintf(file->fd, "[ ");

    ns_info_t ns;
    ns.buf = malloc(MIN_CAPACITY * sizeof(ns.buf[0]));
    if (ns.buf == NULL) {
        PRINT_ERROR("Could not allocate initial space for namespace string buffer");
        return false;
    }
    ns.capacity = MIN_CAPACITY;
    ns.firstFunc = true;

    for (int i = 0; i < info->sizeRoots; i++) {
        ns.buf[0] = '\0';
        ns.entry = info->roots[i];

        if (!write_funcs_dfs(file, info, &ns)) {
            return false;
        }
    }

    dr_fprintf(file->fd, " ]");

    return true;
}

static bool write_funcs_dfs(debug_file_t *file, module_debug_t *info, ns_info_t *ns) {
    entry_t entry = info->entries[ns->entry];

    switch (entry.tag) {
    case DW_TAG_subprogram:
        if (!entry.hasLowPC || !entry.hasHighPC || entry.name == NULL) {
            return true;
        }

        if (!ns->firstFunc) {
            dr_fprintf(file->fd, ", ");
        }
        ns->firstFunc = false;

        dr_fprintf(file->fd, "{ \"name\": \"");

        if (strlen(ns->buf) == 0) {
            dr_fprintf(file->fd, "%s", entry.name);
        } else {
            dr_fprintf(file->fd, "%s::%s", ns->buf, entry.name);
        }

        dr_fprintf(file->fd,
                "\", \"lowpc\": \"%p\", \"highpc\": \"%p\" }",
                info->start + (size_t)entry.lowPC,
                info->start + (size_t)entry.highPC);
        break;

    case DW_TAG_namespace:
    case DW_TAG_class_type:
    case DW_TAG_structure_type:
        char *currName = entry.name == NULL ? "<NULL>" : entry.name;
        size_t minCapacity = strlen(ns->buf) + strlen(currName) + 3;

        if (ns->capacity < minCapacity) {
            ns->capacity *= 2;
            ns->buf = realloc(ns->buf, ns->capacity * sizeof(ns->buf[0]));
            if (ns->buf == NULL) {
                PRINT_ERROR("Could not allocate further space for namespace name buffer");
                return false;
            }
        }

        strcat(ns->buf, "::");
        strcat(ns->buf, currName);

    case DW_TAG_compile_unit:
        size_t currNameLen = strlen(ns->buf);
        for (int i = 0; i < entry.sizeChildren; i++) {
            ns->buf[currNameLen] = '\0';
            ns->entry = entry.children[i];
            if (!write_funcs_dfs(file, info,  ns)) {
                return false;
            }
        }
        break;

    default:
        break;
    }

    return true;
}
