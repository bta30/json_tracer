#include "trace_information.h"

#include "trace_entry.h"
#include "instr_vals_buffer.h"
#include "json_generation.h"
#include "query.h"
#include "address_lookup.h"
#include "error.h"

/**
 * Gets the trace entry information for given instrumentation values
 *
 * Returns: Whether successful
 */
static bool get_entry_info(instrument_vals_t vals, trace_entry_t *entry);

/**
 * Gets the time of an entry as a string, given a buffer to write it into
 *
 * Returns: Whether successful
 */
static bool get_time_str(time_t time, char *str);

/**
 * Gets the operand information for given instrumented operand values
 *
 * Returns: Whether successful
 */
static bool get_opnd_info(opnd_vals_t vals, opnd_info_t *info);

/**
 * Gets the register information for given instrumented register values
 *
 * Returns: Whether successful
 */
static bool get_reg_info(reg_vals_t regVals, int size, reg_info_t *regInfo);

/**
 * Gets the memory reference information for given instrumented values
 *
 * Returns: Whether successful
 */
static bool get_mem_ref_info(mem_ref_t memRef, mem_ref_info_t *memRefInfo);

/**
 * Get the hexadecimal string an array of bytes in little-endian order, without
 * a preceeding "0x"
 */
static void bytes_to_str(uint8_t *bytes, int size, char *str);

/**
 * Gets the hexadecimal character to represent a nibble
 */
static char nibble_to_char(uint8_t nibble);

/**
 * Gets the debugging information for an operand
 *
 * Returns: Whether successful
 */
static bool get_opnd_debug_info(opnd_info_t *info, void *pcAddr, void *bp);

bool flush_vals_buffer(vals_buf_t *buf) {
    PRINT_DEBUG("Entered flush values buffer");
    init_lookup();

    while (buf->size > 0) {
        instrument_vals_t vals;
        if (!entry_dequeue(buf, &vals)) {
            deinit_lookup();
            PRINT_ERROR("Unable to dequeue entry to flush buffer");
            return false;
        }

        trace_entry_t entry;
        if(!get_entry_info(vals, &entry)) {
            deinit_lookup();
            PRINT_ERROR("Unable to get entry information");
            return false;
        }

        json_file_t *file = &buf->file;
        if (!add_json_entry(file, entry)) {
            deinit_lookup();
            PRINT_ERROR("Unable to add JSON entry to flush buffer");
            return false;
        }
    }

    deinit_lookup();
    PRINT_DEBUG("Exited flush values buffer");
    return true;
}

static bool get_entry_info(instrument_vals_t vals, trace_entry_t *entry) {
    entry->tid = vals.tid;
    entry->pc = vals.pc;

    if (!query_line(entry->pc, &entry->file, &entry->line, true)) {
        PRINT_ERROR("Error when querying line in get entry info");
        return false;
    }

    entry->opcodeValue = vals.opcode;
    entry->opcodeName = decode_opcode_name(entry->opcodeValue);
    if (entry->opcodeName == NULL) {
        PRINT_ERROR("Unable to get opcode name");
        return false;
    }

    entry->sizeSrcs = vals.sizeSrcs;
    entry->sizeDsts = vals.sizeDsts;
    
    if (!get_time_str(vals.time, entry->time)) {
        PRINT_ERROR("Unable to get entry time");
        return false;
    }

    for (int i = 0; i < entry->sizeSrcs; i++) {
        if (!get_opnd_info(vals.srcs[i], &entry->srcs[i])) {
            PRINT_ERROR("Unable to get source operand information");
            return false;
        }

        if (!get_opnd_debug_info(&entry->srcs[i], vals.pc, vals.rbp)) {
            PRINT_ERROR("Unable to get source operand debug information");
            return false;
        }
    }

    for (int i = 0; i < entry->sizeDsts; i++) {
        if (!get_opnd_info(vals.dsts[i], &entry->dsts[i])) {
            PRINT_ERROR("Unable to get destination operand information");
            return false;
        }

        if (!get_opnd_debug_info(&entry->dsts[i], vals.pc, vals.rbp)) {
            PRINT_ERROR("Unable to get destination operand debug information");
            return false;
        }
    }

    return true;
}

static bool get_time_str(time_t time, char *str) {
    struct tm *timeInfo = localtime(&time);

    sprintf(str,
            "%04d-%02d-%02d %02d:%02d:%02d",
            timeInfo->tm_year + 1900, timeInfo->tm_mon + 1, timeInfo->tm_mday,
            timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
            
}
static bool get_opnd_info(opnd_vals_t vals, opnd_info_t *info) {
    info->size = (int)opnd_size_in_bytes(vals.size);

    info->type = vals.type;
    info->typeName = opnd_type_to_str(info->type);
    if (info->typeName == NULL) {
        PRINT_ERROR("Unable to get operand type");
        return false;
    }

    switch (vals.type) {
    case immedInt:
        info->info.immedInt = vals.vals.immedInt;
        return true;
    
    case immedFloat:
        info->info.immedFloat = vals.vals.immedFloat;
        return true;

    case pc:
        info->info.pc = vals.vals.pc;
        return true;

    case reg:
        return get_reg_info(vals.vals.reg, info->size, &info->info.reg);

    case memRef:
        return get_mem_ref_info(vals.vals.memRef, &info->info.memRef);

    default:
        PRINT_ERROR("Unable to set operand values");
        return false;
    }
}

static bool get_reg_info(reg_vals_t regVals, int size, reg_info_t *regInfo) {
    regInfo->reg = get_register_name(regVals.reg);
    if (regInfo->reg == NULL) {
        PRINT_ERROR("Unable to get register name");
        return false;
    }

    bytes_to_str(regVals.val, size, regInfo->val);

    regInfo->isPointerSized = reg_is_pointer_sized(regVals.reg);
    regInfo->addrVal = (void *)*(ptr_int_t *)regVals.val;
    return true;
}

static bool get_mem_ref_info(mem_ref_t memRef, mem_ref_info_t *memRefInfo) {
    memRefInfo->type = mem_ref_type_to_str(memRef.type);
    memRefInfo->isFar = bool_to_str(memRef.isFar);
    memRefInfo->addr = memRef.addr;

    memRefInfo->outputVal = memRef.deref;
    memRefInfo->val = memRef.val;

    memRefInfo->outputBaseDisp = memRef.type == baseDisp;
    if (!memRefInfo->outputBaseDisp) {
        return true;
    }

    memRefInfo->base = get_register_name(memRef.base);
    if (memRefInfo->base == NULL) {
        PRINT_ERROR("Unable to get base name");
        return false;
    }
    memRefInfo->baseVal = memRef.baseVal;

    memRefInfo->index = get_register_name(memRef.index);
    if (memRefInfo->index == NULL) {
        PRINT_ERROR("Unable to get index name");
        return false;
    }
    memRefInfo->indexVal = memRef.indexVal;
    memRefInfo->scale = memRef.scale;

    memRefInfo->disp = memRef.disp;

    return true;
}

static void bytes_to_str(uint8_t *bytes, int size, char *str) {
    str[2 * size] = '\0';

    for (int i = 0; i < size; i++) {
        str[2 * (size - i) - 1] = nibble_to_char(bytes[i] & 0xf);
        str[2 * (size - i) - 2] = nibble_to_char(bytes[i] >> 4);
    }
}

static char nibble_to_char(uint8_t nibble) {
    return nibble < 0xa ? '0' + nibble :
                          'a' + nibble - 0xa;
}

static bool get_opnd_debug_info(opnd_info_t *info, void *pcAddr, void *bp) {
    void *addr;
    switch (info->type) {
    case pc:
        addr = info->info.pc;
        break;
    
    case memRef:
        addr = info->info.memRef.addr;
        break;

    case reg:
        if (info->info.reg.isPointerSized) {
            addr = info->info.reg.addrVal;
            break;
        }

    default:
        info->debugInfo = blank_query_results();
        return true;
    }

    return query_addr(addr, pcAddr, bp, &info->debugInfo);
}
