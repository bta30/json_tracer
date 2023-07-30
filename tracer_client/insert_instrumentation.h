#ifndef INSERT_INSTRUMENTATION_H
#define INSERT_INSTRUMENTATION_H

#include "dr_api.h"

dr_emit_flags_t insert_instrumentation(void *drcontext, void *tag,
                                       instrlist_t *instrList, instr_t *instr,
                                       bool toTrace, bool translating,
                                       void *userData);

#endif
