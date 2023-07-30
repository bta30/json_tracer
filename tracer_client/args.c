#include "args.h"

#include <string.h>
#include <getopt.h>

#include "filter.h"
#include "error.h"

static const struct option opts[] = {
    (struct option) { "include_module", 1, NULL, 'i' },
    (struct option) { "exclude_module", 1, NULL, 'e' },
    (struct option) { "include_file",   1, NULL, 'I' },
    (struct option) { "exclude_file",   1, NULL, 'E' },
    (struct option) { "help",           0, NULL, 'h' },
    (struct option) { 0,                0, 0,    0   }
};

/**
 * Processes a single given option
 *
 * Returns: Whether successful
 */
static bool process_opt(int opt);

/**
 * Prints the help message
 */
static void print_help(void);

bool process_args(int argc, const char **argv) {
    PRINT_DEBUG("Enter process args");

    if (!init_filter(false)) {
        return false;
    }

    char **nonConstArgv = malloc(sizeof(argv[0]) * argc);
    if (nonConstArgv == NULL) {
        PRINT_ERROR("Could not allocate space for non const argv");
        return false;
    }
    memcpy(nonConstArgv, argv, sizeof(argv[0]) * argc);

    int opt;
    while ((opt = getopt_long(argc, nonConstArgv, "", opts, NULL)) != -1) {
        if (!process_opt(opt)) {
            free(nonConstArgv);
            return false;
        }
    }
    
    free(nonConstArgv);

    PRINT_DEBUG("Exit process args");
    return true;
}

bool deinit_args(void) {
    PRINT_DEBUG("Enter deinit args");

    if (!deinit_filter()) {
        return false;
    }

    PRINT_DEBUG("Exit deinit args");
    return true;
}

static bool process_opt(int opt) {
    filter_entry_t entry;
    switch (opt) {
        case 'i':  // include_module
            entry = (filter_entry_t) { true, module, optarg };
            break;

        case 'e':  // exclude_module
            entry = (filter_entry_t) { false, module, optarg };
            break;

        case 'I':  // include_file
            entry = (filter_entry_t) { true, file, optarg };
            break;

        case 'E':  // exclude_file
            entry = (filter_entry_t) { false, file, optarg };
            break;

        case 'h':  // help
            print_help();
            return false;

        default:
            printf("Error: Invalid client usage - use --help for options\n");
            return false;
    }

    return add_filter_entry(entry);
}

static void print_help(void) {
    printf("Client usage: drrun -c libjsontracer.so [OPTIONS] -- [COMMAND]\n"
           "Produces a trace of the execution of a given command in JSON\n"
           "In the case of a conflict of options, the options later in "
           "[OPTIONS] supersede the earlier options.\n"
           "It is possible that you may have to further specify a -stack_size "
           "for drrun if your command opens modules with a lot of debugging "
           "information\n"
           "\n"
           "Options:\n"
           "    --include_module [PATH]     "
           "Includes instrucitons in a module in the trace output\n"
           "    --exclude_module [PATH]     "
           "Excludes instructions in a module from the trace output\n"
           "    --include_file [PATH]       "
           "Includes instructions in a given source file in the trace output\n"
           "    --exclude_file [PATH]       "
           "Excludes instructions in a given source file in the trace output\n"
           "    --help                      "
           "Prints this help message\n");
}
