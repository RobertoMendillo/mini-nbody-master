#pragma once
#define SOFTENING 1e-9f  // needed to avoid distance equal to zero
#define G 6.67e-11f      // universal gravitational constant
#define BODY_SIZE sizeof(Body)

typedef struct {
    float x, y, z, vx, vy, vz, m;
} Body;

// Struttura per definire i confini spaziali di un nodo
typedef struct {
    float min_x, min_y, min_z;
    float max_x, max_y, max_z;
} BoundingBox;

// Struttura del nodo dell'Octree
typedef struct OctreeNode {
    float mass;          // Massa totale contenuta nel nodo
    float cx, cy, cz;    // Coordinate del centro di massa
    int id;

    BoundingBox bbox;    // Confini del cubo che il nodo rappresenta

    // Puntatori agli 8 sottomultipli (ottanti).
    // Se tutti sono NULL, il nodo è una foglia.
    struct OctreeNode* children[8];

    // Se il nodo è una foglia e contiene una particella, punta al corpo.
    // Altrimenti è NULL.
    Body* body;
} OctreeNode;

// Struttura per il Memory Pool (per evitare malloc/free ad ogni iterazione)
typedef struct {
    OctreeNode* nodes;   // Array contiguo pre-allocato di nodi
    int max_nodes;       // Capacità massima dell'array
    int next_free;       // Indice del prossimo nodo disponibile da usare
} OctreePool;
