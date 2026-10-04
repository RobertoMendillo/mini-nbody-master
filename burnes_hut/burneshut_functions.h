//
// Created by Roberto on 26/09/2026.
//

#ifndef MINI_NBODY_MASTER_BURNESHUT_FUNCTIONS_H
#define MINI_NBODY_MASTER_BURNESHUT_FUNCTIONS_H
#endif //MINI_NBODY_MASTER_BURNESHUT_FUNCTIONS_H


#include "data_structures.h"

#define OCTREE_CHILDREN_SIZE 8

// funzioni di gestione del pool
void initOctreePool(int numBodies);
void resetOctreePool();
void freeOctreePool();

// funzioni di gestione dell'Octree
void traverseOctree(OctreeNode* node, int depth);
OctreeNode* buildOctree(Body* bodies, int n);

// funzioni di gestione dei nodi
OctreeNode* newOctreeNode();
void insertBody(OctreeNode* root, Body* body);
int checkIfNodeIsLeaf(OctreeNode* node);
void divideNodeIntoOctree(OctreeNode* node);
int checkBodyOutsideOfOctreeNode(OctreeNode* node, Body* body);

// funzioni di calcolo
void computeMassDistribution(OctreeNode* node);
void computeCentersOfMass();
void updatePhysics(Body* bodies, int numBodies, OctreeNode* root, float theta, float dt);
