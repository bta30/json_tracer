import sys
import json

def get_debug_info_string(debugInfo):
    if debugInfo['type'] == 'function':
        return "(%s)" % (debugInfo['name'])
    else:
        output = "(%s: %s" % \
                 (debugInfo['name'], debugInfo['valueType']['name'])

        for compound in debugInfo['valueType']['compound']:
            output += ";%s" % (compound)

        if debugInfo['isLocal']:
            output += " local"
        else:
            output += " global"

        output += ")"

        return output

def get_opnd_string(opnd):
    output = ""
    if opnd['type'] == 'reg':
        output += opnd['register']['name']
    elif opnd['type'] == 'immedInt' or opnd['type'] == 'immedFloat':
        output += opnd['value']
    elif opnd['type'] == 'pc':
        output += opnd['value']
    else:
        output += opnd['reference']['addr']
 
    if 'debugInfo' in opnd:
        for debugInfo in opnd['debugInfo']:
            output += " "
            ouput += get_debug_info_string(debugInfo)

    return output
    

def output_trace_json(jsonTrace):
    for entry in jsonTrace:
        if 'lineInfo' not in entry:
            continue

        output = '%s\t%s\t%s\t%i\t%s\t' %
                 (entry['time'], entry['tid'], entry['lineInfo']['file'],
                 entry['lineInfo']['line'], entry['opcode']['name'])

        for opnd in entry['srcs'] + entry['dsts']:
            output += get_opnd_string(opnd)
            output += "\t"

        print(output)

with open(sys.argv[1]) as file:
    output_trace_json(json.load(file))
