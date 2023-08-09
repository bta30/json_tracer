import sys
import os
import json

def get_json(filePath):
    with open(filePath) as file:
        return json.load(file)

def get_all_file_lines(debugInfo, file):
    file_lines = []
    for moduleEntry in get_json(debugInfo):
        for fileEntry in moduleEntry['sourceFiles']:
            if fileEntry['path'] == file:
               file_lines += fileEntry['lines']

    return set(file_lines)

def get_covered_file_lines(trace, file):
    cov_lines = set()
    for traceEntry in get_json(trace):
        if 'lineInfo' in traceEntry and traceEntry['lineInfo']['file'] == file:
            cov_lines.add(traceEntry['lineInfo']['line'])

    return cov_lines

def get_file_coverage(trace, debugInfo, file):
    allLines = get_all_file_lines(debugInfo, file)
    covLines = get_covered_file_lines(trace, file)

    return {'lines': len(allLines), 'linesCovered': len(covLines)}

def output_coverage(name, cov):
    fracCov = 1 if cov['lines'] == 0 else cov['linesCovered'] / cov['lines']

    print(f'{name}: {cov["linesCovered"]} lines covered out of ' \
          f'{cov["lines"]} lines - {fracCov * 100}%')

def output_overall_coverage(trace, debugInfo, files):
    overallCov = { 'lines': 0, 'linesCovered': 0 }

    for file in files:
        filePath = os.path.abspath(file)

        fileCov = get_file_coverage(trace, debugInfo, filePath)
        output_coverage(file, fileCov)

        overallCov['lines'] += fileCov['lines']
        overallCov['linesCovered'] += fileCov['linesCovered']

    output_coverage('Overall', overallCov)

if len(sys.argv) < 4:
    print("Error: Requires more arguments")
else:
    output_overall_coverage(sys.argv[1], sys.argv[2], sys.argv[3:])
