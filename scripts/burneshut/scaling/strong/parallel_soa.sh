#!/bin/bash
OUTPUT_FILE="burnes_hut/results/scaling/strong/results_parallel_soa.data"
MACHINEFILE="machinefile_p.txt"

# Inizializza il file CSV aggiungendo la colonna 'threads'
printf "network,threads,bodies,cpu_time,net_time,cache_miss_L1,cache_miss_L2\n" > $OUTPUT_FILE

# Compilazione
echo "Compilazione in corso..."
mpicc -O3 burnes_hut/parallel/nbody_burneshut_soa.c burnes_hut/burneshut_functions.c ./papi/papi_helper.c -lm -I./papi/ -Iburnes_hut/ -fopenmp /usr/local/lib/libpapi.a -o burnes_hut/parallel/nbody_soa.out
echo "Compilazione completata. Inizio benchmark."

# Array con i thread OpenMP e le dimensioni della simulazione
THREADS=(1 2 4 8 16)
BODIES=(50000 200000 400000 800000 1600000)

for t in "${THREADS[@]}"; do
    echo "=========================================================="
    echo "  TEST CON $t THREAD OMP"
    echo "=========================================================="

    for size in "${BODIES[@]}"; do
        echo "Esecuzione simulazione per $size corpi ($t threads)..."

        # TCP/IP Ethernet
        printf "tcpip/ethernet,$t," >> $OUTPUT_FILE
        mpirun -x OMP_NUM_THREADS=$t --mca btl self,tcp --mca btl_tcp_if_include em2 -machinefile $MACHINEFILE burnes_hut/parallel/nbody_soa.out "$size" >> $OUTPUT_FILE 2>&1

        echo "Attesa di 10 secondi per stabilizzare la rete..."
        sleep 10

        # TCP/IP InfiniBand
        printf "tcpip/infiniband,$t," >> $OUTPUT_FILE
        mpirun -x OMP_NUM_THREADS=$t --mca btl self,tcp --mca btl_tcp_if_include ib0 -machinefile $MACHINEFILE burnes_hut/parallel/nbody_soa.out "$size" >> $OUTPUT_FILE 2>&1

        echo "Attesa di 10 secondi per stabilizzare la rete..."
        sleep 10

        # Native InfiniBand
        printf "native/infiniband,$t," >> $OUTPUT_FILE
        mpirun -x OMP_NUM_THREADS=$t --mca btl self,openib -machinefile $MACHINEFILE burnes_hut/parallel/nbody_soa.out "$size" >> $OUTPUT_FILE 2>&1

        echo "Attesa di 10 secondi per stabilizzare la rete..."
        sleep 10
    done
done

echo "Benchmark completato! Risultati salvati in $OUTPUT_FILE"