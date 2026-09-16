#!/bin/bash
OUTPUT_FILE="results_parallel_soa.data"
MACHINEFILE="machinefile_p.txt"

# Inizializza il file CSV
printf "network, processors, bodies, cpu_time, net_time, total_time\n" > $OUTPUT_FILE

# Compilazione
echo "Compilazione in corso..."
mpicc -O3 -mavx -ffast-math parallelized_nbody/nbody_mpi_soa.c ./papi/papi_helper.c -lm -I./papi/ -I. -fopenmp /usr/local/lib/libpapi.a -o nbody_mpi_soa.out
echo "Compilazione completata. Inizio benchmark."

# Array con tutte le dimensioni della simulazione
BODIES=(30000 50000 100000 200000 400000 600000 800000 1000000)

for size in "${BODIES[@]}"; do
    echo "Esecuzione simulazione per $size corpi..."

    # TCP/IP Ethernet
    printf "tcpip/ethernet, " >> $OUTPUT_FILE
    mpirun --mca btl self,tcp --mca btl_tcp_if_include em2 -machinefile $MACHINEFILE nbody_mpi_soa.out "$size" >> $OUTPUT_FILE 2>&1

    # TCP/IP InfiniBand
    printf "tcpip/infiniband, " >> $OUTPUT_FILE
    mpirun --mca btl self,tcp --mca btl_tcp_if_include ib0 -machinefile $MACHINEFILE nbody_mpi_soa.out "$size" >> $OUTPUT_FILE 2>&1

    # Native InfiniBand
    printf "native/infiniband, " >> $OUTPUT_FILE
    mpirun --mca btl self,openib -machinefile $MACHINEFILE nbody_mpi_soa.out "$size" >> $OUTPUT_FILE 2>&1
done

echo "Benchmark completato! Risultati salvati in $OUTPUT_FILE"