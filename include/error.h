#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>

#define PRINT_ERROR(str) do { fprintf(stderr, "Error: %s\n", str); } while (0)

#define EXIT_FAIL() exit(1)

#ifdef DEBUG
#define PRINT_DEBUG(str) do { fprintf(stdout, "Debug: %s\n", str); } while(0)
#else
#define PRINT_DEBUG(str) do {} while(0)
#endif

#endif
