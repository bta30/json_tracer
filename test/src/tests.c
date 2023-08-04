#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "minunit.h"

#define SCRIPTDIR "../../test/scripts/"

/**
 * Executes a test script and compares output with an expected output
 *
 * Returns: 0 on success, non-zero on failure
 */
static int test_script_output(const char *scriptName, const char *output) {
    int fd[2];
    pipe(fd);

    int ret = fork();
    if (ret == -1) {
        return 1;
    }

    if (ret == 0) {
        close(fd[0]);
        dup2(fd[1], 1);
        close(fd[1]);

        char cmdBuf[512];
        sprintf(cmdBuf, "/usr/bin/python %s%s ../../trace.0000.log",
                SCRIPTDIR, scriptName);
        system(cmdBuf);
        return 1;
    } 

    close(fd[1]);

    int outputLen = strlen(output);
    char buf[outputLen];
    read(fd[0], buf, sizeof(buf));
    return strncmp(buf, output, sizeof(buf));
}

MU_TEST(test_eraser_lockset_sum_one_thread_filter_summariser_cpp) {
    system("cd ../.. && rm -f *.log && "
           "./run.sh --output_interleaved --exclude --module --all "
           "--include --file test/sum/src/summariser.cpp -- "
           "bin/sum 1 test/sum/tables/*.csv > /dev/null");

    const char *scriptName = "eraser_lockset.py";
    const char *expectedOutput = "netSummary, ['netSummaryMutex']\n";

    int result = test_script_output(scriptName, expectedOutput);
    const char *errorMessage = "Eraser lockset - sum one thread - "
                               "filter file summariser.cpp\n";

    mu_check(result == 0);
}

MU_TEST_SUITE(test_suite) {
    MU_RUN_TEST(test_eraser_lockset_sum_one_thread_filter_summariser_cpp);
}

int main(int argc, char **argv) {
    MU_RUN_SUITE(test_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
