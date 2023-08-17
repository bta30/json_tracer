import sys
import json

def get_json(filePath):
    with open(filePath) as file:
        return json.load(file)

def get_function_name(pc, debugInfo):
    for cu in debugInfo:
        for func in cu['funcs']:
            if pc >= int(func['lowpc'], 16) and pc < int(func['highpc'], 16):
                return func['name']
    return None

def get_function_load(trace, debugInfo):
    funcInstrs = {}

    for entry in trace:
        funcName = get_function_name(int(entry['pc'], 16), debugInfo)
        if funcName is None:
            continue

        if funcName not in funcInstrs:
            funcInstrs[funcName] = 1
        else:
            funcInstrs[funcName] += 1

    return funcInstrs

def get_variable_load(trace, debugInfo):
    varAccs = {}

    for entry in trace:
        for opnd in entry['srcs'] + entry['dsts']:
            if 'debugInfo' not in opnd or opnd['debugInfo'][0]['type'] != 'variable':
                continue

            varName = opnd['debugInfo'][0]['name']
            if varName not in varAccs:
                varAccs[varName] = 1
            else:
                varAccs[varName] += 1

    return varAccs

def get_trace_load(tracePath, debugInfoPath):
    trace = get_json(tracePath)
    debugInfo = get_json(debugInfoPath)

    funcLoad = get_function_load(trace, debugInfo)
    varLoad = get_variable_load(trace, debugInfo)

    return funcLoad, varLoad

def combine_loads(dstLoad, srcLoad):
    for name in srcLoad:
        if name not in dstLoad:
            dstLoad[name] = srcLoad[name]
        else:
            dstLoad[name] += srcLoad[name]

def output_load(load):
    loadPairs = [(i, load[i]) for i in load]
    loadPairs.sort(key = lambda i: i[1], reverse = True)
    for pair in loadPairs:
        print(f'{pair[0]}: {pair[1]}')

def output_loads(name, funcLoad, varLoad):
    if name is not None:
        print(name + ':\n')

    print('Function load:')
    output_load(funcLoad)

    print('\nVariable load:')
    output_load(varLoad)
    

def output_overall_load(traceDebugInfo):
    for entry in traceDebugInfo:
        if ',' not in entry or entry[0] == ',' or entry[-1] == ',':
            print(f'Error: Invalid trace/debugging information - {entry}')
            return

    overallFuncLoad = {}
    overallVarLoad = {}

    for entry in traceDebugInfo:
        tracePath = entry[:entry.index(',')]
        debugInfoPath = entry[entry.index(',')+1:]

        funcLoad, varLoad = get_trace_load(tracePath, debugInfoPath)
        if len(traceDebugInfo) != 1:
            output_loads(tracePath, funcLoad, varLoad)
            print('\n')

        combine_loads(overallFuncLoad, funcLoad)
        combine_loads(overallVarLoad, varLoad)

    if len(traceDebugInfo) == 1:
        output_loads(None, overallFuncLoad, overallVarLoad)
    else:
        output_loads('Overall', overallFuncLoad, overallVarLoad)

if len(sys.argv) == 1:
    print('Error: Requires trace/debugging information input')
else:
    output_overall_load(sys.argv[1:])
