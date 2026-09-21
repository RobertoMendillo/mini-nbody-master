#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
#include "papi_helper.h"
#endif

#define SOFTENING 1e-9f
#define G 6.67e-11f
#define MAIN_PROC 0

typedef struct {
    float *x, *y, *z, *vx, *vy, *vz, *m;
} BodySystem;

void randomizeBodies(float* data, int n);
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

    // MPI ========
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    nBodies = nBodies - (nBodies % size);
    const int blockSize = nBodies / size;

    double total_net_time = 0.0; // communication time
    double total_cpu_time = 0.0; // computational time
    double t0, t1;

    int global_floats = 7 * nBodies;
    float* global_buffer = (float*)malloc(global_floats * sizeof(float));

    BodySystem bodysystem_global;
    bodysystem_global.x = global_buffer + 0 * nBodies;
    bodysystem_global.y = global_buffer + 1 * nBodies;
    bodysystem_global.z = global_buffer + 2 * nBodies;
    bodysystem_global.vx = global_buffer + 3 * nBodies;
    bodysystem_global.vy = global_buffer + 4 * nBodies;
    bodysystem_global.vz = global_buffer + 5 * nBodies;
    bodysystem_global.m  = global_buffer + 6 * nBodies;

    int local_floats = 7 * blockSize;
    float* local_buffer = (float*)malloc(local_floats * sizeof(float));

    BodySystem bodysystem_local;
    bodysystem_local.x  = local_buffer + 0 * blockSize;
    bodysystem_local.y  = local_buffer + 1 * blockSize;
    bodysystem_local.z  = local_buffer + 2 * blockSize;
    bodysystem_local.vx = local_buffer + 3 * blockSize;
    bodysystem_local.vy = local_buffer + 4 * blockSize;
    bodysystem_local.vz = local_buffer + 5 * blockSize;
    bodysystem_local.m  = local_buffer + 6 * blockSize;

#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
    Papi_Monitor* papi_monitor;
    if (rank == MAIN_PROC) {
        papi_monitor = malloc(sizeof(Papi_Monitor));

        papi_helper_init(papi_monitor);
    }
#endif

    if (rank == MAIN_PROC) {
        t0 = MPI_Wtime();
        randomizeBodies(global_buffer, global_floats);  // Init position, velocity, mass
        t1 = MPI_Wtime();
        total_cpu_time += (t1 - t0);
    }


    // initial distribution of data
    t0 = MPI_Wtime();
    MPI_Scatter(bodysystem_global.x, blockSize, MPI_FLOAT, bodysystem_local.x, blockSize, MPI_FLOAT,
    MAIN_PROC, MPI_COMM_WORLD);
    MPI_Scatter(bodysystem_global.y, blockSize, MPI_FLOAT, bodysystem_local.y, blockSize, MPI_FLOAT,
    MAIN_PROC, MPI_COMM_WORLD);
    MPI_Scatter(bodysystem_global.z, blockSize, MPI_FLOAT, bodysystem_local.z, blockSize, MPI_FLOAT,
    MAIN_PROC, MPI_COMM_WORLD);
    MPI_Scatter(bodysystem_global.vx, blockSize, MPI_FLOAT, bodysystem_local.vx, blockSize, MPI_FLOAT,
    MAIN_PROC, MPI_COMM_WORLD);
    MPI_Scatter(bodysystem_global.vy, blockSize, MPI_FLOAT, bodysystem_local.vy, blockSize, MPI_FLOAT,
    MAIN_PROC, MPI_COMM_WORLD);
    MPI_Scatter(bodysystem_global.vz, blockSize, MPI_FLOAT, bodysystem_local.vz, blockSize, MPI_FLOAT,
                MAIN_PROC, MPI_COMM_WORLD);
    // mass is sent in broadcast to avoid useless traffic later
    MPI_Bcast(bodysystem_global.m, nBodies, MPI_FLOAT, MAIN_PROC, MPI_COMM_WORLD);
    t1 = MPI_Wtime();
    total_net_time += t1 - t0;


#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
    if (rank == MAIN_PROC) {
        papi_helper_start(papi_monitor);
    }
#endif

    MPI_Barrier(MPI_COMM_WORLD);
    int iter;
    for (iter = 1; iter <= nIters; iter++) {

        // synchronization
        t0 = MPI_Wtime();
        MPI_Allgather(bodysystem_local.x, blockSize, MPI_FLOAT, bodysystem_global.x, blockSize, MPI_FLOAT, MPI_COMM_WORLD);
        MPI_Allgather(bodysystem_local.y, blockSize, MPI_FLOAT, bodysystem_global.y, blockSize, MPI_FLOAT, MPI_COMM_WORLD);
        MPI_Allgather(bodysystem_local.z, blockSize, MPI_FLOAT, bodysystem_global.z, blockSize, MPI_FLOAT, MPI_COMM_WORLD);
        MPI_Allgather(bodysystem_local.vx, blockSize, MPI_FLOAT, bodysystem_global.vx, blockSize, MPI_FLOAT, MPI_COMM_WORLD);
        MPI_Allgather(bodysystem_local.vy, blockSize, MPI_FLOAT, bodysystem_global.vy, blockSize, MPI_FLOAT, MPI_COMM_WORLD);
        MPI_Allgather(bodysystem_local.vz, blockSize, MPI_FLOAT, bodysystem_global.vz, blockSize, MPI_FLOAT, MPI_COMM_WORLD);
        t1 = MPI_Wtime();
        total_net_time += t1 - t0;

        t0 = MPI_Wtime();
        bodyForce(bodysystem_global, dt, nBodies, bodysystem_local, blockSize);  // compute interbody forces
        t1 = MPI_Wtime();
        total_cpu_time += t1 - t0;


        // update positions
        int i;
        float* __restrict__ lx = bodysystem_local.x;
        float* __restrict__ ly = bodysystem_local.y;
        float* __restrict__ lz = bodysystem_local.z;
        const float* __restrict__ lvx = bodysystem_local.vx;
        const float* __restrict__ lvy = bodysystem_local.vy;
        const float* __restrict__ lvz = bodysystem_local.vz;

        t0 = MPI_Wtime();
#pragma omp parallel for schedule(static)
        for (i = 0; i < blockSize; i++) {
            lx[i] += lvx[i] * dt;
            ly[i] += lvy[i] * dt;
            lz[i] += lvz[i] * dt;
        }
        t1 = MPI_Wtime();
        total_cpu_time += t1 - t0;

    }  // end of iterations

    if (rank == MAIN_PROC) {
        double totalTime = total_net_time + total_cpu_time;

#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
        papi_helper_stop(papi_monitor);
        // papi_helper_print(papi_monitor);
        free(papi_monitor);
#endif


        printf("%d,%d,%.4f,%.4f,%.4f\n", size, nBodies, total_cpu_time, total_net_time, totalTime);
    }

    free(global_buffer);
    free(local_buffer);
    MPI_Finalize();
}

void randomizeBodies(float* data, int n) {
    int i;
    for (i = 0; i < n; i++) {
        data[i] = 2.0f * (rand() / (float)RAND_MAX) - 1.0f;
    }
}

void bodyForce(BodySystem p, float dt, int n, BodySystem localBuffer, int blocksize) {
    // Use __restrict__ instead of restrict
    const float* __restrict__ px = p.x;
    const float* __restrict__ py = p.y;
    const float* __restrict__ pz = p.z;
    const float* __restrict__ pm = p.m;

    float* __restrict__ lx = localBuffer.x;
    float* __restrict__ ly = localBuffer.y;
    float* __restrict__ lz = localBuffer.z;
    float* __restrict__ lvx = localBuffer.vx;
    float* __restrict__ lvy = localBuffer.vy;
    float* __restrict__ lvz = localBuffer.vz;

    int i;
#pragma omp parallel for schedule(static)
    for (i = 0; i < blocksize; i++) {
        float Fx = 0.0f, Fy = 0.0f, Fz = 0.0f;

        float lxi = lx[i], lyi = ly[i], lzi = lz[i];

        int j;
#pragma omp simd reduction(+ : Fx, Fy, Fz)
        for (j = 0; j < n; j++) {
            float dx = px[j] - lxi;
            float dy = py[j] - lyi;
            float dz = pz[j] - lzi;
            float distSqr = dx * dx + dy * dy + dz * dz + SOFTENING;
            float invDist = 1.0f / sqrtf(distSqr);
            float invDist3 = invDist * invDist * invDist;

            float massProduct = pm[j] * G;

            Fx += dx * invDist3 * massProduct;
            Fy += dy * invDist3 * massProduct;
            Fz += dz * invDist3 * massProduct;
        }

        lvx[i] += dt * Fx;
        lvy[i] += dt * Fy;
        lvz[i] += dt * Fz;
    }
}