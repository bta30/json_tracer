#include "trace_entry.h"

const char *opnd_type_to_str(opnd_type_t opndType) {
    switch (opndType) {
    case immedInt:
        return "immedInt";

    case immedFloat:
        return "immedFloat";

    case pc:
        return "pc";

    case reg:
        return "reg";

    case memRef:
        return "memRef";

    case invalidOpnd:
    default:
        return NULL;
    }
}

const char *mem_ref_type_to_str(mem_ref_type_t memRefType) {
    switch (memRefType) {
    case absAddr:
        return "absAddr";

    case pcRelAddr:
        return "pcRelAddr";

    case baseDisp:
        return "baseDisp";

    case invalidRef:
    default:
        return NULL;
    }
}

const char *bool_to_str(bool b) {
    return b ? "true" : "false";
}
