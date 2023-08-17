#!/bin/sh

cd $(dirname $0)
source ./config.sh

${drrun} -stack_size 2M -opt_cleancall 2 -c ${client} $@
