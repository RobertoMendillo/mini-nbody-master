#!/bin/bash
OUTPUT_FILE="results.data"
MACHINEFILE="machinefile.txt"

# Inizializza il file CSV
printf "bodies,total_time\n" > $OUTPUT_FILE

# Compilazione
echo "Compilazione in corso..."
mpicc -O3 nbody.c ./papi/papi_helper.c -lm -I./papi/ /usr/local/lib/libpapi.a -o nbody.out
echo "Compilazione completata. Inizio benchmark."

# Array con tutte le dimensioni della simulazione
BODIES=(30000 50000 100000 200000 400000 600000)

for size in "${BODIES[@]}"; do
  echo "Esecuzione simulazione per $size corpi..."
mpirun -np 1 -machinefile $MACHINEFILE nbody.out "$size" >> $OUTPUT_FILE 2>&1
done

echo "Benchmark completato! Risultati salvati in $OUTPUT_FILE"