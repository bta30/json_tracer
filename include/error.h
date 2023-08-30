#ifndef ERROR_H
#define ERROR_H

#include "dr_api.h"

#define PRINT_ERROR(str) do { dr_fprintf(2, "Error: %s\n", str); } while (0)

#define EXIT_FAIL() exit(1)

#ifdef DEBUG
#define PRINT_DEBUG(str) do { dr_printf("Debug: %s\n", str); } while(0)
#else
#define PRINT_DEBUG(str) do {} while(0)
#endif

#endif
