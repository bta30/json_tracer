#ifndef TRACE_ENTRY_H
#define TRACE_ENTRY_H

#include <stdint.h>
#include <time.h>

#include "dr_api.h"

#include "query.h"

typedef enum opnd_type {
    invalidOpnd,
    immedInt,
    immedFloat,
    pc,
    reg,
    memRef
} opnd_type_t;

typedef enum mem_ref_type {
    invalidRef,
    absAddr,
    pcRelAddr,
    baseDisp
} mem_ref_type_t;

typedef struct reg_vals {
    reg_id_t reg;
    byte val[sizeof(dr_zmm_t)];
} reg_vals_t;

typedef struct mem_ref {
    mem_ref_type_t type;
    bool isFar;
    void *addr;
    bool deref;
    uint64_t val;

    reg_id_t base;
    uint64_t baseVal;
    reg_id_t index;
    uint64_t indexVal;
    int scale;
    int disp;
} mem_ref_t;

typedef struct opnd_vals {
    opnd_size_t size;
    opnd_type_t type;
    union {
        ptr_int_t immedInt;
        float immedFloat;
        app_pc pc;
        reg_vals_t reg;
        mem_ref_t memRef;
    } vals;
} opnd_vals_t;

typedef struct instrument_vals {
    thread_id_t tid;
    void *pc;
    void *rbp;
    int opcode;
    opnd_vals_t srcs[4];
    opnd_vals_t dsts[4];
    int sizeSrcs, sizeDsts;
    time_t time;
} instrument_vals_t;

typedef struct reg_info {
    const char *reg;
    char val[2 * sizeof(dr_zmm_t) + 1]; 

    bool isPointerSized;
    void *addrVal;
} reg_info_t;

typedef struct mem_ref_info {
    const char *type;
    const char *isFar;
    void *addr;

    bool outputVal;
    uint64_t val;

    bool outputBaseDisp;
    const char *base;
    uint64_t baseVal;
    const char *index;
    uint64_t indexVal;
    int scale;
    int disp;
} mem_ref_info_t;

typedef struct opnd_info {
    int size;
    opnd_type_t type;
    const char *typeName;
    union {
        uint64_t immedInt;
        float immedFloat;
        void *pc;
        reg_info_t reg;
        mem_ref_info_t memRef;
    } info;

    query_results_t debugInfo;
} opnd_info_t;

typedef struct trace_entry {
    char time[32];

    thread_id_t tid;

    char *file;
    uint64_t line;

    void *pc;
    int opcodeValue;

    const char *opcodeName;
    opnd_info_t srcs[4];
    opnd_info_t dsts[4];
    int sizeSrcs, sizeDsts;
} trace_entry_t;

/**
 * Returns the string for an operand type, or NULL if invalid
 */
const char *opnd_type_to_str(opnd_type_t opndType);

/**
 * Returns the string for a memory reference type, or NULL if invalid
 */
const char *mem_ref_type_to_str(mem_ref_type_t memRefType);

/**
 * Returns the string for a boolean value
 */
const char *bool_to_str(bool b);

#define BUF_LEN 4096

#endif
