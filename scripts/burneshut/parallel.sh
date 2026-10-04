#!/bin/bash
OUTPUT_FILE="burnes_hut/results/results_parallel.data"
MACHINEFILE="machinefile_p.txt"

# Inizializza il file CSV
printf "network,processors,bodies,cpu_time,net_time,total_time,cache_miss_L1,cache_miss_L2\n" > $OUTPUT_FILE

# Compilazione
echo "Compilazione in corso..."
mpicc -O3 burnes_hut/parallel/nbody_burneshut_mpi.c burnes_hut/burneshut_functions.c ./papi/papi_helper.c -lm -I./papi/ -Iburnes_hut/ -fopenmp /usr/local/lib/libpapi.a -o burnes_hut/parallel/nbody_mpi.out
echo "Compilazione completata. Inizio benchmark."

# Array con tutte le dimensioni della simulazione
BODIES=(30000 50000 100000 200000 400000 600000)

for size in "${BODIES[@]}"; do
    echo "Esecuzione simulazione per $size corpi..."

    # TCP/IP Ethernet
    printf "tcpip/ethernet, " >> $OUTPUT_FILE
    mpirun --mca btl self,tcp --mca btl_tcp_if_include em2 -machinefile $MACHINEFILE burnes_hut/parallel/nbody_mpi.out "$size" >> $OUTPUT_FILE 2>&1

    # TCP/IP InfiniBand
    printf "tcpip/infiniband, " >> $OUTPUT_FILE
    mpirun --mca btl self,tcp --mca btl_tcp_if_include ib0 -machinefile $MACHINEFILE burnes_hut/parallel/nbody_mpi.out "$size" >> $OUTPUT_FILE 2>&1

    # Native InfiniBand
    printf "native/infiniband, " >> $OUTPUT_FILE
    mpirun --mca btl self,openib -machinefile $MACHINEFILE burnes_hut/parallel/nbody_mpi.out "$size" >> $OUTPUT_FILE 2>&1

    echo "Attesa di 10 secondi per stabilizzare la rete..."
    sleep 10
done

echo "Benchmark completato! Risultati salvati in $OUTPUT_FILE"