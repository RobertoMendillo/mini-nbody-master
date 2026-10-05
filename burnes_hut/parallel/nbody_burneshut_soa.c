#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#include "burneshut_functions.h"
#include "data_structures.h"
#include "support_functions.h"

#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
#include "papi_helper.h"
#endif

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
    // =========================================================================
    int* recvcounts = malloc(size * sizeof(int));
    int* displs = malloc(size * sizeof(int));

    int remainder = numBodies % size;
    int offset = 0;

    int i;
    for (i = 0; i < size; i++) {
        int count = numBodies / size + (i < remainder ? 1 : 0);

        // Per il SoA, usiamo direttamente il numero di elementi per MPI_FLOAT
        recvcounts[i] = count;
        displs[i] = offset;

        offset += count;
    }

    int my_start = displs[rank];
    int my_count = recvcounts[rank];
    int my_end = my_start + my_count;

#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
    Papi_Monitor* papi_monitor = malloc(sizeof(Papi_Monitor));
    if (papi_monitor == NULL) {
        fprintf(stderr, "[Rank %d] Errore allocazione memoria papi_monitor\n", rank);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    papi_helper_init(papi_monitor);
    papi_helper_start(papi_monitor);
#endif

    // =========================================================================
    // 2. INIZIALIZZAZIONE DATI (SoA)
    // =========================================================================
    BodiesSOA bodies = createBodiesSOA(numBodies);

    if (rank == MAIN_PROC) {
        // NOTA: Dovrai aggiornare anche randomizeBodies per accettare &bodies
        randomizeBodiesSOA(&bodies, numBodies);
    }

    // Il processo MAIN distribuisce lo stato iniziale: trasmettiamo gli array separatamente
    MPI_Bcast(bodies.x, numBodies, MPI_FLOAT, MAIN_PROC, MPI_COMM_WORLD);
    MPI_Bcast(bodies.y, numBodies, MPI_FLOAT, MAIN_PROC, MPI_COMM_WORLD);
    MPI_Bcast(bodies.z, numBodies, MPI_FLOAT, MAIN_PROC, MPI_COMM_WORLD);
    MPI_Bcast(bodies.vx, numBodies, MPI_FLOAT, MAIN_PROC, MPI_COMM_WORLD);
    MPI_Bcast(bodies.vy, numBodies, MPI_FLOAT, MAIN_PROC, MPI_COMM_WORLD);
    MPI_Bcast(bodies.vz, numBodies, MPI_FLOAT, MAIN_PROC, MPI_COMM_WORLD);
    MPI_Bcast(bodies.m, numBodies, MPI_FLOAT, MAIN_PROC, MPI_COMM_WORLD);

    initOctreePoolSOA(numBodies);

    MPI_Barrier(MPI_COMM_WORLD);


    // =========================================================================
    // 3. CICLO DI SIMULAZIONE
    // =========================================================================
    int step;
    for (step = 0; step < nIters; step++) {

        t0 = MPI_Wtime();
        // A. Costruisce l'albero spaziale SoA
        OctreeNodeSOA* root = buildOctreeSOA(&bodies, numBodies);

        // B. Calcola i centri di massa
        computeCentersOfMassSOA(&bodies);

        // C. Calcola le forze e aggiorna posizioni (SIMD + OMP)
        updatePhysicsWithIndexVectorized(&bodies, my_start, my_end, root, theta, dt);

        t1 = MPI_Wtime();
        total_cpu_time += t1 - t0;
        // D. Sincronizzazione: tutti i nodi si scambiano le posizioni aggiornate
        net_t0 = MPI_Wtime();

        // Raccogliamo separatamente le coordinate e le velocità
        MPI_Allgatherv(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, bodies.x, recvcounts, displs, MPI_FLOAT, MPI_COMM_WORLD);
        MPI_Allgatherv(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, bodies.y, recvcounts, displs, MPI_FLOAT, MPI_COMM_WORLD);
        MPI_Allgatherv(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, bodies.z, recvcounts, displs, MPI_FLOAT, MPI_COMM_WORLD);
        MPI_Allgatherv(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, bodies.vx, recvcounts, displs, MPI_FLOAT, MPI_COMM_WORLD);
        MPI_Allgatherv(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, bodies.vy, recvcounts, displs, MPI_FLOAT, MPI_COMM_WORLD);
        MPI_Allgatherv(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, bodies.vz, recvcounts, displs, MPI_FLOAT, MPI_COMM_WORLD);
        // Non serve raccogliere la massa (bodies.m) perché non cambia mai!

        net_t1 = MPI_Wtime();
        total_net_time += (net_t1 - net_t0);
    }


    long long cacheMissL1 = 0LL;
    long long cacheMissL2 = 0LL;
#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
    papi_helper_stop(papi_monitor);
    cacheMissL1 = papi_get_values(papi_monitor, L1_CACHE_MISS_INDEX);
    cacheMissL2 = papi_get_values(papi_monitor, L2_CACHE_MISS_INDEX);

    papi_helper_destroy(papi_monitor);
    free(papi_monitor);
#endif

    if (rank == MAIN_PROC) {
        printf("%d,%.4f,%.4f,%lld,%lld\n", numBodies, total_cpu_time, total_net_time, cacheMissL1, cacheMissL2);
    }

    free(recvcounts);
    free(displs);
    freeOctreePoolSOA();
    freeBodiesSOA(&bodies);

    MPI_Finalize();
    return 0;
}