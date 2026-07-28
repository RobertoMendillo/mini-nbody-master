#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#include "timer.h"

#define SOFTENING 1e-9f

typedef struct {
    float *x, *y, *z, *vx, *vy, *vz;
} BodySystem;

void randomizeBodies(BodySystem* bodies, int n);
void bodyForce(BodySystem p, float dt, int n);

int main(const int argc, const char** argv) {
    int nBodies = 30000;
    // reading number of bodies as
    // command line argument
    if (argc > 1) nBodies = atoi(argv[1]);

    int nIters = 10;  // simulation iterations
    if (argc > 2) nIters = atoi(argv[2]);
    float dt = 0.01f;  // time step
    if (argc > 3) dt = atof(argv[3]);

    const float dt = 0.01f;  // time step
    const int nIters = 10;   // simulation iterations

    const BODY_SIZE = 6 * sizeof(float);

    int bytes = nBodies * BODY_SIZE;
    float* global_buffer = (float*)malloc(bytes);
    BodySystem p;
    p.x = global_buffer + 0 * nBodies;
    p.y = global_buffer + 1 * nBodies;
    p.z = global_buffer + 2 * nBodies;
    p.vx = global_buffer + 3 * nBodies;
    p.vy = global_buffer + 4 * nBodies;
    p.vz = global_buffer + 5 * nBodies;

    // MPI ========
    int rank, size, i;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int blockSize = nBodies / size;
    int blockRemainder = nBodies % size;

    if (rank == MAIN_PROC) {
        StartTimer();
    }

    BodySystem* local_buffer = (BodySystem*)malloc((blockSize + blockRemainder) * BODY_SIZE);

    if (rank == MAIN_PROC) {
        printf(
            "Running simulation of %d "
            "bodies on %d iterations with "
            "time step of "
            "%.2f on %d nodes\n",
            nBodies, nIters, dt, size);

        randomizeBodies(global_buffer, nBodies);  // Init position, velocity, mass
    }

    MPI_Scatter(global_buffer, BODY_SIZE * blockSize, MPI_BYTE, local_buffer, BODY_SIZE * blockSize, MPI_BYTE,
                MAIN_PROC, MPI_COMM_WORLD);

    double totalTime = 0.0;
    MPI_Barrier(MPI_COMM_WORLD);

    int iter;
    for (iter = 1; iter <= nIters; iter++) {
        MPI_Allgather(local_buffer, BODY_SIZE * blockSize, MPI_BYTE, global_buffer, BODY_SIZE * blockSize, MPI_BYTE,
                      MPI_COMM_WORLD);

        bodyForce(global_buffer, dt, nBodies, local_buffer, blockSize);  // compute interbody forces

#pragma omp parallel for schedule(static)
        for (int i = 0; i < nBodies; i++) {  // integrate position
            p.x[i] += p.vx[i] * dt;
            p.y[i] += p.vy[i] * dt;
            p.z[i] += p.vz[i] * dt;
        }

        const double tElapsed = GetTimer() / 1000.0;
    }
    totalTime = GetTimer();
    double avgTime = totalTime / (double)(nIters - 1);

    if (rank == MAIN_PROC) {
        totalTime = GetTimer();
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

        // printf(
        //     "Average rate for iterations 2 "
        //     "through %d: %.3f steps per "
        //     "second, %.3f "
        //     "average per iteration.\n",
        //     nIters, (float)nIters / totalTime, avgTime);
        // printf(
        //     "%d Bodies: average %0.3f "
        //     "Billion Interactions / "
        //     "second\n",
        //     nBodies, 1e-9 * nBodies * nBodies / avgTime);

        int minutes = ((int)totalTime) / 60.0;
        int seconds = ((int)totalTime % 60);

        printf("Duration of simulation: %d m %d s\n", minutes, seconds);
    }
    free(global_buffer);
    free(local_buffer);
    MPI_Finalize();
}

void randomizeBodies(BodySystem* bodies, int n) {
    int i;
    float* data = (float*)bodies;
    for (i = 0; i < n; i++) {
        data[i] = 2.0f * (rand() / (float)RAND_MAX) - 1.0f;
    }
}

void bodyForce(BodySystem p, float dt, int n, BodySystem* localBuffer, int blocksize) {
    int i, j;
#pragma omp parallel for schedule(dynamic) private(j)
    for (i = 0; i < blocksize; i++) {
        float Fx = 0.0f;
        float Fy = 0.0f;
        float Fz = 0.0f;

#pragma omp simd reduction(+ : Fx, Fy, Fz)
        for (j = 0; j < n; j++) {
            float dy = p.y[j] - localBuffer.y[i];
            float dz = p.z[j] - localBuffer.z[i];
            float dx = p.x[j] - localBuffer.x[i];
            float distSqr = dx * dx + dy * dy + dz * dz + SOFTENING;
            float invDist = 1.0f / sqrtf(distSqr);
            float invDist3 = invDist * invDist * invDist;

            Fx += dx * invDist3;
            Fy += dy * invDist3;
            Fz += dz * invDist3;
        }

        localBuffer.vx[i] += dt * Fx;
        localBuffer.vy[i] += dt * Fy;
        localBuffer.vz[i] += dt * Fz;
    }
}