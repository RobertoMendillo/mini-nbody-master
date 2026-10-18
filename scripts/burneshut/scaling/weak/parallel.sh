#!/bin/bash
NP=4
BODIES_PER_CORE=25000
SOURCEFILE="burnes_hut/parallel/nbody_burneshut_mpi.c burnes_hut/burneshut_functions.c"
OUTPUT_FILE="burnes_hut/results/scaling/weak/results_parallel.data"
MACHINEFILE="machinefile_p.txt"

mkdir -p burnes_hut/results/scaling/weak

# Compilazione
echo "Compilazione in corso..."
mpicc -O3 $SOURCEFILE ./papi/papi_helper.c -lm -I./papi/ -Iburnes_hut/ -fopenmp /usr/local/lib/libpapi.a -o burnes_hut/parallel/scaling/nbody_mpi.out
echo "Compilazione completata. Inizio benchmark Weak Scaling."

# Intestazione CSV (campi script + output di nbody_burneshut_mpi.c)
echo "MPI_Ranks,OMP_Threads,Total_Cores,NumBodies,CPU_Time,Net_Time,L1_Miss,L2_Miss" > "$OUTPUT_FILE"

for t in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16; do
    TOTAL_CORES=$((NP * t))
    BODIES=$((BODIES_PER_CORE * TOTAL_CORES))
    echo "Avvio Weak Scaling: NP=$NP, OMP_NUM_THREADS=$t (Core totali: $TOTAL_CORES, Corpi: $BODIES)..."

    RESULT=$(mpirun -np $NP --map-by node --bind-to none \
        -x OMP_NUM_THREADS=$t \
        --mca btl self,openib \
        -machinefile "$MACHINEFILE" \
        burnes_hut/parallel/scaling/nbody_mpi.out "$BODIES")

    # Salva la riga nel CSV e la mostra a terminale
    echo "$NP,$t,$TOTAL_CORES,$RESULT" | tee -a "$OUTPUT_FILE"
done

echo "Weak Scaling completato. Risultati salvati in $OUTPUT_FILE"
