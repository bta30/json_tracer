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
    echo -e "Benchmark $4:"

    outputPrefix="$4"

    timeNormal=$(timeCommand "$1 $2" $3)
    echo "Normal time: $timeNormal ms"

    timeExcludeAll=$(timeCommand "./run.sh --output_interleaved --output_prefix $outputPrefix --exclude --module --all -- $1 $2" $3)  
    excludeAllFactor=$(echo "$timeExcludeAll/$timeNormal" | bc -l)
    echo "Time exclude all: $timeExcludeAll ms, $excludeAllFactor times slower than normal"

    timeIncludeMainModule=$(timeCommand "./run.sh --output_interleaved --output_prefix $outputPrefix --exclude --module --all --include --module $1 -- $1 $2" $3)
    includeMainModuleFactor=$(echo "$timeIncludeMainModule/$timeNormal" | bc -l)
    echo "Time include only main module: $timeIncludeMainModule ms, $includeMainModuleFactor times slower than normal"

    timeIncludeAll=$(timeCommand "./run.sh --output_interleaved --output_prefix $outputPrefix -- $1 $2" $3)
    includeAllFactor=$(echo "$timeIncludeAll/$timeNormal" | bc -l)
    echo "Time include all: $timeIncludeAll ms, $includeAllFactor times slower than normal"

    echo ""
}

function doBenchmarks {
    timeBenchmark "bin/sum" "2 test/sum/tables/*" "/dev/null" "sum"
    
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

cd $(dirname $0)
source ./config.sh

[ -d benchmark_programs ] || fetchBenchmarks

doBenchmarks
