import sys
import json

class GlobalVariable:
    def __init__(self, jsonOpndEntry):
        if 'debugInfo' not in jsonOpndEntry:
            self.valid = False
            return

        if jsonOpndEntry['debugInfo'][0]['type'] != 'variable':
            self.valid = False
            return

        if 'isLocal' not in jsonOpndEntry['debugInfo'][0]:
            self.valid = False
            return

        if jsonOpndEntry['debugInfo'][0]['isLocal']:
            self.valid = False
            return
        
        if 'valueType' not in jsonOpndEntry['debugInfo'][0]:
            self.valid = False
            return

        if 'name' not in jsonOpndEntry['debugInfo'][0]['valueType']:
            self.valid = False
            return

        self.valid = True
        self.name = jsonOpndEntry['debugInfo'][0]['name']
        self.addr = jsonOpndEntry['debugInfo'][0]['address']
        self.type = jsonOpndEntry['debugInfo'][0]['valueType']['name']
    
    def is_valid(self):
        return self.valid
    
    def is_mutex(self):
        return self.type == 'mutex' or self.type == 'pthread_mutex_t'

def is_valid_ref(ref):
    if 'type' not in ref or 'isFar' not in ref or 'addr' not in ref:
        return False

    if ref['type'] in ('absAddr', 'pcRelAddr'):
        return True
    elif ref['type'] == 'baseDisp':
        return 'base' in ref and 'name' in ref['base'] and \
               'value' in ref['base'] and 'index' in ref and \
               'name' in ref['index'] and 'value' in ref['index'] and \
               'scale' in ref and 'disp' in ref
    else:
        return False

def is_valid_opnd(opnd):
    if 'type' not in opnd or 'size' not in opnd:
        return False

    if opnd['type'] in ('immedInt', 'immedFloat', 'pc'):
        return 'value' in opnd
    elif opnd['type'] == 'reg':
        return 'register' in opnd and 'name' in opnd['register'] and \
               'value' in opnd['register']
    elif opnd['type'] == 'memRef':
        return 'reference' in opnd and is_valid_ref(opnd['reference'])
    else:
        return False

def is_valid_entry(entry):
    valid =  'time' in entry and 'tid' in entry and 'opcode' in entry and \
             'name' in entry['opcode'] and 'value' in entry['opcode'] and \
             'srcs' in entry and 'dsts' in entry
    
    if not valid:
        return False

    for opnd in entry['srcs'] + entry['dsts']:
        if not is_valid_opnd(opnd):
            return False

    return True

def variable_in_set(var, setVars):
    for i in setVars:
        if i.addr == var.addr:
            return True
    return False

def get_global_variables(jsonTrace):
    globalVariables = set()

    for entry in jsonTrace:
        if not is_valid_entry(entry):
            raise Exception('Invalid trace format')

        for opnd in entry['srcs'] + entry['dsts']:
            var = GlobalVariable(opnd)
            if var.is_valid() and not variable_in_set(var, globalVariables):
                globalVariables.add(var)

    return globalVariables

def calls_function(jsonTraceEntry, funcName):
    if jsonTraceEntry['opcode']['name'] != 'call':
        return False

    if 'srcs' not in jsonTraceEntry or len(jsonTraceEntry['srcs']) == 0:
        return False

    target = jsonTraceEntry['srcs'][0]
    if 'type' not in target or target['type'] != 'pc':
        return False

    if 'debugInfo' not in target:
        return False

    debugInfo = target['debugInfo'][0]
    if 'type' not in debugInfo or debugInfo['type'] != 'function':
        return False

    return 'name' in debugInfo and debugInfo['name'] == funcName

def is_call_lock(jsonTraceEntry):
    return calls_function(jsonTraceEntry, 'std::mutex::lock') or calls_function(jsonTraceEntry, 'pthread_mutex_lock')

def is_call_unlock(jsonTraceEntry):
    return calls_function(jsonTraceEntry, 'std::mutex::unlock') or calls_function(jsonTraceEntry, 'pthread_mutex_unlock')

def get_current_rax(jsonTrace, entryIndex):
    tid = jsonTrace[entryIndex]['tid']

    for i in range(entryIndex, -1, -1):
        if jsonTrace[i]['tid'] != tid:
            continue

        for opnd in jsonTrace[i]['srcs'] + jsonTrace[i]['dsts']:
            if opnd['type'] == 'reg' and opnd['register']['name'] == 'rax':
                return opnd['register']['value']

    return 0

def add_current_lock(mutexes, currLocks, addr):
    for i in mutexes:
        if int(i.addr, 16) == int(addr, 16):
            currLocks.add(i)

def remove_current_lock(mutexes, currLocks, addr):
    for i in mutexes:
        if int(i.addr, 16) == int(addr, 16):
            currLocks.remove(i)

def opnd_is_variable(opnd, variable):
    return 'debugInfo' in opnd and \
           'address' in opnd['debugInfo'][0] and \
           opnd['debugInfo'][0]['address'] == variable.addr

def locksStr(locks):
    string = "{ "
    for l in locks:
        string += l.name + " "
    return string + "}"

def eraser_lockset_one_var(jsonTrace, mutexes, variable):
    firstThread = 0
    lastThread = 0
    newThreadCandidates = {}
    threadCandidates = {}
    threadLocks = {}

    state = 'virgin'

    for i in range(len(jsonTrace)):
        entry = jsonTrace[i]
        tid = entry['tid']

        if tid not in newThreadCandidates:
            newThreadCandidates[tid] = set(mutexes)
        if tid not in threadLocks:
            threadLocks[tid] = set()

        if is_call_lock(entry):
            rax = get_current_rax(jsonTrace, i)
            add_current_lock(mutexes, threadLocks[tid], rax)
        elif is_call_unlock(entry):
            rax = get_current_rax(jsonTrace, i)
            remove_current_lock(mutexes, threadLocks[tid], rax)

        for j in entry['srcs'] + entry['dsts']:
            if not opnd_is_variable(j, variable):
                continue

            if state == 'virgin':
                state = 'exclusive'
                firstThread = tid
                lastThread = tid

            if state == 'exclusive' and tid != firstThread:
                state = 'shared'

            if state == 'shared':
                if lastThread != tid:
                    threadCandidates[lastThread] = newThreadCandidates[lastThread]
                    lastThread = tid

                cands = newThreadCandidates[tid]
                locks = threadLocks[tid]
                newThreadCandidates[tid] = set.intersection(cands, locks)

    candidates = set(mutexes)
    for tid in threadCandidates:
        candidates = set.intersection(candidates, threadCandidates[tid])

    return candidates

def get_json(filePath):
    with open(filePath) as file:
        return json.load(file)

def eraser_lockset_file(filePath):
    try:
        jsonTrace = get_json(filePath)
    except Exception as e:
        print(f'Error: Could not open {filePath} as JSON - {e}')
        return

    try:
        globalVariables = get_global_variables(jsonTrace)
    except Exception as e:
        print(f'Error: {e}')
        return

    mutexes = [var for var in globalVariables if var.is_mutex()]
    variables = [var for var in globalVariables if not var.is_mutex()]

    results = {} 
    for var in variables:
        locks = eraser_lockset_one_var(jsonTrace, mutexes, var)
        results[var.name] = [l.name for l in locks]

    return results

def print_lockset(lockset):
    for varName in lockset:
        print(f'{varName}, {lockset[varName]}')

def combine_locksets(locksets):
    combined = {}
    for lockset in locksets:
        for varName in lockset:
            if varName in combined:
                combined[varName] = list(set.intersection(set(combined[varName]), set(lockset[varName])))
            else:
                combined[varName] = lockset[varName]

    return combined

def eraser_lockset_files(filePaths):
    if len(filePaths) == 1:
        print_lockset(eraser_lockset_file(filePaths[0]))
    else:
        locksets = [eraser_lockset_file(filePath) for filePath in filePaths]

        for i in range(len(filePaths)):
            print(f'{filePaths[i]}:')
            print_lockset(locksets[i])
            print()

        print('Combined Output:')
        print_lockset(combine_locksets(locksets))

eraser_lockset_files(sys.argv[1:])
