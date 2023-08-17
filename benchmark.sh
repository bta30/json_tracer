#!/bin/sh

logDir="/local/scratch"

function getSplash3 {
    cd benchmark_programs  
    git clone $splash3Repo
    cd Splash-3/codes
    awk '{if($0~"CFLAGS :=") {print $0" -g3 -gdwarf"} else {print $0}}' Makefile.config | sed 's/O2/O0/g' > Makefile.config.new
    rm Makefile.config
    mv Makefile.config.new Makefile.config
    make
    cd ../../..
}

function fetchBenchmarks {
    mkdir benchmark_programs
    getSplash3
}

function timeCommand {
    start=$(($(date +%s%N)/1000000))
    $1 <"$2" >/dev/null
    end=$(($(date +%s%N)/1000000))
    echo "$((end - start))"
}

# Given: executable arguments stdinfile name
function timeBenchmark {
    rm -f *.log

    echo -e "Benchmark $4:"

    outputPrefix="$4"

    timeNormal=$(timeCommand "$1 $2" $3)
    echo "Normal time: $timeNormal ms"

    timeExcludeAll=$(timeCommand "./run.sh --output_interleaved --output_prefix $outputPrefix --exclude --module --all -- $1 $2" $3)  
    excludeAllFactor=$(echo "scale=3; $timeExcludeAll/$timeNormal" | bc -l)
    excludeAllTraceName="$outputPrefix.0000.log"
    excludeAllTraceSize=$(du -sm $excludeAllTraceName | awk '{print $1}')
    excludeAllWriteSpeed=$(echo "scale=3; $excludeAllTraceSize/($timeExcludeAll/1000)" | bc -l)
    echo "Time exclude all: $timeExcludeAll ms, $excludeAllFactor times slower than normal, writing at speed $excludeAllWriteSpeed MB/s"

    timeIncludeMainModule=$(timeCommand "./run.sh --output_interleaved --output_prefix $outputPrefix --exclude --module --all --include --module $1 --exclude --instr -Ncall -- $1 $2" $3)
    includeMainModuleFactor=$(echo "scale=3; $timeIncludeMainModule/$timeNormal" | bc -l)
    includeMainModuleTraceName="$outputPrefix.0001.log"
    includeMainModuleTraceSize=$(du -sm $includeMainModuleTraceName | awk '{print $1}')
    includeMainModuleWriteSpeed=$(echo "scale=3; $includeMainModuleTraceSize/($timeIncludeMainModule/1000)" | bc -l)
    echo "Time include only main module (only call instructions) : $timeIncludeMainModule ms, $includeMainModuleFactor times slower than normal, writing at speed $includeMainModuleWriteSpeed MB/s"

    timeIncludeAll=$(timeCommand "./run.sh --output_interleaved --output_prefix $outputPrefix --exclude --instr -Ncall -- $1 $2" $3)
    includeAllFactor=$(echo "scale=3; $timeIncludeAll/$timeNormal" | bc -l)
    includeAllTraceName="$outputPrefix.0002.log"
    includeAllTraceSize=$(du -sm $includeAllTraceName | awk '{print $1}')
    echo $includeAllTraceSize MB $timeIncludeAll/1000 s
    includeAllWriteSpeed=$(echo "scale=3; $includeAllTraceSize/($timeIncludeAll/1000)" | bc -l)
    echo "Time include all modules (only call instructions): $timeIncludeAll ms, $includeAllFactor times slower than normal, writing at speed $includeAllWriteSpeed MB/s"

    echo ""
}

function doSumBenchmarks {
    timeBenchmark "bin/sum" "2 test/sum/tables/*" "/dev/null" "sum"
}

function doSplash3Benchmarks {
    timeBenchmark "benchmark_programs/Splash-3/codes/apps/barnes/BARNES" "" "benchmark_programs/Splash-3/codes/apps/barnes/inputs/n16384-p1" "Splash-3_Barnes_1"
    timeBenchmark "benchmark_programs/Splash-3/codes/apps/barnes/BARNES" "" "benchmark_programs/Splash-3/codes/apps/barnes/inputs/n16384-p16" "Splash-3_Barnes_16"
    timeBenchmark "benchmark_programs/Splash-3/codes/apps/barnes/BARNES" "" "benchmark_programs/Splash-3/codes/apps/barnes/inputs/n16384-p256" "Splash-3_Barnes_256"

    timeBenchmark "benchmark_programs/Splash-3/codes/apps/fmm/FMM" "" "benchmark_programs/Splash-3/codes/apps/fmm/inputs/input.1.16384" "Splash-3_FMM_1_16384"
    timeBenchmark "benchmark_programs/Splash-3/codes/apps/fmm/FMM" "" "benchmark_programs/Splash-3/codes/apps/fmm/inputs/input.2.16384" "Splash-3_FMM_2_16384"
    timeBenchmark "benchmark_programs/Splash-3/codes/apps/fmm/FMM" "" "benchmark_programs/Splash-3/codes/apps/fmm/inputs/input.64.16384" "Splash-3_FMM_64_16384"

    timeBenchmark "benchmark_programs/Splash-3/codes/apps/ocean/contiguous_partitions/OCEAN" "-p1 -n258" "/dev/null" "Splash-3_Ocean_1"
    timeBenchmark "benchmark_programs/Splash-3/codes/apps/ocean/contiguous_partitions/OCEAN" "-p4 -n258" "/dev/null" "Splash-4_Ocean_1"
    timeBenchmark "benchmark_programs/Splash-3/codes/apps/ocean/contiguous_partitions/OCEAN" "-p64 -n258" "/dev/null" "Splash-64_Ocean_1"

    timeBenchmark "benchmark_programs/Splash-3/codes/apps/radiosity/RADIOSITY" "-p 1 -ae 5000 -bf 0.1 -en 0.05 -room -batch" "/dev/null" "Splash-64_Radiosity_1"
    timeBenchmark "benchmark_programs/Splash-3/codes/apps/radiosity/RADIOSITY" "-p 4 -ae 5000 -bf 0.1 -en 0.05 -room -batch" "/dev/null" "Splash-64_Radiosity_4"
    timeBenchmark "benchmark_programs/Splash-3/codes/apps/radiosity/RADIOSITY" "-p 64 -ae 5000 -bf 0.1 -en 0.05 -room -batch" "/dev/null" "Splash-64_Radiosity_64"

    timeBenchmark "benchmark_programs/Splash-3/codes/apps/raytrace/RAYTRACE" "-p1 -m64 benchmark_programs/Splash-3/codes/apps/raytrace/inputs/car.env" "/dev/null" "Splash-64_Raytrace_1"
    timeBenchmark "benchmark_programs/Splash-3/codes/apps/raytrace/RAYTRACE" "-p4 -m64 benchmark_programs/Splash-3/codes/apps/raytrace/inputs/car.env" "/dev/null" "Splash-64_Raytrace_4"
    timeBenchmark "benchmark_programs/Splash-3/codes/apps/raytrace/RAYTRACE" "-p64 -m64 benchmark_programs/Splash-3/codes/apps/raytrace/inputs/car.env" "/dev/null" "Splash-64_Raytrace_64"

    timeBenchmark "benchmark_programs/Splash-3/codes/apps/volrend/VOLREND" "1 benchmark_programs/Splash-3/codes/apps/volrend/inputs/head 8" "/dev/null" "Splash-3_Volrend_1"
    timeBenchmark "benchmark_programs/Splash-3/codes/apps/volrend/VOLREND" "4 benchmark_programs/Splash-3/codes/apps/volrend/inputs/head 8" "/dev/null" "Splash-3_Volrend_4"
    timeBenchmark "benchmark_programs/Splash-3/codes/apps/volrend/VOLREND" "64 benchmark_programs/Splash-3/codes/apps/volrend/inputs/head 8" "/dev/null" "Splash-3_Volrend_64"

    timeBenchmark "benchmark_programs/Splash-3/codes/kernels/cholesky/CHOLESKY" "-p1" "benchmark_programs/Splash-3/codes/kernels/cholesky/inputs/tk15.O" "Splash-3_Cholesky_1"
    timeBenchmark "benchmark_programs/Splash-3/codes/kernels/cholesky/CHOLESKY" "-p4" "benchmark_programs/Splash-3/codes/kernels/cholesky/inputs/tk15.O" "Splash-3_Cholesky_4"
    timeBenchmark "benchmark_programs/Splash-3/codes/kernels/cholesky/CHOLESKY" "-p64" "benchmark_programs/Splash-3/codes/kernels/cholesky/inputs/tk15.O" "Splash-3_Cholesky_64"

    timeBenchmark "benchmark_programs/Splash-3/codes/kernels/fft/FFT" "-p1 -m16" "/dev/null" "Splash-3_FFT_1"
    timeBenchmark "benchmark_programs/Splash-3/codes/kernels/fft/FFT" "-p4 -m16" "/dev/null" "Splash-3_FFT_4"
    timeBenchmark "benchmark_programs/Splash-3/codes/kernels/fft/FFT" "-p64 -m16" "/dev/null" "Splash-3_FFT_64"

    timeBenchmark "benchmark_programs/Splash-3/codes/kernels/lu/contiguous_blocks/LU" "-p1 -n512" "/dev/null" "Splash-3_LU_1"
    timeBenchmark "benchmark_programs/Splash-3/codes/kernels/lu/contiguous_blocks/LU" "-p4 -n512" "/dev/null" "Splash-3_LU_4"
    timeBenchmark "benchmark_programs/Splash-3/codes/kernels/lu/contiguous_blocks/LU" "-p64 -n512" "/dev/null" "Splash-3_LU_64"

    timeBenchmark "benchmark_programs/Splash-3/codes/kernels/radix/RADIX" "-p1 -n1048576" "/dev/null" "Splash-3_Radix_1"
    timeBenchmark "benchmark_programs/Splash-3/codes/kernels/radix/RADIX" "-p4 -n1048576" "/dev/null" "Splash-3_Radix_4"
    timeBenchmark "benchmark_programs/Splash-3/codes/kernels/radix/RADIX" "-p64 -n1048576" "/dev/null" "Splash-3_Radix_64"
}

function doBenchmarks {
    doSumBenchmarks
    doSplash3Benchmarks
}

cd $(dirname $0)
source ./config.sh

[ -d benchmark_programs ] || fetchBenchmarks

doBenchmarks
