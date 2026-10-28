#!/bin/bash
# Weak Scaling al variare del NUMERO DI NODI (corpi proporzionali ai nodi, nodi 1..MAX_NODES)
# Modello ibrido: 1 processo MPI per nodo, THREADS_PER_NODE thread OpenMP
#
# Uso: ./weak.sh [thread_per_nodo] [aos|soa]   (default: 16 aos)
THREADS_PER_NODE=${1:-16}
VARIANT=${2:-aos}

if ! [[ "$THREADS_PER_NODE" =~ ^[1-9][0-9]*$ ]]; then
    echo "Errore: il numero di thread per nodo deve essere un intero positivo (ricevuto: '$THREADS_PER_NODE')" >&2
    exit 1
fi

if [ "$VARIANT" != "aos" ] && [ "$VARIANT" != "soa" ]; then
    echo "Errore: la variante deve essere 'aos' o 'soa' (ricevuto: '$VARIANT')" >&2
    exit 1
fi

MAX_NODES=4
BODIES_PER_NODE=250000
MACHINEFILE="machinefile_p.txt"
OUT_DIR="burnes_hut/results/scaling/nodes/weak"
# AoS mantiene il nome storico (results_<t>t.data), SoA usa il prefisso soa_
if [ "$VARIANT" = "soa" ]; then
    SOURCEFILE="burnes_hut/parallel/nbody_burneshut_soa.c burnes_hut/burneshut_functions.c"
    CFLAGS="-O3 -mavx -ffast-math"
    OUTPUT_FILE="$OUT_DIR/results_soa_${THREADS_PER_NODE}t.data"
else
    SOURCEFILE="burnes_hut/parallel/nbody_burneshut_mpi.c burnes_hut/burneshut_functions.c"
    CFLAGS="-O3"
    OUTPUT_FILE="$OUT_DIR/results_${THREADS_PER_NODE}t.data"
fi
BINARY="burnes_hut/parallel/scaling/nbody_nodes_${VARIANT}.out"
TMP_MACHINEFILE="$OUT_DIR/.machinefile_tmp.txt"

mkdir -p "$OUT_DIR"

# Compilazione
echo "Compilazione in corso ($VARIANT)..."
mpicc $CFLAGS $SOURCEFILE ./papi/papi_helper.c -lm -I./papi/ -Iburnes_hut/ -fopenmp /usr/local/lib/libpapi.a -o "$BINARY"
echo "Compilazione completata. Inizio Weak Scaling sui nodi ($VARIANT, 1 proc/nodo, $THREADS_PER_NODE thread/nodo)."

# Intestazione CSV
echo "Nodes,OMP_Threads,Total_Cores,NumBodies,CPU_Time,Net_Time,L1_Miss,L2_Miss" > "$OUTPUT_FILE"

for nodes in $(seq 1 $MAX_NODES); do
    # Machinefile con i soli primi $nodes nodi, 1 slot ciascuno (1 processo MPI per nodo)
    head -n "$nodes" "$MACHINEFILE" | sed "s/\$/ slots=1/" > "$TMP_MACHINEFILE"

    TOTAL_CORES=$((nodes * THREADS_PER_NODE))
    BODIES=$((BODIES_PER_NODE * nodes))
    echo "Avvio Weak Scaling: Nodi=$nodes, OMP_NUM_THREADS=$THREADS_PER_NODE (Core totali: $TOTAL_CORES, Corpi: $BODIES)..."

    RESULT=$(mpirun -np $nodes --map-by node --bind-to none \
        -x OMP_NUM_THREADS=$THREADS_PER_NODE \
        --mca btl self,openib \
        -machinefile "$TMP_MACHINEFILE" \
        "$BINARY" "$BODIES")

    # Salva la riga nel CSV e la mostra a terminale
    echo "$nodes,$THREADS_PER_NODE,$TOTAL_CORES,$RESULT" | tee -a "$OUTPUT_FILE"
done

rm -f "$TMP_MACHINEFILE"
echo "Weak Scaling completato. Risultati salvati in $OUTPUT_FILE"
