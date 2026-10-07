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
    int* body_counts = malloc(size * sizeof(int));
    int* body_displs = malloc(size * sizeof(int));
    int* mpi_counts  = malloc(size * sizeof(int)); // 3 * body_counts (x, y, z)
    int* mpi_displs  = malloc(size * sizeof(int)); // 3 * body_displs

    int remainder = numBodies % size;
    int offset = 0;

    int i;
    for (i = 0; i < size; i++) {
        int count = numBodies / size + (i < remainder ? 1 : 0);

        body_counts[i] = count;
        body_displs[i] = offset;

        // Impacchettiamo (x, y, z) insieme: ogni corpo richiede 3 float
        mpi_counts[i] = count * 3;
        mpi_displs[i] = offset * 3;

        offset += count;
    }

    int my_start = body_displs[rank];
    int my_count = body_counts[rank];
    int my_end   = my_start + my_count;

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
        randomizeBodiesSOA(&bodies, numBodies);
    }

    // Distribuzione iniziale
    MPI_Bcast(bodies.x, numBodies, MPI_FLOAT, MAIN_PROC, MPI_COMM_WORLD);
    MPI_Bcast(bodies.y, numBodies, MPI_FLOAT, MAIN_PROC, MPI_COMM_WORLD);
    MPI_Bcast(bodies.z, numBodies, MPI_FLOAT, MAIN_PROC, MPI_COMM_WORLD);
    MPI_Bcast(bodies.vx, numBodies, MPI_FLOAT, MAIN_PROC, MPI_COMM_WORLD);
    MPI_Bcast(bodies.vy, numBodies, MPI_FLOAT, MAIN_PROC, MPI_COMM_WORLD);
    MPI_Bcast(bodies.vz, numBodies, MPI_FLOAT, MAIN_PROC, MPI_COMM_WORLD);
    MPI_Bcast(bodies.m,  numBodies, MPI_FLOAT, MAIN_PROC, MPI_COMM_WORLD);

    initOctreePoolSOA(numBodies);

    // =========================================================================
    // 3. BUFFER AUSILIARI (Allocati una sola volta fuori dal ciclo)
    // =========================================================================
    float* force_x = (float*)malloc(my_count * sizeof(float));
    float* force_y = (float*)malloc(my_count * sizeof(float));
    float* force_z = (float*)malloc(my_count * sizeof(float));

    // Buffer locale per inviare [x, y, z] e globale per ricevere tutto con UNA SOLA Allgatherv
    float* send_coords_buf = (float*)malloc(my_count * 3 * sizeof(float));
    float* recv_coords_buf = (float*)malloc(numBodies * 3 * sizeof(float));

    MPI_Barrier(MPI_COMM_WORLD);

    // =========================================================================
    // 4. CICLO DI SIMULAZIONE
    // =========================================================================
    int step;
    for (step = 0; step < nIters; step++) {
        t0 = MPI_Wtime();

        // A. Costruzione dell'albero spaziale SoA
        OctreeNodeSOA* root = buildOctreeSOA(&bodies, numBodies);

        // B. Calcolo centri di massa
        computeCentersOfMassSOA(&bodies);

        // C. Calcolo forze e aggiornamento posizioni (SIMD + OMP)
        updatePhysicsWithIndexVectorized(&bodies, my_start, my_end, root, theta, dt, force_x, force_y, force_z);

        t1 = MPI_Wtime();
        total_cpu_time += (t1 - t0);

        // D. Sincronizzazione Rete
        net_t0 = MPI_Wtime();

        // 1) Pack locale: copiamo x, y, z in send_coords_buf (contiguo)
        int i;
        #pragma omp parallel for schedule(static)
        for (i = 0; i < my_count; i++) {
            int global_i = my_start + i;
            send_coords_buf[i * 3 + 0] = bodies.x[global_i];
            send_coords_buf[i * 3 + 1] = bodies.y[global_i];
            send_coords_buf[i * 3 + 2] = bodies.z[global_i];
        }

        // 2) UNICA CHIAMATA DI RETE per tutti i dati e tutti i nodi
        MPI_Allgatherv(send_coords_buf, my_count * 3, MPI_FLOAT,
                       recv_coords_buf, mpi_counts, mpi_displs, MPI_FLOAT,
                       MPI_COMM_WORLD);

        // 3) Unpack globale: ripopoliamo i vettori SoA (x, y, z) per il prossimo step
        int i;
        #pragma omp parallel for schedule(static)
        for (i = 0; i < numBodies; i++) {
            bodies.x[i] = recv_coords_buf[i * 3 + 0];
            bodies.y[i] = recv_coords_buf[i * 3 + 1];
            bodies.z[i] = recv_coords_buf[i * 3 + 2];
        }

        net_t1 = MPI_Wtime();
        total_net_time += (net_t1 - net_t0);
    }

    // Deallocazione buffer ausiliari
    free(force_x);
    free(force_y);
    free(force_z);
    free(send_coords_buf);
    free(recv_coords_buf);

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

    free(body_counts);
    free(body_displs);
    free(mpi_counts);
    free(mpi_displs);
    freeOctreePoolSOA();
    freeBodiesSOA(&bodies);

    MPI_Finalize();
    return 0;
}