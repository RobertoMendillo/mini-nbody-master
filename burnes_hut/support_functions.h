//
// Created by Roberto on 24/09/2026.
//

#ifndef MINI_NBODY_MASTER_SUPPORT_FUNCTIONS_H
#define MINI_NBODY_MASTER_SUPPORT_FUNCTIONS_H

#endif //MINI_NBODY_MASTER_SUPPORT_FUNCTIONS_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>


static void randomizeBodies(Body* data, int n);
static void bodyForce(Body* p, float dt, int n, Body* localBuffer, int blocksize);

// sets up the bodies with random
// position, velocity and mass
void randomizeBodies(Body* bodies, int n) {
    int i;
    for (i = 0; i < n; i++) {
        bodies[i].x = (2.0f * (rand() / (float)RAND_MAX) - 1.0f) * 1000;
        bodies[i].y = (2.0f * (rand() / (float)RAND_MAX) - 1.0f) * 1000;
        bodies[i].z = (2.0f * (rand() / (float)RAND_MAX) - 1.0f) * 1000;
        bodies[i].vx = (2.0f * (rand() / (float)RAND_MAX) - 1.0f) * 1000;
        bodies[i].vy = (2.0f * (rand() / (float)RAND_MAX) - 1.0f) * 1000;
        bodies[i].vz = (2.0f * (rand() / (float)RAND_MAX) - 1.0f) * 1000;
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
            float invDist = 1.0f / sqrtf(distSqr);  // -->
            // 1/r
            float invDist3 = invDist * invDist * invDist;  // --> 1/r^3

            float massProduct = p[j].m * G;

            // to optimize calculations,
            // this is actually an
            // acceleration because it
            // is already divided by
            // mass (avoid division by
            // mass later)
            Fx += dx * invDist3 * massProduct;  // component of force with respect to x (dx / r^3)
            Fy += dy * invDist3 * massProduct;  // component of force with respect to y (dy /  r^3)
            Fz += dz * invDist3 * massProduct;  // component of force with respect to z (dz / r^3)
        }

        // compute velocity on every
        // direction
        localBuffer[i].vx += dt * Fx;  // velocity on x axis
        localBuffer[i].vy += dt * Fy;  // velocity on y axis
        localBuffer[i].vz += dt * Fz;  // velocity on z axis
    }
}



