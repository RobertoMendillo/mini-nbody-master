#!/bin/bash
NODES=4
OMP_THREADS=1
BODIES=1000000
SOURCEFILE="burnes_hut/parallel/nbody_burneshut_mpi.c burnes_hut/burneshut_functions.c"
OUTPUT_FILE="burnes_hut/results/scaling/strong/results_parallel_mpi_only.data"
MACHINEFILE="machinefile_p.txt"

mkdir -p burnes_hut/results/scaling/strong

# Compilazione
echo "Compilazione in corso..."
mpicc -O3 $SOURCEFILE ./papi/papi_helper.c -lm -I./papi/ -Iburnes_hut/ -fopenmp /usr/local/lib/libpapi.a -o burnes_hut/parallel/scaling/nbody_mpi.out
echo "Compilazione completata. Inizio benchmark Strong Scaling (Solo MPI, OMP_NUM_THREADS=1)."

# Intestazione CSV
echo "MPI_Ranks,OMP_Threads,Total_Cores,NumBodies,CPU_Time,Net_Time,L1_Miss,L2_Miss" > "$OUTPUT_FILE"

# Incrementiamo i processi MPI per nodo da 1 a 16 (NP totale da 4 a 64, con 1 thread OpenMP per processo)
for ppn in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16; do
    NP=$((NODES * ppn))
    TOTAL_CORES=$((NP * OMP_THREADS))
    echo "Avvio test: NP=$NP ($ppn proc/nodo), OMP_NUM_THREADS=$OMP_THREADS (Core totali: $TOTAL_CORES)..."

    RESULT=$(mpirun -np $NP --map-by node --bind-to none \
        -x OMP_NUM_THREADS=$OMP_THREADS \
        --mca btl self,openib \
        -machinefile "$MACHINEFILE" \
        burnes_hut/parallel/scaling/nbody_mpi.out $BODIES)

    # Salva la riga nel CSV e la mostra a terminale
    echo "$NP,$OMP_THREADS,$TOTAL_CORES,$RESULT" | tee -a "$OUTPUT_FILE"
done

echo "Test completato. Risultati salvati in $OUTPUT_FILE"
