import sys
import os
import json

def get_json(filePath):
    with open(filePath) as file:
        return json.load(file)

def get_all_file_lines(traceDebugInfo, file):
    file_lines = []

    for entry in traceDebugInfo:
        debugInfo = entry[entry.index(',')+1:]
        for moduleEntry in get_json(debugInfo):
            for fileEntry in moduleEntry['sourceFiles']:
                if fileEntry['path'] == file:
                   file_lines += fileEntry['lines']

    return set(file_lines)

def get_covered_file_lines(traceDebugInfo, file):
    cov_lines = set()
    
    for entry in traceDebugInfo:
        trace = entry[:entry.index(',')]
        for traceEntry in get_json(trace):
            if 'lineInfo' in traceEntry and traceEntry['lineInfo']['file'] == file:
                cov_lines.add(traceEntry['lineInfo']['line'])

    return cov_lines

def get_file_coverage(traceDebugInfo, file):
    allLines = get_all_file_lines(traceDebugInfo, file)
    covLines = get_covered_file_lines(traceDebugInfo, file)

    return {'lines': len(allLines), 'linesCovered': len(covLines)}

def output_coverage(name, cov):
    fracCov = 1 if cov['lines'] == 0 else cov['linesCovered'] / cov['lines']

    print(f'{name}: {cov["linesCovered"]} lines covered out of ' \
          f'{cov["lines"]} lines - {fracCov * 100}%')

def output_overall_coverage(traceDebugInfo, files):
    for entry in traceDebugInfo:
        if ',' not in entry or entry[0] == ',' or entry[-1] == ',':
            print(f'Error: Invalid trace/debugging information - {entry}')
            return

    overallCov = { 'lines': 0, 'linesCovered': 0 }

    for file in files:
        filePath = os.path.abspath(file)

        fileCov = get_file_coverage(traceDebugInfo, filePath)
        output_coverage(file, fileCov)

        overallCov['lines'] += fileCov['lines']
        overallCov['linesCovered'] += fileCov['linesCovered']

    output_coverage('Overall', overallCov)

if '--' not in sys.argv:
    print("Error: Expected argument '--'")
else:
    ind = sys.argv.index('--')
    if ind == sys.argv[1]:
        print("Error: Require traces/debugging information of form trace.log,debug.js")
    elif ind == len(sys.argv) - 1:
        print("Error: Expected source files")
    else:
        output_overall_coverage(sys.argv[1:ind], sys.argv[ind+1:])
