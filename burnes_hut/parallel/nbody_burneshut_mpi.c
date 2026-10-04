#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#include "../burneshut_functions.h"
#include "../data_structures.h"
#include "../support_functions.h"

#define MAIN_PROC 0

int main(int argc, char** argv) {
    int numBodies = 30000;
    if (argc > 1) numBodies = atoi(argv[1]);

    int nIters = 10;
    if (argc > 2) nIters = atoi(argv[2]);

    float dt = 0.01f;
    if (argc > 3) dt = atof(argv[3]);

    float theta = 0.5f;

    // MPI ========
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    double total_cpu_time = 0.0;
    double total_net_time = 0.0;
    double t0, t1, net_t0, net_t1;

    // =========================================================================
    // 1. PARTIZIONAMENTO DEL LAVORO
    // Calcoliamo quanti corpi deve gestire ogni processo e gli offset
    // =========================================================================
    int* recvcounts = malloc(size * sizeof(int));
    int* displs = malloc(size * sizeof(int));

    int remainder = numBodies % size;
    int offset = 0;

    int i;
    for (i = 0; i < size; i++) {
        // I primi 'remainder' processi prendono un corpo in più
        int count = numBodies / size + (i < remainder ? 1 : 0);

        // Calcoliamo i byte per MPI_Allgatherv
        recvcounts[i] = count * sizeof(Body);
        displs[i] = offset * sizeof(Body);

        offset += count;
    }

    // Indici utili per il processo corrente (non in byte, ma in indici array)
    int my_start = displs[rank] / sizeof(Body);
    int my_count = recvcounts[rank] / sizeof(Body);
    int my_end = my_start + my_count;

    // =========================================================================
    // 2. INIZIALIZZAZIONE DATI
    // =========================================================================
    Body* bodies = malloc(numBodies * sizeof(Body));

    if (rank == MAIN_PROC) {
        randomizeBodies(bodies, numBodies);
    }

    // Il processo MAIN distribuisce lo stato iniziale a tutti gli altri nodi
    MPI_Bcast(bodies, numBodies * sizeof(Body), MPI_BYTE, MAIN_PROC, MPI_COMM_WORLD);

    initOctreePool(numBodies);

    MPI_Barrier(MPI_COMM_WORLD);  // Sincronizzazione prima di far partire i timer
    t0 = MPI_Wtime();

    // =========================================================================
    // 3. CICLO DI SIMULAZIONE
    // =========================================================================
    int step;
    for (step = 0; step < nIters; step++) {
        // A. Costruisce l'albero spaziale per TUTTI i corpi (avviene in parallelo su ogni nodo)
        OctreeNode* root = buildOctree(bodies, numBodies);

        // B. Calcola i centri di massa dal basso verso l'alto
        computeCentersOfMass();

        // C. Calcola le forze e aggiorna posizioni SOLO per la propria porzione (my_start -> my_end)
        // [!] ATTENZIONE: Devi modificare updatePhysics per accettare my_start e my_end
        updatePhysicsWithIndex(bodies, my_start, my_end, root, theta, dt);

        // D. Sincronizzazione: tutti i nodi si scambiano le posizioni aggiornate
        net_t0 = MPI_Wtime();

        // Usiamo MPI_IN_PLACE perché i dati aggiornati dal rank corrente
        // si trovano già nella corretta posizione nell'array 'bodies'
        MPI_Allgatherv(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, bodies, recvcounts, displs, MPI_BYTE, MPI_COMM_WORLD);

        net_t1 = MPI_Wtime();
        total_net_time += (net_t1 - net_t0);
    }
    t1 = MPI_Wtime();
    total_cpu_time += t1 - t0;

    long long cacheMissL1 = 0LL;
    long long cacheMissL2 = 0LL;

    // Output stampato solo dal processo principale
    if (rank == MAIN_PROC) {
        printf("%d,%.4f,%.4f,%lld,%lld\n", numBodies, total_cpu_time, total_net_time, cacheMissL1, cacheMissL2);
        // printf("Tempo comunicazione (Network): %.4f\n", total_net_time);
    }

    free(recvcounts);
    free(displs);
    freeOctreePool();
    free(bodies);

    MPI_Finalize();
    return 0;
}