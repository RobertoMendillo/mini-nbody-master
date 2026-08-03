#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#include "timer.h"

#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
#include "papi_helper.h"
#endif

#define SOFTENING 1e-9f
#define MAIN_PROC 0

typedef struct {
    float *x, *y, *z, *vx, *vy, *vz;
} BodySystem;

void randomizeBodies(BodySystem* bodies, int n);
void bodyForce(BodySystem p, float dt, int n, BodySystem localBuffer, int blocksize);

int main(int argc, char** argv) {
    int nBodies = 30000;
    // reading number of bodies as
    // command line argument
    if (argc > 1) nBodies = atoi(argv[1]);

    int nIters = 10;  // simulation iterations
    if (argc > 2) nIters = atoi(argv[2]);
    float dt = 0.01f;  // time step
    if (argc > 3) dt = atof(argv[3]);

    int body_size = 6 * sizeof(float);

    int bytes = nBodies * body_size;
    float* global_buffer = (float*)malloc(bytes);
    BodySystem bodysystem_global;
    bodysystem_global.x = global_buffer + 0 * nBodies;
    bodysystem_global.y = global_buffer + 1 * nBodies;
    bodysystem_global.z = global_buffer + 2 * nBodies;
    bodysystem_global.vx = global_buffer + 3 * nBodies;
    bodysystem_global.vy = global_buffer + 4 * nBodies;
    bodysystem_global.vz = global_buffer + 5 * nBodies;

    // MPI ========
    int rank, size, i;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int blockSize = nBodies / size;
    int blockRemainder = nBodies % size;

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
        StartTimer();
    }

    int local_capacity = blockSize + blockRemainder;
    float* local_buffer = (float*)malloc(local_capacity * body_size);
    BodySystem bodysystem_local;
    bodysystem_local.x = local_buffer + 0 * local_capacity;
    bodysystem_local.y = local_buffer + 1 * local_capacity;
    bodysystem_local.z = local_buffer + 2 * local_capacity;
    bodysystem_local.vx = local_buffer + 3 * local_capacity;
    bodysystem_local.vy = local_buffer + 4 * local_capacity;
    bodysystem_local.vz = local_buffer + 5 * local_capacity;

    if (rank == MAIN_PROC) {
        printf(
            "Running simulation of %d "
            "bodies on %d iterations with "
            "time step of "
            "%.2f on %d nodes\n",
            nBodies, nIters, dt, size);

        randomizeBodies((BodySystem*)global_buffer, nBodies);  // Init position, velocity, mass
    }

    MPI_Scatter(global_buffer, body_size * blockSize, MPI_BYTE, local_buffer, body_size * blockSize, MPI_BYTE,
                MAIN_PROC, MPI_COMM_WORLD);

    double totalTime = 0.0;
    MPI_Barrier(MPI_COMM_WORLD);

    int iter;
    for (iter = 1; iter <= nIters; iter++) {
        MPI_Allgather(local_buffer, body_size * blockSize, MPI_BYTE, global_buffer, body_size * blockSize, MPI_BYTE,
                      MPI_COMM_WORLD);

        bodyForce(bodysystem_global, dt, nBodies, bodysystem_local, blockSize);  // compute interbody forces

        int i;
#pragma omp parallel for schedule(static)
        for (i = 0; i < blockSize; i++) {  // integrate local positions
            bodysystem_local.x[i] += bodysystem_local.vx[i] * dt;
            bodysystem_local.y[i] += bodysystem_local.vy[i] * dt;
            bodysystem_local.z[i] += bodysystem_local.vz[i] * dt;
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

void bodyForce(BodySystem p, float dt, int n, BodySystem localBuffer, int blocksize) {
    // Restrict pointers to tell compiler there is no aliasing
    const float* restrict px = p.x;
    const float* restrict py = p.y;
    const float* restrict pz = p.z;

    float* restrict lx = localBuffer.x;
    float* restrict ly = localBuffer.y;
    float* restrict lz = localBuffer.z;
    float* restrict lvx = localBuffer.vx;
    float* restrict lvy = localBuffer.vy;
    float* restrict lvz = localBuffer.vz;
    int i, j;
#pragma omp parallel for schedule(static)
    for (i = 0; i < blocksize; i++) {
        float Fx = 0.0f, Fy = 0.0f, Fz = 0.0f;

        float lxi = lx[i], lyi = ly[i], lzi = lz[i];

#pragma omp simd reduction(+ : Fx, Fy, Fz)
        for (j = 0; j < n; j++) {
            float dx = px[j] - lxi;
            float dy = py[j] - lyi;
            float dz = pz[j] - lzi;
            float distSqr = dx * dx + dy * dy + dz * dz + SOFTENING;
            float invDist = 1.0f / sqrtf(distSqr);
            float invDist3 = invDist * invDist * invDist;

            Fx += dx * invDist3;
            Fy += dy * invDist3;
            Fz += dz * invDist3;
        }

        lvx[i] += dt * Fx;
        lvy[i] += dt * Fy;
        lvz[i] += dt * Fz;
    }
}