#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#include "timer.h"

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

typedef struct {
    float x, y, z, vx, vy, vz, m;
} Body;
#define BODY_SIZE sizeof(Body)

void randomizeBodies(Body* data, int n);
void bodyForce(Body* p, float dt, int n, Body* localBuffer, int blocksize);
void exportBodies(Body* p, int n, int iter);

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

    int bytes = nBodies * sizeof(Body);
    Body* global_buffer = (Body*)malloc(bytes);

    // MPI ========
    int rank,
        size,  // indica il numero di
               // processi in
               // comunicazione
        i;
    MPI_Init(&argc,
             &argv);  // inizializza la
                      // comunicazione
                      // con MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int blockSize = nBodies / size;
    int blockRemainder = nBodies % size;

#ifdef DEBUG
    printf("#%d Memory allocation for local buffer ... ", rank);
#endif
    Body* local_buffer = (Body*)malloc((blockSize + blockRemainder) * sizeof(Body));
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
        printf(
            "Running simulation of %d "
            "bodies on %d iterations with "
            "time step of "
            "%.2f on %d nodes\n",
            nBodies, nIters, dt, size);

#ifdef DEBUG
        printf("Randomizing bodies ...");
#endif
        randomizeBodies(global_buffer, nBodies);  // Init position, velocity, mass
#ifdef DEBUG
        printf("... done.\n");
#endif
    }

    // distribute blocks to all nodes
#ifdef DEBUG
    printf("distributing work...");
#endif
    MPI_Scatter(global_buffer, BODY_SIZE * blockSize, MPI_BYTE, local_buffer, BODY_SIZE * blockSize,
                MPI_BYTE, MAIN_PROC, MPI_COMM_WORLD);
#ifdef DEBUG
    printf("... done.\n");
#endif

    double totalTime = 0.0;  // simulation total execution time

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
        // raccogliamo lo stato attuale dei corpi
        MPI_Allgather(local_buffer, BODY_SIZE * blockSize, MPI_BYTE, global_buffer,
                      BODY_SIZE * blockSize, MPI_BYTE, MPI_COMM_WORLD);

        if (rank == MAIN_PROC) {
            printf("Iteration %d start ...", iter);
            StartTimer();
        }

#ifdef EXPORT
        if (rank == MAIN_PROC) exportBodies(global_buffer, nBodies, iter);
#endif

        bodyForce(global_buffer, dt, nBodies, local_buffer, blockSize);  // compute interbody forces

#pragma omp parallel for schedule(static)
        int i;
        for (i = 0; i < blockSize; i++) {  // integrate position
            local_buffer[i].x += local_buffer[i].vx * dt;
            local_buffer[i].y += local_buffer[i].vy * dt;
            local_buffer[i].z += local_buffer[i].vz * dt;
        }

        double tElapsed;
        if (rank == MAIN_PROC) {
            tElapsed = GetTimer() / 1000.0;
            if (iter > 1) {  // First iter is warm up
                totalTime += tElapsed;
            }

#ifndef SHMOO
            printf(
                " ... %.3f "
                "seconds\n",
                tElapsed);
#endif
        }

    }  // end of iterations

    if (rank == MAIN_PROC) {
        double avgTime = totalTime / (double)(nIters - 1);

#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
#ifdef DEBUG
        printf("Stopping papi monitors ...\n");
#endif
        papi_helper_stop(papi_monitor);
#ifdef DEBUG
        printf("... stopped\n");
#endif
        papi_helper_print(papi_monitor);
#endif

#ifdef SHMOO
        printf("%d, %0.3f\n", nBodies, 1e-9 * nBodies * nBodies / avgTime);
#else
        printf(
            "Average rate for iterations 2 "
            "through %d: %.3f steps per "
            "second, %.3f "
            "average per iteration.\n",
            nIters, (float)nIters / totalTime, avgTime);
        printf(
            "%d Bodies: average %0.3f "
            "Billion Interactions / "
            "second\n",
            nBodies, 1e-9 * nBodies * nBodies / avgTime);
#endif
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
#pragma omp parallel for schedule(dynamic)
    int i;
    int j;
    for (i = 0; i < blocksize; i++) {
        // total force on every axis
        // applied by every other body
        float Fx = 0.0f;
        float Fy = 0.0f;
        float Fz = 0.0f;

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

void exportBodies(Body* p, int n, int iter) {
    // Apre il file in modalità "append"
    // (scrive in coda al file
    // esistente)
    FILE* f = fopen("simulation_data.csv", "a");
    if (f == NULL) {
        printf(
            "Errore nell'apertura del "
            "file!\n");
        return;
    }

    // Se è la primissima iterazione,
    // scrive l'intestazione delle
    // colonne (header)
    if (iter == 1) {
        fprintf(f,
                "iteration,body_id,x,y,"
                "z\n");
    }

    // Scrive i dati di ogni corpo
    int i;
    for (i = 0; i < n; i++) {
        fprintf(f, "%d,%d,%.5f,%.5f,%.5f\n", iter, i, p[i].x, p[i].y, p[i].z);
    }

    fclose(f);  // Chiude il file per
                // salvare i dati sul disco
}
