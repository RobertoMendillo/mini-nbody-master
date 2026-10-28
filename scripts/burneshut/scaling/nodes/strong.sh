#!/bin/bash
# Strong Scaling al variare del NUMERO DI NODI (problema fisso, nodi 1..MAX_NODES)
# Versione AoS, modello ibrido: 1 processo MPI per nodo, THREADS_PER_NODE thread OpenMP
#
# Uso: ./strong.sh [thread_per_nodo]   (default: 16)
THREADS_PER_NODE=${1:-16}

if ! [[ "$THREADS_PER_NODE" =~ ^[1-9][0-9]*$ ]]; then
    echo "Errore: il numero di thread per nodo deve essere un intero positivo (ricevuto: '$THREADS_PER_NODE')" >&2
    exit 1
fi

MAX_NODES=4
BODIES=1000000
SOURCEFILE="burnes_hut/parallel/nbody_burneshut_mpi.c burnes_hut/burneshut_functions.c"
BINARY="burnes_hut/parallel/scaling/nbody_nodes.out"
MACHINEFILE="machinefile_p.txt"
OUT_DIR="burnes_hut/results/scaling/nodes/strong"
OUTPUT_FILE="$OUT_DIR/results_${THREADS_PER_NODE}t.data"
TMP_MACHINEFILE="$OUT_DIR/.machinefile_tmp.txt"

mkdir -p "$OUT_DIR"

# Compilazione
echo "Compilazione in corso..."
mpicc -O3 $SOURCEFILE ./papi/papi_helper.c -lm -I./papi/ -Iburnes_hut/ -fopenmp /usr/local/lib/libpapi.a -o "$BINARY"
echo "Compilazione completata. Inizio Strong Scaling sui nodi (1 proc/nodo, $THREADS_PER_NODE thread/nodo)."

# Intestazione CSV
echo "Nodes,MPI_Ranks,OMP_Threads,Total_Cores,NumBodies,CPU_Time,Net_Time,L1_Miss,L2_Miss" > "$OUTPUT_FILE"

for nodes in $(seq 1 $MAX_NODES); do
    # Machinefile con i soli primi $nodes nodi, 1 slot ciascuno
    head -n "$nodes" "$MACHINEFILE" | sed "s/\$/ slots=1/" > "$TMP_MACHINEFILE"

    NP=$nodes
    TOTAL_CORES=$((NP * THREADS_PER_NODE))
    echo "Avvio test: Nodi=$nodes, NP=$NP, OMP_NUM_THREADS=$THREADS_PER_NODE (Core totali: $TOTAL_CORES)..."

    RESULT=$(mpirun -np $NP --map-by node --bind-to none \
        -x OMP_NUM_THREADS=$THREADS_PER_NODE \
        --mca btl self,openib \
        -machinefile "$TMP_MACHINEFILE" \
        "$BINARY" $BODIES)

    # Salva la riga nel CSV e la mostra a terminale
    echo "$nodes,$NP,$THREADS_PER_NODE,$TOTAL_CORES,$RESULT" | tee -a "$OUTPUT_FILE"
done

rm -f "$TMP_MACHINEFILE"
echo "Test completato. Risultati salvati in $OUTPUT_FILE"
