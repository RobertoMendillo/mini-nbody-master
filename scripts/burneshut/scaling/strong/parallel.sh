#!/bin/bash
OUTPUT_FILE="burnes_hut/results/scaling/strong/results_parallel.data"
MACHINEFILE="machinefile_p.txt"

# Inizializza il file CSV aggiungendo la colonna 'threads'
printf "network,threads,bodies,cpu_time,net_time,cache_miss_L1,cache_miss_L2\n" > $OUTPUT_FILE

# Compilazione
echo "Compilazione in corso..."
mpicc -O3 burnes_hut/parallel/nbody_burneshut_mpi.c burnes_hut/burneshut_functions.c ./papi/papi_helper.c -lm -I./papi/ -Iburnes_hut/ -fopenmp /usr/local/lib/libpapi.a -o burnes_hut/parallel/nbody_mpi.out
echo "Compilazione completata. Inizio benchmark."

# Array con i thread OpenMP e le dimensioni della simulazione
THREADS=(1 2 4 8 16)
BODIES=(50000 200000 400000 800000 1600000)

for size in "${BODIES[@]}"; do
  for t in "${THREADS[@]}"; do
      echo "=========================================================="
      echo "  TEST CON $t THREAD OMP"
      echo "=========================================================="
      echo "Esecuzione simulazione per $size corpi ($t threads)..."

        # TCP/IP Ethernet
        #printf "tcpip/ethernet,$t," >> $OUTPUT_FILE
        #mpirun --map-by node -x OMP_NUM_THREADS=$t --mca btl self,tcp --mca btl_tcp_if_include em2 -machinefile $MACHINEFILE burnes_hut/parallel/nbody_mpi.out "$size" >> $OUTPUT_FILE 2>&1

        #echo "Attesa di 10 secondi per stabilizzare la rete..."
        #sleep 10

        # TCP/IP InfiniBand
        #printf "tcpip/infiniband,$t," >> $OUTPUT_FILE
        #mpirun --map-by node -x OMP_NUM_THREADS=$t --mca btl self,tcp --mca btl_tcp_if_include ib0 -machinefile $MACHINEFILE burnes_hut/parallel/nbody_mpi.out "$size" >> $OUTPUT_FILE 2>&1

        echo "Attesa di 10 secondi per stabilizzare la rete..."
        sleep 10

        # Native InfiniBand
        printf "native/infiniband,$t," >> $OUTPUT_FILE
        mpirun --map-by node --mca btl self,openib -machinefile $MACHINEFILE burnes_hut/parallel/nbody_mpi.out "$size" >> $OUTPUT_FILE 2>&1

        echo "Attesa di 10 secondi per stabilizzare la rete..."
        sleep 10
    done
done

echo "Benchmark completato! Risultati salvati in $OUTPUT_FILE"


#!/bin/bash
NP=4
BODIES=1000000
SOURCEFILE="burnes_hut/parallel/nbody_burneshut_mpi.c burnes_hut/burneshut_functions.c"
OUTPUT_FILE="burnes_hut/results/scaling/strong/results_parallel.data"
MACHINEFILE="machinefile_p.txt"

# Compilazione
echo "Compilazione in corso..."
mpicc -O3 $SOURCEFILE ./papi/papi_helper.c -lm -I./papi/ -Iburnes_hut/ -fopenmp /usr/local/lib/libpapi.a -o burnes_hut/parallel/nbody_mpi.out
echo "Compilazione completata. Inizio benchmark."

# Intestazione CSV (campi script + output di nbody_burneshut_mpi.c)
echo "MPI_Ranks,OMP_Threads,Total_Cores,NumBodies,CPU_Time,Net_Time,L1_Miss,L2_Miss" > "$OUTPUT_FILE"

# Puoi usare "1 2 4 8 16" oppure "{1..16}"
for t in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16; do
    TOTAL_CORES=$((NP * t))
    echo "Avvio test: NP=$NP, OMP_NUM_THREADS=$t (Core totali: $TOTAL_CORES)..."

    RESULT=$(mpirun -np $NP --map-by node --bind-to none \
        -x OMP_NUM_THREADS=$t \
        --mca btl self,openib \
        -machinefile $MACHINEFILE \
        burnes_hut/parallel/scaling/nbody_mpi.out $BODIES)

    # Salva la riga nel CSV e la mostra a terminale
    echo "$NP,$t,$TOTAL_CORES,$RESULT" | tee -a "$OUTPUT_FILE"
done

echo "Test completato. Risultati salvati in $OUTPUT_FILE"