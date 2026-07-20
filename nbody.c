#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "timer.h"

#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
#include "papi_helper.h"
#endif

#define SOFTENING 1e-9f  // needed to avoid distance equal to zero
#define G 6.67e-11f      // universal gravitational constant

typedef struct {
    float x, y, z, vx, vy, vz, m;
} Body;

// sets up the bodies with random position, velocity and mass
void randomizeBodies(float* data, int n) {
    int i;
    for (i = 0; i < n; i++) {
        data[i] = 2.0f * (rand() / (float)RAND_MAX) - 1.0f;
        if (i % 7 == 6)
            data[i] = (rand() / (float)RAND_MAX) * 100;  // set mass to a positive number
    }
}

// computes interbody forces assuming mass of bodies equal to 1
void bodyForce(Body* p, float dt, int n) {
#pragma omp parallel for schedule(dynamic)
    int i;
    int j;
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
            float distSqr =
                dx * dx + dy * dy + dz * dz + SOFTENING;   // total distance between the two bodies
            float invDist = 1.0f / sqrtf(distSqr);         // --> 1/r
            float invDist3 = invDist * invDist * invDist;  // --> 1/r^3

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

void exportBodies(Body* p, int n, int iter) {
    // Apre il file in modalità "append" (scrive in coda al file esistente)
    FILE* f = fopen("simulation_data.csv", "a");
    if (f == NULL) {
        printf("Errore nell'apertura del file!\n");
        return;
    }

    // Se è la primissima iterazione, scrive l'intestazione delle colonne (header)
    if (iter == 1) {
        fprintf(f, "iteration,body_id,x,y,z\n");
    }

    // Scrive i dati di ogni corpo
    int i;
    for (i = 0; i < n; i++) {
        fprintf(f, "%d,%d,%.5f,%.5f,%.5f\n", iter, i, p[i].x, p[i].y, p[i].z);
    }

    fclose(f);  // Chiude il file per salvare i dati sul disco
}

/*
  Command line arguments:
    [1] --> number of bodies: default 30.000
    [2] --> simulation iterations: default 10
    [3] --> time step: default 0.01
*/
int main(const int argc, const char** argv) {
    // number of bodies in the simulation
    int nBodies = 30000;
    // reading number of bodies as command line argument
    if (argc > 1) nBodies = atoi(argv[1]);

    int nIters = 10;  // simulation iterations
    if (argc > 2) nIters = atoi(argv[2]);
    float dt = 0.01f;  // time step
    if (argc > 3) dt = atof(argv[3]);

    int bytes = nBodies * sizeof(Body);
    float* buf = (float*)malloc(bytes);
    Body* p = (Body*)buf;

#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
    Papi_Monitor* papi_monitor = malloc(sizeof(Papi_Monitor));
    printf("Init papi monitors ...\n");
    papi_helper_init(papi_monitor);
    printf("... completed\n");
#endif

    printf("Running simulation of %d bodies on %d iterations with time step of %.2f\n", nBodies,
           nIters, dt);

    randomizeBodies(buf, 7 * nBodies);  // Init pos / vel data

    double totalTime = 0.0;  // simulation total execution time

#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
    printf("Starting papi monitors ...\n");
    papi_helper_start(papi_monitor);
    printf("... started\n");
#endif
    int iter;
    for (iter = 1; iter <= nIters; iter++) {
        StartTimer();

        bodyForce(p, dt, nBodies);  // compute interbody forces

#pragma omp parallel for schedule(static)
        int i;
        for (i = 0; i < nBodies; i++) {  // integrate position
            p[i].x += p[i].vx * dt;
            p[i].y += p[i].vy * dt;
            p[i].z += p[i].vz * dt;
        }

#ifdef EXPORT
        exportBodies(p, nBodies, iter);
#endif

        const double tElapsed = GetTimer() / 1000.0;
        if (iter > 1) {  // First iter is warm up
            totalTime += tElapsed;
        }
#ifndef SHMOO
        printf("Iteration %d: %.3f seconds\n", iter, tElapsed);
#endif
    }
    double avgTime = totalTime / (double)(nIters - 1);

#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
    printf("Stopping papi monitors ...\n");
    papi_helper_stop(papi_monitor);
    printf("... stopped\n");
    papi_helper_print(papi_monitor);
#endif

#ifdef SHMOO
    printf("%d, %0.3f\n", nBodies, 1e-9 * nBodies * nBodies / avgTime);
#else
    printf(
        "Average rate for iterations 2 through %d: %.3f steps per second, %.3f average per "
        "iteration.\n",
        nIters, (float)nIters / totalTime, avgTime);
    printf("%d Bodies: average %0.3f Billion Interactions / second\n", nBodies,
           1e-9 * nBodies * nBodies / avgTime);
#endif
    free(buf);
}
