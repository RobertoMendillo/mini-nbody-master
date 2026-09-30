#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "data_structures.h"
#include "burneshut_functions.h"
#include "support_functions.h"

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

    float theta = 0.05f;

    Body* bodies = malloc(numBodies * sizeof(Body));
    randomizeBodies(bodies, numBodies);

    initOctreePool(numBodies);

    // Ciclo di simulazione (es. 1000 step)
    for (int step = 0; step < nIters; step++) {
        printf("step %i\n", step);

        // 1. Costruisce l'albero spaziale (resettando automaticamente il pool)
        OctreeNode* root = buildOctree(bodies, numBodies);

        printf("buit octree\n");

        // 2. Calcola i centri di massa dal basso verso l'alto
        computeCentersOfMass();

        printf("computed centers of mass\n");

        // 3. Calcola le forze e aggiorna posizioni e velocità
        updatePhysics(bodies, numBodies, root, theta, dt);

        // (Opzionale) Salva le posizioni su file o renderizza a schermo
        // printBodies(bodies, numBodies);
    }

    freeOctreePool();
    free(bodies);
}
