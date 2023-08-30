#!/bin/bash

runDir=$(dirname $0)
source $runDir/config.sh

$runDir/$drrun -stack_size 2M -opt_cleancall 2 -c $runDir/$client $@
