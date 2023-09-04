#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "minunit.h"

#define ROOTDIR ".."
#define SUMPROG "build_test/sum/sum"
#define SUMTABLES "test/sum/tables/*.csv"
#define SCRIPTDIR ROOTDIR "/scripts/"
#define ERASER "eraser_lockset.py"

/**
 * Executes a the Eraser script and compares output with an expected output
 *
 * Returns: 0 on success, non-zero on failure
 */
static int eraser(const char *trace, const char *output) {
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

        execl("/usr/bin/python3", "python", SCRIPTDIR ERASER, trace, NULL);
    } 

    close(fd[1]);

    int outputLen = strlen(output);
    char buf[outputLen + 128];
    buf[0] = '\0';
    int outSize = read(fd[0], buf, sizeof(buf));
    if (strncmp(buf, output, outputLen)) {
        printf("Invalid output of Eraser on trace %s:\n", trace);
        for (int  i = 0; i < outSize; i++) {
            printf("%c", buf[i]);
        }
        printf("\nExpected:\n%.*s\n", outputLen, output);
        return 1;
    }

    return 0;
}

MU_TEST(test_eraser_lockset_sum_one_thread_no_filter) {
    system("cd " ROOTDIR " && "
           "./run.sh --output_interleaved --output_prefix 1tnf -- "
           SUMPROG " 1 " SUMTABLES " >/dev/null");

    const char *traceName = ROOTDIR "/1tnf.0000.log";
    const char *expectedOutput = "netSummary, ['netSummaryMutex']\n";

    int result = eraser(traceName, expectedOutput);
    mu_check(result == 0);
}

MU_TEST(test_eraser_lockset_sum_two_thread_no_filter) {
    system("cd " ROOTDIR " && "
           "./run.sh --output_interleaved --output_prefix 2tnf -- "
           SUMPROG " 1 " SUMTABLES " >/dev/null");

    const char *traceName = ROOTDIR "/2tnf.0000.log";
    const char *expectedOutput = "netSummary, ['netSummaryMutex']\n";

    int result = eraser(traceName, expectedOutput);
    mu_check(result == 0);
}

MU_TEST(test_eraser_lockset_sum_one_thread_filter_module_sum) {
    system("cd " ROOTDIR " && "
           "./run.sh --output_interleaved --output_prefix 1tfm "
           "--exclude --module --all --include --module " SUMPROG " -- "
           SUMPROG " 1 " SUMTABLES " >/dev/null");

    const char *traceName = ROOTDIR "/1tfm.0000.log";
    const char *expectedOutput = "netSummary, ['netSummaryMutex']\n";

    int result = eraser(traceName, expectedOutput);
    mu_check(result == 0);
}

MU_TEST(test_eraser_lockset_sum_two_thread_filter_module_sum) {
    system("cd " ROOTDIR " && "
           "./run.sh --output_interleaved --output_prefix 2tfm "
           "--exclude --module --all --include --module " SUMPROG " -- "
           SUMPROG " 2 " SUMTABLES " >/dev/null");

    const char *traceName = ROOTDIR "/2tfm.0000.log";
    const char *expectedOutput = "netSummary, ['netSummaryMutex']\n";

    int result = eraser(traceName, expectedOutput);
    mu_check(result == 0);
}

MU_TEST(test_eraser_lockset_sum_one_thread_filter_summariser_cpp) {
    system("cd " ROOTDIR " && "
           "./run.sh --output_interleaved --output_prefix 1tff "
           "--exclude --module --all "
           "--include --file test/sum/src/summariser.cpp -- "
           SUMPROG " 1 " SUMTABLES " >/dev/null");

    const char *traceName = ROOTDIR "/1tff.0000.log";
    const char *expectedOutput = "netSummary, ['netSummaryMutex']\n";

    int result = eraser(traceName, expectedOutput);
    mu_check(result == 0);
}

MU_TEST(test_eraser_lockset_sum_two_thread_filter_summariser_cpp) {
    system("cd " ROOTDIR " && "
           "./run.sh --output_interleaved --output_prefix 2tff "
           "--exclude --module --all "
           "--include --file test/sum/src/summariser.cpp -- "
           SUMPROG " 2 " SUMTABLES " >/dev/null");

    const char *traceName = ROOTDIR "/2tff.0000.log";
    const char *expectedOutput = "netSummary, ['netSummaryMutex']\n";

    int result = eraser(traceName, expectedOutput);
    mu_check(result == 0);
}

MU_TEST(test_eraser_lockset_sum_one_thread_filter_all) {
    system("cd " ROOTDIR " && "
           "./run.sh --output_interleaved --output_prefix 1tfa "
           "--exclude --module --all -- "
           SUMPROG " 1 " SUMTABLES " >/dev/null");

    const char *traceName = ROOTDIR "/1tfa.0000.log";
    const char *expectedOutput = "";

    int result = eraser(traceName, expectedOutput);
    mu_check(result == 0);
}

MU_TEST(test_eraser_lockset_sum_two_thread_filter_all) {
    system("cd " ROOTDIR " && "
           "./run.sh --output_interleaved --output_prefix 2tfa "
           "--exclude --module --all -- "
           SUMPROG " 2 " SUMTABLES " >/dev/null");

    const char *traceName = ROOTDIR "/2tfa.0000.log";
    const char *expectedOutput = "";

    int result = eraser(traceName, expectedOutput);
    mu_check(result == 0);
}

MU_TEST_SUITE(test_suite) {
    MU_RUN_TEST(test_eraser_lockset_sum_one_thread_no_filter);
    MU_RUN_TEST(test_eraser_lockset_sum_two_thread_no_filter);
    MU_RUN_TEST(test_eraser_lockset_sum_one_thread_filter_module_sum);
    MU_RUN_TEST(test_eraser_lockset_sum_two_thread_filter_module_sum);
    MU_RUN_TEST(test_eraser_lockset_sum_one_thread_filter_summariser_cpp);
    MU_RUN_TEST(test_eraser_lockset_sum_two_thread_filter_summariser_cpp);
    MU_RUN_TEST(test_eraser_lockset_sum_one_thread_filter_all);
    MU_RUN_TEST(test_eraser_lockset_sum_two_thread_filter_all);
}

int main(int argc, char **argv) {
    MU_RUN_SUITE(test_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
