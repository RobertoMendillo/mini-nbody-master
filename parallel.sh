#!/bin/bash
OUTPUT_FILE="results.data"

echo "Esecuzione parallela" >> $OUTPUT_FILE
echo "" >> OUTPUT_FILE
mpicc -O0 parallelized_nbody/nbody_mpi.c ./papi/papi_helper.c -lm -I./papi/ -I. /usr/local/lib/libpapi.a -o nbody_mpi.out
mpirun -machinefile machinefile.txt nbody_mpi.out >> $OUTPUT_FILE 2>&1