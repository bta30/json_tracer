#include "filter.h"

#include <stdlib.h>
#include <string.h>

#include "query.h"
#include "error.h"

#define MIN_CAPACITY 16

static struct {
    filter_entry_t *entries;
    int sizeEntries, capacityEntries, maxValueSize;
    bool defaultInclude;
} filter;

bool init_filter(bool include) {
    PRINT_DEBUG("Enter init filter");

    filter.entries = __wrap_malloc(sizeof(filter.entries[0]) * MIN_CAPACITY);
    filter.sizeEntries = 0;
    filter.capacityEntries = MIN_CAPACITY;
    filter.maxValueSize = 0;
    filter.defaultInclude = include;
    
    if (filter.entries == NULL) {
        PRINT_ERROR("Could not allocate initial space for filter entries");
        return false;
    }

    PRINT_DEBUG("Exit init filter");
    return true;;
}

bool deinit_filter(void) {
    PRINT_DEBUG("Enter deinit filter");

    for (int i = 0; i < filter.sizeEntries; i++) {
        if (filter.entries[i].value != NULL) {
            __wrap_free(filter.entries[i].value);
        }
    }

    if (filter.entries == NULL) {
        __wrap_free(filter.entries);
    }

    PRINT_DEBUG("Exit deinit filter");
    return true;
}

bool add_filter_entry(filter_entry_t entry) {
    PRINT_DEBUG("Enter add filter entry");

    if (filter.sizeEntries == filter.capacityEntries) {
        filter.capacityEntries *= 2;
        filter.entries = __wrap_realloc(filter.entries, filter.sizeEntries *
                                                 sizeof(filter.entries[0]));

        if (filter.entries) {
            PRINT_ERROR("Could not allocate further space for filter entries");
            return false;
        }
    }

    filter.entries[filter.sizeEntries++] = entry;

    if (entry.value != NULL) {
        int valueSize = strlen(entry.value) + 1;
        if (valueSize > filter.maxValueSize) {
            filter.maxValueSize = valueSize;
        }
    }

    PRINT_DEBUG("Exit add filter entry");
    return true;
}

bool filter_include_instr(instr_t *instr) {
    PRINT_DEBUG("Enter filter include pc");

    void *pc = instr_get_app_pc(instr);
    const char *opcodeName = decode_opcode_name(instr_get_opcode(instr));

    char modulePath[filter.maxValueSize], sourcePath[filter.maxValueSize];
    if (!query_file(pc, modulePath, sourcePath, filter.maxValueSize)) {
        return false;
    }

    bool include = filter.defaultInclude;
    for (int i = filter.sizeEntries - 1; i >= 0; i--) {
        filter_entry_t entry = filter.entries[i];
        bool applyEntry = (entry.type == module &&
                            (entry.value == NULL ||
                             strcmp(entry.value, modulePath) == 0)) ||
                          (entry.type == file &&
                            (entry.value == NULL ||
                             strcmp(entry.value, sourcePath) == 0)) ||
                          (entry.type == instruction &&
                            (entry.value == NULL ||
                            strcmp(entry.value, opcodeName) == 0));

        if (applyEntry != entry.matchNot) {
            include = entry.include;
            break;
        }
    }

    PRINT_DEBUG("Exit filter include pc");
    return include;
}
