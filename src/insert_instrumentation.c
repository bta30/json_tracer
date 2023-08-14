#include "insert_instrumentation.h"

#include "dr_api.h"

#include "instr_vals_buffer.h"
#include "filter.h"
#include "error.h"

/**
 * Gets the current machine context
 *
 * Returns: The retrieved context
 */
static dr_mcontext_t get_mcontext(void);

/**
 * Clean call called before an application instruction to instrument values
 */
static void instrument(app_pc pc);

/**
 * Gets an instruction given its PC
 *
 * Returns: Whether successful
 */
static bool instr_from_pc(void *drcontext, app_pc pc, instr_t *instr);

/**
 * Gets the instrument values for the current context, given the next
 * instruction
 */
static instrument_vals_t get_instr_vals(app_pc pc, instr_t *instr);

/**
 * Returns the current thread ID
 */
static thread_id_t get_current_tid(void);

/**
 * Gets the value of register RBP in the current context
 *
 * Returns: Whether successful
 */
static bool get_rbp(void **rbp);

/**
 * Fills the operands of the instrument values
 *
 * Returns: Whether successful
 */
static bool fill_entry_opnds(instrument_vals_t *entry, instr_t *instr);

/**
 * Gets the operand values for a given operand, with an option to dereference
 * memory addresses
 *
 * Returns: Whether successful
 */
static bool get_opnd_vals(opnd_t opnd, bool deref, opnd_vals_t *opndVals);

/**
 * Gets the target operand for a given instruction
 *
 * Returns: Whether successful
 */
static bool get_target_opnd(instr_t *instr, void **target);

/**
 * Gets invalid operand values
 *
 * Returns: Whether successful
 */
static bool get_invalid_opnd_vals(opnd_vals_t *opndVals);

/**
 * Gets immediate operand values
 *
 * Returns: Whether successful
 */
static bool get_immed_opnd_vals(opnd_t opnd, opnd_vals_t *opndVals);

/**
 * Gets PC operand values
 *
 * Returns: Whether successful
 */
static bool get_pc_opnd_vals(opnd_t opnd, opnd_vals_t *opndVals);

/**
 * Gets register operand values
 *
 * Returns: Whether successful
 */
static bool get_reg_opnd_vals(opnd_t opnd, opnd_vals_t *opndVals);

/**
 * Gets address operand values
 *
 * Returns: Whether successful
 */
static bool get_mem_ref_opnd_vals(opnd_t opnd, bool deref, opnd_vals_t *vals);

/**
 * Gets the type of a memory reference operand
 *
 * Returns: The type found
 */
static mem_ref_type_t get_mem_ref_type(opnd_t opnd);

/**
 * Determines if a given memory reference operand is a far address
 *
 * Returns: Whether far
 */
static bool mem_ref_is_far(opnd_t opnd);

dr_emit_flags_t insert_instrumentation(void *drcontext, void *tag,
                                       instrlist_t *instrList, instr_t *instr,
                                       bool toTrace, bool translating,
                                       void *userData)
{
    PRINT_DEBUG("Entered insert instrumentation");

    if (!instr_is_app(instr)) {
        PRINT_DEBUG("Exited insert instrumentation from not app");
        return DR_EMIT_DEFAULT;
    }

    if (!filter_include_instr(instr)) {
        PRINT_DEBUG("Exited insert instrumentation from filter");
        return DR_EMIT_DEFAULT;
    }

    opnd_t pc = OPND_CREATE_INTPTR(instr_get_app_pc(instr));
    dr_insert_clean_call(drcontext, instrList, instr, instrument, true, 1, pc);

    PRINT_DEBUG("Exited insert instrumentation");
    return DR_EMIT_DEFAULT;
}

static dr_mcontext_t get_mcontext(void) {
    void *drcontext = dr_get_current_drcontext();

    dr_mcontext_t mcontext;
    mcontext.size = sizeof(mcontext);
    mcontext.flags = DR_MC_ALL;
    if (!dr_get_mcontext(drcontext, &mcontext)) {
        PRINT_DEBUG("Unable to get machine context");
        EXIT_FAIL();
    }

    return mcontext;
}

static void instrument(app_pc pc) {
    PRINT_DEBUG("Entered instrument clean call");

    void *drcontext = dr_get_current_drcontext();
    instr_t instr;
    if (!instr_from_pc(drcontext, pc, &instr)) {
        PRINT_ERROR("Could not get instruction from PC");
        return;
    }

    instrument_vals_t entry = get_instr_vals(pc, &instr);
    bool success = entry_enqueue(drcontext, entry);

    instr_free(drcontext, &instr);

    if (!success) {
        PRINT_ERROR("Could not enqueue to instrumented values buffer");
        EXIT_FAIL();
    }

    PRINT_DEBUG("Exited instrument clean call");
}

static bool instr_from_pc(void *drcontext, app_pc pc, instr_t *instr) {
    instr_init(drcontext, instr);
    return decode(drcontext, pc, instr) != NULL;
}

static instrument_vals_t get_instr_vals(app_pc pc, instr_t *instr) {
    instrument_vals_t entry;
    entry.pc = pc;
    entry.opcode = instr_get_opcode(instr);
    entry.time = time(NULL);
    entry.tid = get_current_tid();

    if (!get_rbp(&entry.rbp)) {
        PRINT_ERROR("Could not get RBP value");
        EXIT_FAIL();
    }

    if (!fill_entry_opnds(&entry, instr)) {
        PRINT_ERROR("Could not fill entry operands");
        EXIT_FAIL();
    }

    return entry;
}

static thread_id_t get_current_tid(void) {
    void *drcontext = dr_get_current_drcontext();
    return dr_get_thread_id(drcontext);
}

static bool get_rbp(void **rbp) {
    dr_mcontext_t mcontext = get_mcontext();
    *rbp = (void *)reg_get_value(DR_REG_RBP, &mcontext);
    return true;
}

static bool fill_entry_opnds(instrument_vals_t *entry, instr_t *instr) {
    entry->sizeSrcs = 0;
    entry->sizeDsts = 0;

    for (int i = 0; i < instr_num_srcs(instr); i++) {
        opnd_t opnd = instr_get_src(instr, i);
        opnd_vals_t opndVals;
        if (!get_opnd_vals(opnd, instr_reads_memory(instr), &opndVals)) {
            PRINT_ERROR("Unable to get operand values of source operand");
            return false;
        } else {
            entry->srcs[entry->sizeSrcs++] = opndVals;
        }
    }

    for (int i = 0; i < instr_num_dsts(instr); i++) {
        opnd_t opnd = instr_get_dst(instr, i);
        opnd_vals_t opndVals;
        if (!get_opnd_vals(opnd, instr_reads_memory(instr), &opndVals)) {
            PRINT_ERROR("Unable to get operand values of destination operand");
            return false;
        } else {
            entry->dsts[entry->sizeDsts++] = opndVals;
        }
    }

    return true;
}

static bool get_opnd_vals(opnd_t opnd, bool deref, opnd_vals_t *opndVals) {
    opndVals->size = opnd_get_size(opnd);

    return opnd_is_immed(opnd) ? get_immed_opnd_vals(opnd, opndVals) :
           opnd_is_pc(opnd)    ? get_pc_opnd_vals(opnd, opndVals) :
           opnd_is_reg(opnd)   ? get_reg_opnd_vals(opnd, opndVals) :
           opnd_is_memory_reference(opnd)
                               ? get_mem_ref_opnd_vals(opnd, deref, opndVals) :
           get_invalid_opnd_vals(opndVals);
}

static bool get_invalid_opnd_vals(opnd_vals_t *opndVals) {
    PRINT_DEBUG("Found invalid operand");
    opndVals->type = invalidOpnd;
    return false;
}

static bool get_immed_opnd_vals(opnd_t opnd, opnd_vals_t *opndVals) {
    if (opnd_is_immed_int(opnd)) {
       opndVals->type = immedInt;
       opndVals->vals.immedInt = opnd_get_immed_int(opnd);
    } else if (opnd_is_immed_float(opnd)) {
        opndVals->type = immedFloat;
        opndVals->vals.immedFloat = opnd_get_immed_float(opnd);
    } else {
        PRINT_ERROR("Unable to get type of immediate operand");
        return false;
    }

    return true;
}

static bool get_pc_opnd_vals(opnd_t opnd, opnd_vals_t *opndVals) {
    opndVals->type = pc;
    opndVals->vals.pc = opnd_get_pc(opnd);
    return true;
}

static bool get_reg_opnd_vals(opnd_t opnd, opnd_vals_t *opndVals) {
    opndVals->type = reg;

    dr_mcontext_t mcontext = get_mcontext();

    reg_vals_t regVals;
    regVals.reg = opnd_get_reg(opnd);
    if (opndVals->size == OPSZ_NA) {
        return false;
    }

    if(!reg_get_value_ex(regVals.reg, &mcontext, regVals.val)) {
        PRINT_ERROR("Unable to get register value");
        return false;
    }

    opndVals->vals.reg = regVals;
    return true;
}

static bool get_mem_ref_opnd_vals(opnd_t opnd, bool deref, opnd_vals_t *vals) {
    vals->type = memRef;

    mem_ref_t memRef;
    memRef.type = get_mem_ref_type(opnd);
    if (memRef.type == invalidRef) {
        PRINT_ERROR("Invalid memory reference");
        return false;
    }

    memRef.isFar = mem_ref_is_far(opnd);
    memRef.deref = deref;

    dr_mcontext_t mcontext = get_mcontext();
    memRef.addr = opnd_compute_address(opnd, &mcontext);

    if (memRef.deref) {
        memRef.val = *(uint64_t *)memRef.addr;
    }

    if (memRef.type == baseDisp) {
        memRef.base = opnd_get_base(opnd);
        memRef.baseVal = reg_get_value(memRef.base, &mcontext);

        memRef.index = opnd_get_index(opnd);
        memRef.indexVal = reg_get_value(memRef.index, &mcontext);
        memRef.scale = opnd_get_scale(opnd);

        memRef.disp = opnd_get_disp(opnd);
    }

    vals->vals.memRef = memRef;
    return true;
}

static mem_ref_type_t get_mem_ref_type(opnd_t opnd) {
    return opnd_is_base_disp(opnd) ? baseDisp :
           opnd_is_rel_addr(opnd)  ? pcRelAddr :
           opnd_is_abs_addr(opnd)  ? absAddr :
           invalidRef;
}

static bool mem_ref_is_far(opnd_t opnd) {
    return opnd_is_far_abs_addr(opnd) || opnd_is_far_rel_addr(opnd)
                                      || opnd_is_far_base_disp(opnd);
}
