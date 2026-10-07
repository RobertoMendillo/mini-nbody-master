#!/bin/bash
OUTPUT_FILE="burnes_hut/results/results_parallel_soa.data"
MACHINEFILE="machinefile_p.txt"

# Inizializza il file CSV
printf "network,bodies,cpu_time,net_time,cache_miss_L1,cache_miss_L2\n" > $OUTPUT_FILE

# Compilazione
echo "Compilazione in corso..."
mpicc -O3 -mavx -ffast-math burnes_hut/parallel/nbody_burneshut_soa.c burnes_hut/burneshut_functions.c ./papi/papi_helper.c -lm -I./papi/ -Iburnes_hut/ -fopenmp /usr/local/lib/libpapi.a -o burnes_hut/parallel/nbody_mpi_soa.out
echo "Compilazione completata. Inizio benchmark."

# Array con tutte le dimensioni della simulazione
BODIES=(10000 20000 40000 60000 80000 100000 200000 400000 600000 800000 1000000 1200000 1400000)

for size in "${BODIES[@]}"; do
    echo "Esecuzione simulazione per $size corpi..."

    # TCP/IP Ethernet
    printf "tcpip/ethernet," >> $OUTPUT_FILE
    mpirun --mca btl self,tcp --mca btl_tcp_if_include em2 -machinefile $MACHINEFILE burnes_hut/parallel/nbody_mpi_soa.out "$size" >> $OUTPUT_FILE 2>&1

    echo "Attesa di 10 secondi per stabilizzare la rete..."
    sleep 10

    # TCP/IP InfiniBand
    printf "tcpip/infiniband," >> $OUTPUT_FILE
    mpirun --mca btl self,tcp --mca btl_tcp_if_include ib0 -machinefile $MACHINEFILE burnes_hut/parallel/nbody_mpi_soa.out "$size" >> $OUTPUT_FILE 2>&1

    echo "Attesa di 10 secondi per stabilizzare la rete..."
    sleep 10

    # Native InfiniBand
    printf "native/infiniband," >> $OUTPUT_FILE
    mpirun --mca btl self,openib -machinefile $MACHINEFILE burnes_hut/parallel/nbody_mpi_soa.out "$size" >> $OUTPUT_FILE 2>&1

    echo "Attesa di 10 secondi per stabilizzare la rete..."
    sleep 10
done

echo "Benchmark completato! Risultati salvati in $OUTPUT_FILE"