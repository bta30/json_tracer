#include "module_debug_info.h"

#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>

#include <dwarf.h>
#include <libdwarf.h>

#include "error.h"

#define MIN_CAPACITY 16

typedef struct dwarf {
    int fd;
    Dwarf_Debug dbg;
    module_debug_t *info;
    Dwarf_Off cuOffset;
} dwarf_t;

typedef struct attrs {
    Dwarf_Half tag;
    Dwarf_Off offset;
    Dwarf_Bool isInfo;

    char *name;

    bool hasType;
    Dwarf_Off typeOffset;
    Dwarf_Bool typeIsInfo;

    bool hasLowPC;
    Dwarf_Addr lowPC;

    bool hasHighPC;
    Dwarf_Addr highPC;
    bool hasHighPCOffset;
    Dwarf_Unsigned highPCOffset;

    bool hasFbregOffset;
    int fbregOffset;

    bool hasAddr;
    Dwarf_Addr addr;

    bool hasByteSize;
    Dwarf_Unsigned byteSize;
} attrs_t;

/**
 * Tests if a return value is DW_DLV_OK, if not thenprinting an error message
 * with dwarf error information if available 
 *
 * Returns: Whether res is DW_DLV_OK
 */
static bool test_ok(char *msg, Dwarf_Debug dbg, Dwarf_Error err, int res);

/**
 * Gets the dwarf_t for a file given its path
 *
 * Returns: Whether successful
 */
static bool get_dwarf(const char *path, module_debug_t *info, dwarf_t *dwarf);

/**
 * Closes a given dwarf_t
 */
static void close_dwarf(dwarf_t dwarf);

/**
 * Gets the Dwarf_Debug for a given file descriptor
 *
 * Returns: Whether successful
 */
static bool get_dwarf_debug(int fd, Dwarf_Debug *dbg);

/**
 * Gets debug information for a module given its Dwarf_Debug
 *
 * Returns: Whether successful
 */
static bool get_debug_info_from_dwarf(dwarf_t dwarf);

/**
 * Allocates space required for and initialises module debug information
 *
 * Returns: Whether successful
 */
static bool module_debug_init(module_debug_t *info);

/**
 * Copies a string to module debug information, giving its new address
 *
 * Returns: Whether successful
 */
static bool copy_string(module_debug_t *info, const char *str, char **strOut);

/**
 * Allocates an entry in the module debug information, giving the address to it
 *
 * Returns: Whether successful
 */
static bool alloc_entry(module_debug_t *info, size_t *entryIndex);

/**
 * Adds a root entry to the module debug information
 *
 * Returns: Whether successful
 */
static bool add_root(module_debug_t *info, size_t root);

/**
 * Adds a child entry to a given entry
 *
 * Returns: Whether successful
 */
static bool add_child(entry_t *entry, size_t child);

/**
 * Gets the root DIE for the next compilation unit
 *
 * Returns: Whether successful
 */
static bool get_next_cu_root(dwarf_t dwarf, bool isInfo, Dwarf_Die *die);

/**
 * Gets the offset of the compilation unit where a given DIE is found
 *
 * Returns: Whether successful
 */
static bool get_cu_offset(dwarf_t dwarf, Dwarf_Die die, Dwarf_Off *offset);

/**
 * Adds the debug information for the next compilation unit in the module
 *
 * Returns: Whether successful
 */
static bool add_cu_info(dwarf_t dwarf, Dwarf_Die die, bool isInfo);

/**
 * Adds an entry to the debug information given its DIE
 *
 * Returns: The entry if successfully added, otherwise -1
 */
static size_t add_entry_die(dwarf_t dwarf, Dwarf_Die die, bool isInfo);

/**
 * Adds an entry to the debug information given its offset and if in info
 *
 * Returns: The entry if successfully added, otherwise -1
 */
static size_t add_entry(dwarf_t dwarf, Dwarf_Off offset, Dwarf_Bool isInfo);

/**
 * Adds attributes to a given entry
 *
 * Returns: Whether successful
 */
static bool add_entry_attrs(dwarf_t dwarf, attrs_t attrs, entry_t *entry);

/**
 * Finds an entry in the debug information
 *
 * Returns: A pointer to the entry if found, otherwise -1
 */
static size_t find_entry(dwarf_t dwarf, Dwarf_Off offset, Dwarf_Bool info);

/**
 * Gets a DW_TAG_base_type entry
 *
 * Returns: Whether successful
 */
static bool get_base_type_entry(dwarf_t dwarf, Dwarf_Die die, attrs_t attrs);

/**
 * Adds a subprogram entry to the debug information given its attributes
 *
 * Returns: Whether successful
 */
static bool add_subprogram_entry(dwarf_t dwarf, Dwarf_Die die, attrs_t attrs);

/**
 * Returns a blank attrs_t
 */
static attrs_t blank_attrs(void);

/**
 * Gets relevant attributes for a given DIE
 *
 * Returns: Whether successful
 */
static bool get_attrs(dwarf_t dwarf, Dwarf_Die die, attrs_t *attrs);

/**
 * Adds the information from an attribute of a given DIE
 *
 * Returns: Whether successfully added or ignored an attribute
 */
static bool add_attr(dwarf_t dwarf, Dwarf_Attribute attr, attrs_t *attrs);

/**
 * Adds a DW_AT_name attribute's information
 *
 * Returns: Whether successful
 */
static bool addattr_name(dwarf_t dwarf, Dwarf_Attribute attr, attrs_t *attrs);

/**
 * Adds a DW_AT_byte_size attribute's information
 *
 * Returns: Whether successful
 */
static bool addattr_bs(dwarf_t dwarf, Dwarf_Attribute attr, attrs_t *attrs);

/**
 * Adds a DW_AT_type attribute's information
 *
 * Returns: Whether successful
 */
static bool addattr_type(dwarf_t dwarf, Dwarf_Attribute attr, attrs_t *attrs);

/**
 * Adds a DW_AT_low_pc attribute's information
 *
 * Returns: Whether successful
 */
static bool addattr_lopc(dwarf_t dwarf, Dwarf_Attribute attr, attrs_t *attrs);

/**
 * Adds a DW_AT_high_pc attribute's information
 *
 * Returns: Whether successful
 */
static bool addattr_hipc(dwarf_t dwarf, Dwarf_Attribute attr, attrs_t *attrs);

/**
 * Adds a DW_AT_location attribute's information
 *
 * Returns: Whether successful
 */
static bool addattr_loc(dwarf_t dwarf, Dwarf_Attribute attr, attrs_t *attrs);

/**
 * Gets the head entry of a location list
 *
 * Returns: Whether successful
 */
static bool get_loc_entry(dwarf_t dwarf, Dwarf_Loc_Head_c head, Dwarf_Locdesc_c *entry);


bool get_module_debug(const char *path, module_debug_t *info) {
    PRINT_DEBUG("Enter get module debug");

    dwarf_t dwarf;
    if (!get_dwarf(path, info, &dwarf)) {
        return false;
    }

    bool success = get_debug_info_from_dwarf(dwarf) &&
                   copy_string(info, path, &info->path);

    close_dwarf(dwarf);

    PRINT_DEBUG("Exit get module debug");

    return success;
}

void deinit_module_debug(module_debug_t info) {
    PRINT_DEBUG("Enter deinit module debug");

    if (info.roots != NULL) {
        __wrap_free(info.roots);
    }

    if (info.strs != NULL) {
        for (int i = 0; i < info.sizeStrs; i++) {
            __wrap_free(info.strs[i]);
        }
        __wrap_free(info.strs);
    }

    if (info.entries == NULL) {
        PRINT_DEBUG("Exit deinit module debug");
        return;
    }

    for (int i = 0; i < info.sizeEntries; i++) {
        bool hasChildren = info.entries[i].children != NULL &&
                           info.entries[i].capacityChildren != 0;
        if (hasChildren) {
            __wrap_free(info.entries[i].children);
        }
    }

    __wrap_free(info.entries);

    PRINT_DEBUG("Exit deinit module debug");
}

static bool test_ok(char *msg, Dwarf_Debug dbg, Dwarf_Error err, int res) {
    if (res == DW_DLV_OK) {
        return true;
    }

    if (res  == DW_DLV_ERROR) {
        char *errMsg = dwarf_errmsg(err);
        char fullMsg[strlen(msg) + strlen(errMsg) + 3];

        sprintf(fullMsg, "%s: %s", msg, errMsg);

        PRINT_ERROR(fullMsg);
        dwarf_dealloc_error(dbg, err);
    } else {
        PRINT_ERROR(msg);
    }

    return false;
}

static bool get_dwarf(const char *path, module_debug_t *info, dwarf_t *dwarf) {
    PRINT_DEBUG("Enter get dwarf");
    
    dwarf->info = info;

    dwarf->fd = open(path, O_RDONLY);
    if (dwarf->fd < 0) {
        PRINT_ERROR("Could not open module file");
        return false;
    }

    Dwarf_Error err;
    int res = dwarf_init_b(dwarf->fd, DW_GROUPNUMBER_ANY, NULL, NULL,
                           &dwarf->dbg, &err);
    bool success = test_ok("Could not initialise Dwarf_Debug from module file",
                           dwarf->dbg, err, res);
    if (!success) {
        close(dwarf->fd);
    }

    PRINT_DEBUG("Exit get dwarf");
    return success;
}

static void close_dwarf(dwarf_t dwarf) {
    dwarf_finish(dwarf.dbg);
    close(dwarf.fd);
}

static bool get_debug_info_from_dwarf(dwarf_t dwarf) {
    if (!module_debug_init(dwarf.info)) {
        return false;
    }

    bool isInfo = false;

    while (true) {
        Dwarf_Die die;
        bool success = get_next_cu_root(dwarf, isInfo, &die) &&
                       get_cu_offset(dwarf, die, &dwarf.cuOffset);

        if (!success && !isInfo) {
            isInfo = true;
            continue;
        }

        if (!success && isInfo) {
            break;
        }

        if (!add_cu_info(dwarf, die, isInfo)) {
            deinit_module_debug(*dwarf.info);
            return false;
        }
    }

    return true;
}

static bool module_debug_init(module_debug_t *info) {
    info->strs = __wrap_malloc(sizeof(info->strs[0]) * MIN_CAPACITY);
    info->sizeStrs = 0;
    info->capacityStrs = MIN_CAPACITY;

    info->entries = __wrap_malloc(sizeof(info->entries[0]) * MIN_CAPACITY);
    info->sizeEntries = 0;
    info->capacityEntries = MIN_CAPACITY;

    info->roots = __wrap_malloc(sizeof(info->roots[0]) * MIN_CAPACITY);
    info->sizeRoots = 0;
    info->capacityRoots = MIN_CAPACITY;

    if (info->strs == NULL || info->entries == NULL || info->roots == NULL) {
        PRINT_ERROR("Could not allocate space for module debug info");
        deinit_module_debug(*info);
        return false;
    }

    return true;
}

static bool copy_string(module_debug_t *info, const char *str, char **strOut) {
    PRINT_DEBUG("Enter copy string");

    if (info->sizeStrs == info->capacityStrs) {
        info->capacityStrs = 2 * info->capacityStrs;
        info->strs = __wrap_realloc(info->strs, info->capacityStrs *
                                         sizeof(info->strs[0]));
        if (info->strs == NULL) {
            PRINT_ERROR("Could not allocate space for new string entry");
            return false;
        }
    }

    int len = strlen(str) + 1;
    info->strs[info->sizeStrs] = __wrap_malloc(len * sizeof(info->strs[0][0]));
    if (info->strs[info->sizeStrs] == NULL) {
        PRINT_ERROR("Could not allocate space for new string");
        return false;
    }

    *strOut = info->strs[info->sizeStrs++];
    memcpy(*strOut, str, len * sizeof(str[0]));

    PRINT_DEBUG("Exit copy string");
    return true;
}

static bool alloc_entry(module_debug_t *info, size_t *entryIndex) {
    PRINT_DEBUG("Enter alloc entry");

    if (info->sizeEntries == info->capacityEntries) {
        info->capacityEntries *= 2;
        info->entries = __wrap_realloc(info->entries, info->capacityEntries *
                                               sizeof(info->entries[0]));
        if (info->entries == NULL) {
            PRINT_ERROR("Could not allocate space for new entry");
            return false;
        }
    }

    *entryIndex = info->sizeEntries++;

    PRINT_DEBUG("Exit alloc entry");
    return true;
}

static bool add_root(module_debug_t *info, size_t root) {
    PRINT_DEBUG("Enter add root");

    if (info->sizeRoots == info->capacityRoots) {
        info->capacityRoots *= 2;
        info->roots = __wrap_realloc(info->roots, info->capacityRoots *
                                           sizeof(info->roots[0]));
        if (info->roots == NULL) {
            PRINT_ERROR("Could not allocate space for new root");
            return false;
        }
    }

    info->roots[info->sizeRoots++] = root;

    PRINT_DEBUG("Exit add root");
    return true;
}

static bool add_child(entry_t *entry, size_t child) {
    PRINT_DEBUG("Enter add child");

    if (entry->children == NULL || entry->capacityChildren == 0) {
        entry->children = __wrap_malloc(sizeof(entry->children[0]) * MIN_CAPACITY);
        entry->sizeChildren = 0;
        entry->capacityChildren = MIN_CAPACITY;
        if (entry->children == NULL) {
            PRINT_ERROR("Could not allocate initial space for children");
            return false;
        }
    }

    if (entry->sizeChildren == entry->capacityChildren) {
        entry->capacityChildren *= 2;
        entry->children = __wrap_realloc(entry->children,
                                       entry->capacityChildren *
                                       sizeof(entry->children[0]));
        if(entry->children == NULL) {
            PRINT_ERROR("Could not allocate further space for children");
            return false;
        }
    }

    entry->children[entry->sizeChildren++] = child;

    PRINT_DEBUG("Exit add child");
    return true;
}

static bool get_next_cu_root(dwarf_t dwarf, bool isInfo, Dwarf_Die *die) {
    Dwarf_Unsigned cuHeaderLength;
    Dwarf_Half versionStamp;
    Dwarf_Off abbrevOffset;
    Dwarf_Half addressSize, lengthSize, extensionSize;
    Dwarf_Sig8 typeSignature;
    Dwarf_Unsigned typeOffset, nextCuHeaderOffset;
    Dwarf_Half headerCuType;
    Dwarf_Error err;

    int res = dwarf_next_cu_header_d(dwarf.dbg, isInfo, &cuHeaderLength,
                                     &versionStamp, &abbrevOffset,
                                     &addressSize, &lengthSize,&extensionSize,
                                     &typeSignature, &typeOffset,
                                     &nextCuHeaderOffset, &headerCuType, &err);

    if (res == DW_DLV_NO_ENTRY) {
        return false;
    }

    if (!test_ok("Could not go to next CU root", dwarf.dbg, err, res)) {
        return false;
    }

    res = dwarf_siblingof_b(dwarf.dbg, 0, isInfo, die, &err);
    return test_ok("Could not get next CU root DIE", dwarf.dbg, err, res);
}

static bool get_cu_offset(dwarf_t dwarf, Dwarf_Die die, Dwarf_Off *offset) {
    Dwarf_Half version;
    Dwarf_Bool isInfo, isDwo;
    Dwarf_Half offsetSize, addrSize, extSize;
    Dwarf_Sig8 *signature;
    Dwarf_Unsigned length;
    Dwarf_Error err;

    int res = dwarf_cu_header_basics(die, &version, &isInfo, &isDwo,
                                     &offsetSize, &addrSize, &extSize,
                                     &signature, offset, &length, &err);
    return test_ok("Could not get CU offset from DIE", dwarf.dbg, err, res);
}

static bool add_cu_info(dwarf_t dwarf, Dwarf_Die die, bool isInfo) {
    size_t root = add_entry_die(dwarf, die, isInfo);
    return root != -1 && add_root(dwarf.info, root);
}

static size_t add_entry_die(dwarf_t dwarf, Dwarf_Die die, bool isInfo) {
    PRINT_DEBUG("Enter add entry die");

    attrs_t attrs = blank_attrs();
    attrs.isInfo = isInfo;
    if (!get_attrs(dwarf, die, &attrs)) {
        return -1;
    }

    size_t foundEntry = find_entry(dwarf, attrs.offset, attrs.isInfo);
    if (foundEntry != -1) {
        return foundEntry;
    }

    size_t entryIndex;
    if (!alloc_entry(dwarf.info, &entryIndex)) {
        return -1;
    }

    if (!add_entry_attrs(dwarf, attrs, &dwarf.info->entries[entryIndex])) {
        PRINT_ERROR("Could not get entry from attributes");
        return -1;
    }

    Dwarf_Die childDie;
    Dwarf_Error err;
    int res = dwarf_child(die, &childDie, &err);
    bool success = res == DW_DLV_NO_ENTRY ||
                   test_ok("Could not get child DIE",
                           dwarf.dbg, err, res);

    while (res == DW_DLV_OK) {
        size_t childEntry = add_entry_die(dwarf, childDie, attrs.isInfo);
        if (childEntry == -1) {
            PRINT_ERROR("Null child");
            return -1;
        }

        if (!add_child(&dwarf.info->entries[entryIndex], childEntry)) {
            PRINT_ERROR("Could not add child");
            return -1;
        }

        Dwarf_Die siblingDie;
        res = dwarf_siblingof_b(dwarf.dbg, childDie,
                                dwarf.info->entries[entryIndex].isInfo,
                                &siblingDie, &err);
        dwarf_dealloc_die(childDie);
        bool success = res == DW_DLV_NO_ENTRY ||
                       test_ok("Could not get sibling DIE",
                               dwarf.dbg, err, res);
        if (!success) {
            return -1;
        }
        childDie = siblingDie;
    }

    PRINT_DEBUG("Exit add entry die");
    return entryIndex;
}

static size_t add_entry(dwarf_t dwarf, Dwarf_Off offset, Dwarf_Bool isInfo) {
    PRINT_DEBUG("Enter add entry from offset");

    Dwarf_Die die;
    Dwarf_Error err;

    int res = dwarf_offdie_b(dwarf.dbg, offset, isInfo, &die, &err);
    bool success = test_ok("Could not get DIE given offset and isInfo",
                           dwarf.dbg, err, res);

    if (!success) {
        PRINT_ERROR("Could not add entry given offset and isInfo");
        return -1;
    }

    size_t entry = add_entry_die(dwarf, die, isInfo);

    PRINT_DEBUG("Exit add entry from offset");
    return entry;
}

static bool add_entry_attrs(dwarf_t dwarf, attrs_t attrs, entry_t *entry) {
    PRINT_DEBUG("Enter add entry attrs");

    entry->children = NULL;
    entry->sizeChildren = 0;
    entry->capacityChildren = 0;

    entry->tag = attrs.tag;
    entry->offset = attrs.offset;
    entry->isInfo = attrs.isInfo;

    entry->name = attrs.name;
    
    entry->hasType = attrs.hasType;
    if (entry->hasType) {
        PRINT_DEBUG("Attempt add type attribute entry");
        entry->type = add_entry(dwarf, attrs.typeOffset + dwarf.cuOffset,
                                attrs.typeIsInfo);
        if (entry->type == -1) {
            PRINT_ERROR("Could not get DIE type");
            return false;
        }
    }

    entry->hasLowPC = attrs.hasLowPC;
    entry->lowPC = (void *)attrs.lowPC;

    if (attrs.hasHighPCOffset) {
        attrs.hasHighPC = true;
        attrs.highPC = attrs.lowPC + attrs.highPCOffset;
    }

    entry->hasHighPC = attrs.hasHighPC;
    entry->highPC = (void *)attrs.highPC;

    entry->hasFbregOffset = attrs.hasFbregOffset;
    entry->fbregOffset = attrs.fbregOffset;

    entry->hasAddr = attrs.hasAddr;
    entry->addr = (void *)attrs.addr;

    entry->hasByteSize = attrs.hasByteSize;
    entry->byteSize = attrs.byteSize;

    PRINT_DEBUG("Exit add entry attrs");
    return true;
}

static size_t find_entry(dwarf_t dwarf, Dwarf_Off offset, Dwarf_Bool info) {
    PRINT_DEBUG("Enter find entry");

    for (size_t i = 0; i < dwarf.info->sizeEntries; i++) {
        bool isEntry = dwarf.info->entries[i].offset == offset &&
                       dwarf.info->entries[i].isInfo == info;
        if (isEntry) {
            PRINT_DEBUG("Exit find entry early");
            return i;
        }
    }

    PRINT_DEBUG("Exit find entry");
    return -1;
}

static attrs_t blank_attrs(void) {
    attrs_t attrs;

    attrs.name = NULL;
    attrs.hasType = false;
    attrs.hasLowPC = false;
    attrs.hasHighPC = false;
    attrs.hasHighPCOffset = false;
    attrs.hasFbregOffset = false;
    attrs.hasAddr = false;
    attrs.hasByteSize = false;

    return attrs;
}

static bool get_attrs(dwarf_t dwarf, Dwarf_Die die, attrs_t *attrs) {
    PRINT_DEBUG("Enter get attrs");

    Dwarf_Error err;

    int res = dwarf_tag(die, &attrs->tag, &err);
    if (!test_ok("Could not get DIE tag", dwarf.dbg, err, res)) {
        return false;
    }

    res = dwarf_dieoffset(die, &attrs->offset, &err);
    if (!test_ok("Could not get DIE offset", dwarf.dbg, err, res)) {
        return false;
    }

    Dwarf_Attribute *attrList;
    Dwarf_Signed sizeAttrList;
    res = dwarf_attrlist(die, &attrList, &sizeAttrList, &err);

    if (res == DW_DLV_NO_ENTRY) {
        return true;
    }

    if (!test_ok("Could not get DIE attribute list", dwarf.dbg, err, res)) {
        return false;
    }

    bool success = true;
    for (int i = 0; i < sizeAttrList; i++) {
        if (success && !add_attr(dwarf, attrList[i], attrs)) {
            PRINT_ERROR("Could not add attribute");
            success = false;
        }

        dwarf_dealloc_attribute(attrList[i]);
    }

    dwarf_dealloc(dwarf.dbg, attrList, DW_DLA_LIST);

    PRINT_DEBUG("Exit get attrs");
    return success;
}

static bool add_attr(dwarf_t dwarf, Dwarf_Attribute attr, attrs_t *attrs) {
    Dwarf_Half attrNum;
    Dwarf_Error err;

    int res = dwarf_whatattr(attr, &attrNum, &err);
    if (!test_ok("Could not get attribute number", dwarf.dbg, err, res)) {
        return false;
    }

    switch (attrNum) {
    case DW_AT_name:
        return addattr_name(dwarf, attr, attrs);
    
    case DW_AT_byte_size:
        return addattr_bs(dwarf, attr, attrs);

    case DW_AT_type:
        return addattr_type(dwarf, attr, attrs);

    case DW_AT_low_pc:
        return addattr_lopc(dwarf, attr, attrs);

    case DW_AT_high_pc:
        return addattr_hipc(dwarf, attr, attrs);

    case DW_AT_location:
        return addattr_loc(dwarf, attr, attrs);

    default:
        return true;
    }
}

static bool addattr_name(dwarf_t dwarf, Dwarf_Attribute attr, attrs_t *attrs) {
    char *str;
    Dwarf_Error err;

    int res = dwarf_formstring(attr, &str, &err);
    return test_ok("Could not get DW_AT_name string", dwarf.dbg, err, res) &&
           copy_string(dwarf.info, str, &attrs->name);
}

static bool addattr_bs(dwarf_t dwarf, Dwarf_Attribute attr, attrs_t *attrs) {
    Dwarf_Error err;

    int res = dwarf_formudata(attr, &attrs->byteSize, &err);
    bool success = test_ok("Could not get DW_AT_byte_size unsigned data",
                          dwarf.dbg, err, res);

    attrs->hasByteSize = success;
    return success;
}

static bool addattr_type(dwarf_t dwarf, Dwarf_Attribute attr, attrs_t *attrs) {
    Dwarf_Error err;

    int res = dwarf_formref(attr, &attrs->typeOffset, &attrs->typeIsInfo,
                            &err);

    attrs->hasType = res == DW_DLV_OK;
    return true;
}

static bool addattr_lopc(dwarf_t dwarf, Dwarf_Attribute attr, attrs_t *attrs) {
    Dwarf_Error err;

    int res = dwarf_formaddr(attr, &attrs->lowPC, &err);
    bool success = test_ok("Could not get DW_AT_low_pc unsigned data",
                          dwarf.dbg, err, res);

    attrs->hasLowPC = success;
    return success;
}

static bool addattr_hipc(dwarf_t dwarf, Dwarf_Attribute attr, attrs_t *attrs) {
    Dwarf_Error err;

    int res = dwarf_formaddr(attr, &attrs->highPC, &err);
    if (res == DW_DLV_OK) {
        attrs->hasHighPC = true;
        return true;
    }

    res = dwarf_formudata(attr, &attrs->highPCOffset, &err);
    bool success = test_ok("Could not get DW_AT_high_pc address or offset",
                          dwarf.dbg, err, res);

    attrs->hasHighPCOffset = success;
    return success;
}

static bool addattr_loc(dwarf_t dwarf, Dwarf_Attribute attr, attrs_t *attrs) {
    Dwarf_Loc_Head_c locListHead;
    Dwarf_Unsigned locEntryCount;
    Dwarf_Error err;

    int res = dwarf_get_loclist_c(attr, &locListHead, &locEntryCount, &err);
    if (!test_ok("Could not get location list head", dwarf.dbg, err, res)) {
        return false;
    }

    if (locEntryCount != 1) {
        return true;
    }

    unsigned int kind;
    res = dwarf_get_loclist_head_kind(locListHead, &kind, &err);
    bool success = test_ok("Could not get location list head kind",
                           dwarf.dbg, err, res);
    if (!success) {
        dwarf_dealloc_loc_head_c(locListHead);
        return false;
    }

    if (kind != DW_LKIND_expression) {
        PRINT_ERROR("Location list head was not DW_LKIND_expression");
        dwarf_dealloc_loc_head_c(locListHead);
        return false;
    }

    Dwarf_Locdesc_c locEntry;
    if (!get_loc_entry(dwarf, locListHead, &locEntry)) {
        PRINT_ERROR("Could not get location list head entry");
        dwarf_dealloc_loc_head_c(locListHead);
        return false;
        
    }

    Dwarf_Small operator;
    Dwarf_Unsigned opnd[3], branchOffset;
    res = dwarf_get_location_op_value_c(locEntry, 0, &operator, &opnd[0],
                                        &opnd[1], &opnd[2], &branchOffset,
                                        &err);

    success = test_ok("Could not get location expression raw values",
                      dwarf.dbg, err, res);
    if (!success) {
        dwarf_dealloc_loc_head_c(locListHead);
        return false;
    }

    if (operator == DW_OP_fbreg) {
        attrs->hasFbregOffset = true;
        attrs->fbregOffset = (int)opnd[0];
    } else if (operator == DW_OP_addr) {
        attrs->hasAddr = true;
        attrs->addr = opnd[0];
    }

    dwarf_dealloc_loc_head_c(locListHead);
    return true;
}

static bool get_loc_entry(dwarf_t dwarf, Dwarf_Loc_Head_c head, Dwarf_Locdesc_c *entry) {
    Dwarf_Small lleValue;
    Dwarf_Unsigned rawLowPC, rawHighPC;
    Dwarf_Bool debugAddrUnavail;
    Dwarf_Addr lowPCCooked, highPCCooked;
    Dwarf_Unsigned locExprOpCount;
    Dwarf_Small locListSource;
    Dwarf_Unsigned exprOffset, locDescOffset;
    Dwarf_Error err;
    
    int res = dwarf_get_locdesc_entry_d(head, 0, &lleValue, &rawLowPC,
                                    &rawHighPC, &debugAddrUnavail,
                                    &lowPCCooked, &highPCCooked,
                                    &locExprOpCount, entry, &locListSource,
                                    &exprOffset, &locDescOffset, &err);
    
    bool success = test_ok("Could not get location expression details",
                      dwarf.dbg, err, res);
    if (!success) {
        return false;
    }

}
