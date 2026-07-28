#!/bin/bash
OUTPUT_FILE="results.data"

printf "Esecuzione sequenziale\n" > $OUTPUT_FILE

mpicc -O3 nbody.c ./papi/papi_helper.c -lm -I./papi/ /usr/local/lib/libpapi.a -o nbody.out

printf "\n\n================ 30_0000 ================\n\n" >> $OUTPUT_FILE 2>&1
printf "\n\n================ 30_0000 ================\n\n"
mpirun -np 1 -machinefile machinefile.txt nbody.out >> $OUTPUT_FILE 2>&1

printf "\n\n================ 50_0000 ================\n\n" >> $OUTPUT_FILE 2>&1
printf "\n\n================ 50_0000 ================\n\n"
mpirun -np 1 -machinefile machinefile.txt nbody.out 50000 >> $OUTPUT_FILE 2>&1

printf "\n\n================ 100_0000 ================\n\n" >> $OUTPUT_FILE 2>&1
printf "\n\n================ 100_0000 ================\n\n"
mpirun -np 1 -machinefile machinefile.txt nbody.out 100000 >> $OUTPUT_FILE 2>&1

printf "\n\n================ 200_0000 ================\n\n" >> $OUTPUT_FILE 2>&1
printf "\n\n================ 200_0000 ================\n\n"
mpirun -np 1 -machinefile machinefile.txt nbody.out 200000 >> $OUTPUT_FILE 2>&1
