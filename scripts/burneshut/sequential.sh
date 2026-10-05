#!/bin/bash
OUTPUT_FILE="burnes_hut/results/results.data"
MACHINEFILE="machinefile.txt"

# Inizializza il file CSV
printf "bodies,total_time,cache_miss_L1,cache_miss_L2\n" > $OUTPUT_FILE

# Compilazione
echo "Compilazione in corso..."
mpicc -O3 burnes_hut/sequential/nbody_burneshut.c burnes_hut/burneshut_functions.c ./papi/papi_helper.c -lm -I./papi/ -Iburnes_hut/ /usr/local/lib/libpapi.a -o burnes_hut/sequential/nbody.out
echo "Compilazione completata. Inizio benchmark."

# Array con tutte le dimensioni della simulazione
BODIES=(30000 50000 100000 200000 400000 600000)

for size in "${BODIES[@]}"; do
  echo "Esecuzione simulazione per $size corpi..."
mpirun -np 1 -machinefile $MACHINEFILE burnes_hut/sequential/nbody.out "$size" >> $OUTPUT_FILE 2>&1
done

echo "Benchmark completato! Risultati salvati in $OUTPUT_FILE"