#!/bin/sh

cd $(dirname $0)
source ./config.sh

${drrun} -stack_size 2048K -c ${client} $@
