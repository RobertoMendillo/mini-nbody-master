#!/bin/bash
OUTPUT_FILE="results.data"

echo "Esecuzione sequenziale" >> $OUTPUT_FILE
echo "" >> OUTPUT_FILE
mpicc -O0 nbody.c ./papi/papi_helper.c -lm -I./papi/ /usr/local/lib/libpapi.a -o nbody.out
mpirun -np 1 -machinefile machinefile.txt nbody.out >> $OUTPUT_FILE 2>&1

echo "" >> $OUTPUT_FILE
echo "" >> $OUTPUT_FILE
