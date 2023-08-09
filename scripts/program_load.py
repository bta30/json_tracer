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

def output_function_load(trace, debugInfo):
    funcInstrs = {}

    for entry in trace:
        funcName = get_function_name(int(entry['pc'], 16), debugInfo)
        if funcName is None:
            continue

        if funcName not in funcInstrs:
            funcInstrs[funcName] = 1
        else:
            funcInstrs[funcName] += 1

    instrsPairs = [(i, funcInstrs[i]) for i in funcInstrs]
    instrsPairs.sort(key = lambda i: i[1], reverse = True)
    for i in instrsPairs:
        print(f'{i[0]}: {i[1]}')

def output_variable_load(trace, debugInfo):
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

    varPairs = [(i, varAccs[i]) for i in varAccs]
    varPairs.sort(key = lambda i: i[1], reverse = True)
    for i in varPairs:
        print(f'{i[0]}: {i[1]}')

def output_program_load(tracePath, debugInfoPath):
    trace = get_json(tracePath)
    debugInfo = get_json(debugInfoPath)

    print('Functions:')
    output_function_load(trace, debugInfo)

    print('\nVariables:')
    output_variable_load(trace, debugInfo)

if len(sys.argv) != 3:
    print('Error: Expected two arguments')
else:
    output_program_load(sys.argv[1], sys.argv[2])
