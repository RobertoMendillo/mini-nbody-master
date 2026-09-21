#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "timer.h"

#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
#include "papi_helper.h"
#endif

#define SOFTENING 1e-9f  // needed to avoid distance equal to zero
#define G 6.67e-11f      // universal gravitational constant

typedef struct {
    float x, y, z, vx, vy, vz, m;
} Body;

static void randomizeBodies(float* data, int n);
static void bodyForce(Body* p, float dt, int n);

/*
  Command line arguments:
    [1] --> number of bodies: default 30.000
    [2] --> simulation iterations: default 10
    [3] --> time step: default 0.01
*/
int main(int argc, char** argv) {
    // number of bodies in the simulation
    int nBodies = 30000;
    // reading number of bodies as command line argument
    if (argc > 1) nBodies = atoi(argv[1]);

    int nIters = 10;  // simulation iterations
    if (argc > 2) nIters = atoi(argv[2]);
    float dt = 0.01f;  // time step
    if (argc > 3) dt = atof(argv[3]);

    MPI_Init(&argc, &argv);

    const unsigned long long bytes = nBodies * sizeof(Body);
    float* buf = malloc(bytes);
    Body* p = (Body*)buf;

    double t0, t1, total_time = 0.0;

// starts papi monitor for compatible architectures
#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
    Papi_Monitor* papi_monitor = malloc(sizeof(Papi_Monitor));
    papi_helper_init(papi_monitor);
#endif

    t0 = MPI_Wtime();
    randomizeBodies(buf, 7 * nBodies);  // Init pos / vel data
    t1 = MPI_Wtime();
    total_time += t1 - t0;

#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
    papi_helper_start(papi_monitor);
#endif
    int iter;
    t0 = MPI_Wtime();
    for (iter = 1; iter <= nIters; iter++) {
        bodyForce(p, dt, nBodies);  // compute interbody forces

        int i;
        for (i = 0; i < nBodies; i++) {  // integrate position
            p[i].x += p[i].vx * dt;
            p[i].y += p[i].vy * dt;
            p[i].z += p[i].vz * dt;
        }
    }
    t1 = MPI_Wtime();
    total_time += t1 - t0;

#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
    papi_helper_stop(papi_monitor);
#endif

    printf("%d,%.4f\n", nBodies, total_time);

    free(buf);
    MPI_Finalize();
}

// sets up the bodies with random position, velocity and mass
void randomizeBodies(float* data, int n) {
    int i;
    for (i = 0; i < n; i++) {
        data[i] = 2.0f * (rand() / (float)RAND_MAX) - 1.0f;
        if (i % 7 == 6) data[i] = (rand() / (float)RAND_MAX) * 100;  // set mass to a positive number
    }
}

// computes interbody forces
void bodyForce(Body* p, float dt, int n) {
    int i, j;
    for (i = 0; i < n; i++) {
        // total force on every axis applied by every other body
        float Fx = 0.0f;
        float Fy = 0.0f;
        float Fz = 0.0f;

        for (j = 0; j < n; j++) {
            float dx = p[j].x - p[i].x;  // distance on x axis
            float dy = p[j].y - p[i].y;  // distance on y axis
            float dz = p[j].z - p[i].z;  // distance on z axis

            // compute force on every direction
            // F = 1/r^2 * D/r = D/r^3
            // D = (dx/r, dy/r, dz/r)
            float distSqr = dx * dx + dy * dy + dz * dz + SOFTENING;  // total distance between the two bodies
            float invDist = 1.0f / sqrtf(distSqr);                    // --> 1/r
            float invDist3 = invDist * invDist * invDist;             // --> 1/r^3

            float massProduct = p[j].m * G;

            // to optimize calculations, this is actually an acceleration because it is already
            // divided by mass (avoid division by mass later)
            Fx += dx * invDist3 * massProduct;  // component of force with respect to x (dx / r^3)
            Fy += dy * invDist3 * massProduct;  // component of force with respect to y (dy / r^3)
            Fz += dz * invDist3 * massProduct;  // component of force with respect to z (dz / r^3)
        }

        // compute velocity on every direction
        p[i].vx += dt * Fx;  // velocity on x axis
        p[i].vy += dt * Fy;  // velocity on y axis
        p[i].vz += dt * Fz;  // velocity on z axis
    }
}