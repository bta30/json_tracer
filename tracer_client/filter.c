#include "filter.h"

#include <stdlib.h>
#include <string.h>

#include "query.h"
#include "error.h"

#define MIN_CAPACITY 16

struct {
    filter_entry_t *entries;
    int sizeEntries, capacityEntries, maxPathSize;
    bool defaultInclude;
} filter;

bool init_filter(bool include) {
    PRINT_DEBUG("Enter init filter");

    filter.entries = malloc(sizeof(filter.entries[0]) * MIN_CAPACITY);
    filter.sizeEntries = 0;
    filter.capacityEntries = MIN_CAPACITY;
    filter.maxPathSize = 0;
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

    if (filter.entries == NULL) {
        free(filter.entries);
    }

    PRINT_DEBUG("Exit deinit filter");
    return true;
}

bool add_filter_entry(filter_entry_t entry) {
    PRINT_DEBUG("Enter add filter entry");

    if (filter.sizeEntries == filter.capacityEntries) {
        filter.capacityEntries *= 2;
        filter.entries = reallocarray(filter.entries, filter.sizeEntries,
                                      sizeof(filter.entries[0]));

        if (filter.entries) {
            PRINT_ERROR("Could not allocate further space for filter entries");
            return false;
        }
    }

    filter.entries[filter.sizeEntries++] = entry;

    if (entry.path != NULL) {
        int pathSize = strlen(entry.path) + 1;
        if (pathSize > filter.maxPathSize) {
            filter.maxPathSize = pathSize;
        }
    }

    PRINT_DEBUG("Exit add filter entry");
    return true;
}

bool filter_include_pc(void *pc) {
    PRINT_DEBUG("Enter filter include pc");

    char modulePath[filter.maxPathSize], sourcePath[filter.maxPathSize];
    if (!query_file(pc, modulePath, sourcePath, filter.maxPathSize)) {
        return false;
    }

    bool include = filter.defaultInclude;
    for (int i = filter.sizeEntries - 1; i >= 0; i++) {
        filter_entry_t entry = filter.entries[i];
        bool applyEntry = (entry.type == module &&
                            (entry.path == NULL ||
                             strcmp(entry.path, modulePath) == 0)) ||
                          (entry.type == file &&
                            (entry.path == NULL ||
                             strcmp(entry.path, sourcePath) == 0));

        if (applyEntry) {
            include = entry.include;
            break;
        }
    }

    PRINT_DEBUG("Exit filter include pc");
    return include;
}
