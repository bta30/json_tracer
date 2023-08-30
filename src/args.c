#include "args.h"

#include <string.h>
#include <getopt.h>

#include "filter.h"
#include "instr_vals_buffer.h"
#include "module_set.h"
#include "error.h"

typedef enum arg_type {
    argInclude,
    argExclude,
    argModule,
    argFile,
    argInstruction,
    argInterleaved,
    argSeparate,
    argPrefix,
    argDebugInfo,
    argHelp
} arg_type_t;

static const struct option opts[] = {
    (struct option) { "include",            0, NULL, argInclude },
    (struct option) { "exclude",            0, NULL, argExclude },
    (struct option) { "module",             1, NULL, argModule },
    (struct option) { "file",               1, NULL, argFile },
    (struct option) { "instruction",        1, NULL, argInstruction },
    (struct option) { "output_interleaved", 0, NULL, argInterleaved },
    (struct option) { "output_separate",    0, NULL, argSeparate },
    (struct option) { "output_prefix",      1, NULL, argPrefix },
    (struct option) { "output_debug_info",  1, NULL, argDebugInfo },
    (struct option) { "help",               0, NULL, argHelp },
    (struct option) { 0,                    0, 0,    0   }
};

/**
 * Processes a single given option
 *
 * Returns: Whether successful
 */
static bool process_opt(int opt, vals_buf_opts_t *outputOpts, const char **debugPath);

/**
 * Prints the help message
 */
static void print_help(void);

/**
 * Adds a filter entry given arguments
 *
 * Returns: Whether successful
 */
static bool add_filter_arg_entry(bool include, arg_type_t argType);

bool process_args(int argc, const char **argv) {
    PRINT_DEBUG("Enter process args");

    if (!init_filter(true)) {
        return false;
    }

    char **nonConstArgv = malloc(sizeof(argv[0]) * argc);
    if (nonConstArgv == NULL) {
        PRINT_ERROR("Could not allocate space for non const argv");
        return false;
    }
    memcpy(nonConstArgv, argv, sizeof(argv[0]) * argc);

    int opt;
    vals_buf_opts_t outputOpts = { false, false, NULL };
    const char *debugPath = NULL;
    while ((opt = getopt_long(argc, nonConstArgv, "", opts, NULL)) != -1) {
        if (!process_opt(opt, &outputOpts, &debugPath)) {
            free(nonConstArgv);
            return false;
        }
    }

    free(nonConstArgv);

    if (outputOpts.prefix == NULL) {
        outputOpts.prefix = "trace";
    }

    if (!init_module_set(debugPath) || !init_vals_buf(outputOpts)) {
        return false;
    }

    PRINT_DEBUG("Exit process args");
    return true;
}

bool deinit_args(void) {
    PRINT_DEBUG("Enter deinit args");

    if (!deinit_vals_buf() || !deinit_module_set() || !deinit_filter()) {
        return false;
    }

    PRINT_DEBUG("Exit deinit args");
    return true;
}

static bool process_opt(int opt, vals_buf_opts_t *outputOpts, const char **debugPath) {
    static bool include = true;

    switch (opt) {
        case argInclude:
            include = true;
            return true;

        case argExclude:
            include = false;
            return true;
            
        case argModule:
        case argFile:
        case argInstruction:
            return add_filter_arg_entry(include, opt);

        case argInterleaved:
            outputOpts->interleaved = true;
            return true;
            
        case argSeparate:
            outputOpts->separate = true;
            return true;

        case argPrefix:
            outputOpts->prefix = optarg;
            return true;

        case argDebugInfo:
            *debugPath = optarg;
            return true;

        case argHelp:
            print_help();
            return false;

        default:
            dr_printf("Error: Invalid client usage - use --help for options\n");
            return false;
    }
}

static void print_help(void) {
    dr_printf("Client usage: drrun -c libjsontracer.so [OPTIONS] -- [COMMAND]\n"
              "Produces a trace of the execution of a given command in JSON\n"
              "In the case of a conflict of options, the options later in "
              "[OPTIONS] supersede the earlier options.\n"
              "It is possible that you may have to further specify a -stack_size "
              "for drrun if your command opens modules with a lot of debugging "
              "information.\n"
              "\n"
              "Options:\n"
               "   --include                                                           "
               "Sets all following options to include what is specified\n"
               "   --exclude                                                           "
               "Sets all following options to exclude what is specified\n"
               "   --module [PATH] | --module -N[PATH] | --module --all            "
               "Specifies a module to filter, or any module\n"
               "   --file  [PATH]  | --file -N[PATH] | --file --all                "
               "Specifies a source file to filter, or any source file\n"
               "   --instr [OPCODE NAME] | --instr -N[OPCODE NAME] | --instr --all "
               "Specifies an instruction to filter, or any instruction\n"
               "   --output_interleaved                                                "
               "Outputs a JSON file with interleaved entries from all threads\n"
               "   --output_separated                                                  "
               "Outputs separate JSON files for entries from each thread\n"
               "   --output_prefix [PREFIX]                                            "
               "Sets the prefix for the output of the trace\n"
               "   --output_debug_info [PATH]                                          "
               "Outputs extra debugging information from the trace\n"
               "   --help                                                              "
               "Prints this help message\n"
               "\n"
               "Note: By default, the client works as if starting with arguments: "
               "--output_prefix trace --include --module --all\n");
}

static bool add_filter_arg_entry(bool include, arg_type_t argType) {
    filter_entry_t entry;
    entry.include = include;
    entry.matchNot = optarg[0] == '-' && optarg[1] == 'N';

    if (strcmp(optarg, "--all") == 0) {
        entry.value = NULL;
    } else {
        const char *arg = optarg + (entry.matchNot ? 2 : 0);

        if (argType == argInstruction) {
            entry.value = malloc(sizeof(arg[0]) * (strlen(arg) + 1));
            strcpy(entry.value, arg);
        } else {
            entry.value = realpath(arg, NULL);
        }
    }
    
    entry.type = argType == argModule ? module :
                 argType == argFile ? file :
                 argType == argInstruction ? instruction :
                 -1;

    return add_filter_entry(entry);
}
