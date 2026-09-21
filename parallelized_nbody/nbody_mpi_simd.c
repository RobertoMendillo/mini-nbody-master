#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef EXPORT
#include "utils.h"
#endif

#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
#include "papi_helper.h"
#endif

#define SOFTENING \
    1e-9f  // needed to avoid distance
           // equal to zero
#define G \
    6.67e-11f  // universal
               // gravitational constant
#define MAIN_PROC 0

#define BODY_SIZE sizeof(Body)

typedef struct {
    float x, y, z, vx, vy, vz, m;
} Body;


static void randomizeBodies(Body* data, int n);
static void bodyForce(Body* p, float dt, int n, Body* localBuffer, int blocksize);

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
    int nBodies = 30000;
    // reading number of bodies as
    // command line argument
    if (argc > 1) nBodies = atoi(argv[1]);

    int nIters = 10;  // simulation iterations
    if (argc > 2) nIters = atoi(argv[2]);
    float dt = 0.01f;  // time step
    if (argc > 3) dt = atof(argv[3]);

    // MPI ========
    int rank, size, i;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // let nbodies be a round number so that the division is an integer
    nBodies = nBodies - (nBodies % size);
    const int blockSize = nBodies / size;

    int bytes = nBodies * sizeof(Body);
    Body *global_buffer = malloc(bytes);

    double total_net_time = 0.0; // communication time
    double total_cpu_time = 0.0; // computational time
    double t0, t1;

#ifdef DEBUG
    printf("#%d Memory allocation for local buffer ... ", rank);
#endif


    Body* local_buffer = (Body*)malloc(blockSize * sizeof(Body));


#ifdef DEBUG
    printf(" ... done.\n");
#endif
    // ============

#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
    Papi_Monitor* papi_monitor;
    if (rank == MAIN_PROC) {
        papi_monitor = malloc(sizeof(Papi_Monitor));
#ifdef DEBUG
        printf("Init papi monitors ...\n");
#endif

        papi_helper_init(papi_monitor);

#ifdef DEBUG
        printf("... completed\n");
#endif
    }
#endif

    if (rank == MAIN_PROC) {

#ifdef DEBUG
        printf("Randomizing bodies ...");
#endif


        t0 = MPI_Wtime();
        randomizeBodies(global_buffer, nBodies); // Init position, velocity, mass
        t1 = MPI_Wtime();
        total_cpu_time += (t1 - t0);


#ifdef DEBUG
        printf("... done.\n");
#endif
    }

    // distribute blocks to all nodes
#ifdef DEBUG
    printf("distributing work...");
#endif


    t0 = MPI_Wtime();
    MPI_Scatter(global_buffer, BODY_SIZE * blockSize, MPI_BYTE, local_buffer, BODY_SIZE * blockSize, MPI_BYTE,
                MAIN_PROC, MPI_COMM_WORLD);
    t1 = MPI_Wtime();
    total_net_time += (t1 - t0);


#ifdef DEBUG
    printf("... done.\n");
#endif

#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
    if (rank == MAIN_PROC) {
#ifdef DEBUG
        printf("Starting papi monitors ...\n");
#endif
        papi_helper_start(papi_monitor);
#ifdef DEBUG
        printf("... started\n");
#endif
    }
#endif

    MPI_Barrier(MPI_COMM_WORLD);
    int iter;
    for (iter = 1; iter <= nIters; iter++) {
        t0 = MPI_Wtime();
        // raccogliamo lo stato attuale dei corpi
        MPI_Allgather(local_buffer, BODY_SIZE * blockSize, MPI_BYTE, global_buffer, BODY_SIZE * blockSize, MPI_BYTE,
                      MPI_COMM_WORLD);
        t1 = MPI_Wtime();
        total_net_time += t1 - t0;

#ifdef EXPORT
        if (rank == MAIN_PROC) exportBodies(global_buffer, nBodies, iter);
#endif

        t0 = MPI_Wtime();
        bodyForce(global_buffer, dt, nBodies, local_buffer, blockSize);  // compute interbody forces
        t1 = MPI_Wtime();
        total_cpu_time += t1 - t0;

        int i;
        t0 = MPI_Wtime();
#pragma omp parallel for schedule(static)
        for (i = 0; i < blockSize; i++) {  // integrate position
            local_buffer[i].x += local_buffer[i].vx * dt;
            local_buffer[i].y += local_buffer[i].vy * dt;
            local_buffer[i].z += local_buffer[i].vz * dt;
        }
        t1 = MPI_Wtime();
        total_cpu_time += t1 - t0;

    }  // end of iterations

    if (rank == MAIN_PROC) {
        double totalTime = total_net_time + total_cpu_time; // elapsed time in seconds
        double avgTime = totalTime / (double) (nIters - 1);

#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
#ifdef DEBUG
        printf("Stopping papi monitors ...\n");
#endif
        papi_helper_stop(papi_monitor);
#ifdef DEBUG
        printf("... stopped\n");
#endif
        // papi_helper_print(papi_monitor);
        free(papi_monitor);
#endif


        printf("%d,%d,%.4f,%.4f,%.4f\n", size, nBodies, total_cpu_time, total_net_time, totalTime);

    }
    free(global_buffer);
    free(local_buffer);
    MPI_Finalize();
}

// sets up the bodies with random
// position, velocity and mass
void randomizeBodies(Body* bodies, int n) {
    int i;
    for (i = 0; i < n; i++) {
        bodies[i].x = 2.0f * (rand() / (float)RAND_MAX) - 1.0f;
        bodies[i].y = 2.0f * (rand() / (float)RAND_MAX) - 1.0f;
        bodies[i].z = 2.0f * (rand() / (float)RAND_MAX) - 1.0f;
        bodies[i].vx = 2.0f * (rand() / (float)RAND_MAX) - 1.0f;
        bodies[i].vy = 2.0f * (rand() / (float)RAND_MAX) - 1.0f;
        bodies[i].vz = 2.0f * (rand() / (float)RAND_MAX) - 1.0f;
        bodies[i].m = (rand() / (float)RAND_MAX) * 100;  // set mass to a positive number
    }
}

// computes interbody forces assuming
// mass of bodies equal to 1
void bodyForce(Body* p, float dt, int n, Body* localBuffer, int blocksize) {
    int i;
    int j;
#pragma omp parallel for schedule(dynamic) private(j)
    for (i = 0; i < blocksize; i++) {
        // total force on every axis
        // applied by every other body
        float Fx = 0.0f;
        float Fy = 0.0f;
        float Fz = 0.0f;

#pragma omp simd reduction(+ : Fx, Fy, Fz)
        for (j = 0; j < n; j++) {
            float dx = p[j].x - localBuffer[i].x;  // distance on x axis
            float dy = p[j].y - localBuffer[i].y;  // distance on y axis
            float dz = p[j].z - localBuffer[i].z;  // distance on z axis

            // compute force on every
            // direction F = 1/r^2 * D/r
            // = D/r^3 D = (dx/r, dy/r,
            // dz/r)
            float distSqr = dx * dx + dy * dy + dz * dz + SOFTENING;  // total
                                                                      // distance
                                                                      // between
                                                                      // the two
                                                                      // bodies
            float invDist = 1.0f / sqrtf(distSqr);                    // -->
                                                                      // 1/r
            float invDist3 = invDist * invDist * invDist;             // --> 1/r^3

            float massProduct = p[j].m * G;

            // to optimize calculations,
            // this is actually an
            // acceleration because it
            // is already divided by
            // mass (avoid division by
            // mass later)
            Fx += dx * invDist3 * massProduct;  // component
                                                // of
                                                // force
                                                // with
                                                // respect
                                                // to x
                                                // (dx /
                                                // r^3)
            Fy += dy * invDist3 * massProduct;  // component
                                                // of
                                                // force
                                                // with
                                                // respect
                                                // to y
                                                // (dy /
                                                // r^3)
            Fz += dz * invDist3 * massProduct;  // component
                                                // of
                                                // force
                                                // with
                                                // respect
                                                // to z
                                                // (dz /
                                                // r^3)
        }

        // compute velocity on every
        // direction
        localBuffer[i].vx += dt * Fx;  // velocity on x axis
        localBuffer[i].vy += dt * Fy;  // velocity on y axis
        localBuffer[i].vz += dt * Fz;  // velocity on z axis
    }
}
