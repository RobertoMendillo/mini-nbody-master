#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#include "../data_structures.h"
#include "../burneshut_functions.h"
#include "../support_functions.h"

// #if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
// #include "papi_helper.h"
// #endif

#define MAIN_PROC 0
/*
  Command line arguments:
    [1] --> number of bodies:
  default 30.000 [2] --> simulation
  iterations: default 10 [3] --> time
  step: default 0.01
*/
int main(int argc, char** argv) {
    // number of bodies in the
    // simulation
    int numBodies = 30000;
    // reading number of bodies as
    // command line argument
    if (argc > 1) numBodies = atoi(argv[1]);

    int nIters = 10;  // simulation iterations
    if (argc > 2) nIters = atoi(argv[2]);
    float dt = 0.01f;  // time step
    if (argc > 3) dt = atof(argv[3]);

    float theta = 0.5f;

    // MPI ========
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

// #if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
//     Papi_Monitor* papi_monitor = malloc(sizeof(Papi_Monitor));
//     if (papi_monitor == NULL) {
//         fprintf(stderr, "[Rank %d] Errore allocazione memoria papi_monitor\n", rank);
//         MPI_Abort(MPI_COMM_WORLD, -1);
//     }
//     papi_helper_init(papi_monitor);
//     papi_helper_start(papi_monitor);
// #endif

    double total_cpu_time = 0.0;  // computational time
    double t0, t1;

    Body* bodies = malloc(numBodies * sizeof(Body));
    randomizeBodies(bodies, numBodies);

    initOctreePool(numBodies);

    t0 = MPI_Wtime();
    // Ciclo di simulazione (es. 1000 step)
    for (int step = 0; step < nIters; step++) {
        // 1. Costruisce l'albero spaziale (resettando automaticamente il pool)
        OctreeNode* root = buildOctree(bodies, numBodies);

        // 2. Calcola i centri di massa dal basso verso l'alto
        computeCentersOfMass();

        // 3. Calcola le forze e aggiorna posizioni e velocità
        updatePhysics(bodies, numBodies, root, theta, dt);
    }
    t1 = MPI_Wtime();
    total_cpu_time = t1-t0;

    long long cacheMissL1 = 0LL;
    long long cacheMissL2 = 0LL;
// #if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
//     papi_helper_stop(papi_monitor);
//     cacheMissL1 = papi_get_values(papi_monitor, L1_CACHE_MISS_INDEX);
//     cacheMissL2 = papi_get_values(papi_monitor, L2_CACHE_MISS_INDEX);
//
//     papi_helper_destroy(papi_monitor);
//     free(papi_monitor);
// #endif

    printf("%d,%.2f,%lld,%lld\n", numBodies, total_cpu_time, cacheMissL1, cacheMissL2);

    MPI_Finalize();

    freeOctreePool();
    free(bodies);

    return 0;
}
