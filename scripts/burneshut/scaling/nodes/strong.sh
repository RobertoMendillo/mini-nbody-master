#!/bin/bash
# Strong Scaling al variare del NUMERO DI NODI (problema fisso, nodi 1..MAX_NODES)
#
# Uso: ./strong.sh [aos|soa] [hybrid|mpi_only]
#   hybrid   -> 1 processo MPI per nodo, CORES_PER_NODE thread OpenMP
#   mpi_only -> CORES_PER_NODE processi MPI per nodo, 1 thread OpenMP
VARIANT=${1:-aos}
MODE=${2:-hybrid}

MAX_NODES=4
CORES_PER_NODE=16
BODIES=1000000
MACHINEFILE="machinefile_p.txt"
OUT_DIR="burnes_hut/results/scaling/nodes/strong"
OUTPUT_FILE="$OUT_DIR/results_${VARIANT}_${MODE}.data"
TMP_MACHINEFILE="$OUT_DIR/.machinefile_tmp.txt"

if [ "$VARIANT" = "soa" ]; then
    SOURCEFILE="burnes_hut/parallel/nbody_burneshut_soa.c burnes_hut/burneshut_functions.c"
    CFLAGS="-O3 -mavx -ffast-math"
else
    SOURCEFILE="burnes_hut/parallel/nbody_burneshut_mpi.c burnes_hut/burneshut_functions.c"
    CFLAGS="-O3"
fi
BINARY="burnes_hut/parallel/scaling/nbody_nodes_${VARIANT}.out"

if [ "$MODE" = "mpi_only" ]; then
    PPN=$CORES_PER_NODE
    OMP_THREADS=1
else
    PPN=1
    OMP_THREADS=$CORES_PER_NODE
fi

mkdir -p "$OUT_DIR"

# Compilazione
echo "Compilazione in corso ($VARIANT)..."
mpicc $CFLAGS $SOURCEFILE ./papi/papi_helper.c -lm -I./papi/ -Iburnes_hut/ -fopenmp /usr/local/lib/libpapi.a -o "$BINARY"
echo "Compilazione completata. Inizio Strong Scaling sui nodi ($MODE: $PPN proc/nodo, $OMP_THREADS thread/proc)."

# Intestazione CSV
echo "Nodes,MPI_Ranks,OMP_Threads,Total_Cores,NumBodies,CPU_Time,Net_Time,L1_Miss,L2_Miss" > "$OUTPUT_FILE"

for nodes in $(seq 1 $MAX_NODES); do
    # Machinefile con i soli primi $nodes nodi, ciascuno con $PPN slot
    head -n "$nodes" "$MACHINEFILE" | sed "s/\$/ slots=$PPN/" > "$TMP_MACHINEFILE"

    NP=$((nodes * PPN))
    TOTAL_CORES=$((NP * OMP_THREADS))
    echo "Avvio test: Nodi=$nodes, NP=$NP, OMP_NUM_THREADS=$OMP_THREADS (Core totali: $TOTAL_CORES)..."

    RESULT=$(mpirun -np $NP --map-by node --bind-to none \
        -x OMP_NUM_THREADS=$OMP_THREADS \
        --mca btl self,openib \
        -machinefile "$TMP_MACHINEFILE" \
        "$BINARY" $BODIES)

    # Salva la riga nel CSV e la mostra a terminale
    echo "$nodes,$NP,$OMP_THREADS,$TOTAL_CORES,$RESULT" | tee -a "$OUTPUT_FILE"
done

rm -f "$TMP_MACHINEFILE"
echo "Test completato. Risultati salvati in $OUTPUT_FILE"
