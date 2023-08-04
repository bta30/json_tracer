#!/bin/sh

cd $(dirname $0)
source ./config.sh

${drrun} -stack_size 1024K -c ${client} $@
