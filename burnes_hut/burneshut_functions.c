//
// Created by Roberto on 28/09/2026.
//
//
// Created by Roberto on 26/09/2026.
//

#ifndef MINI_NBODY_MASTER_BURNESHUT_FUNCTIONS_C
#define MINI_NBODY_MASTER_BURNESHUT_FUNCTIONS_C
#endif  // MINI_NBODY_MASTER_BURNESHUT_FUNCTIONS_C
#include "burneshut_functions.h"

#include <math.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#include "data_structures.h"

#define OCTREE_CHILDREN_SIZE 8
// MAX_DEPTH determina la grandezza dello stack. 256 è più che sufficiente
// per la profondità massima di un octree nella quasi totalità dei casi.
#define MAX_DEPTH 256

OctreePool* pool;

/*
 * Init del pool
 *
 * ottenimento nuovo nodo
 * inserimento body nel nodo
 * divisione del nodo in octree
 *
 * verifica se un nodo è foglia
 * verifica se un body è al di fuori dei confini spaziali di un nodo
 *
 * Attraversamento dell'octree
 *
 * Calcolo dei centri di massa
 * Calcolo delle forze gravitazionali
 * Integrazione del moto
 *
 *
 */

// Inizializza il pool di nodi dell'Octree con 8N nodi
// dove N è il numero di corpi
void initOctreePool(int numBodies) {
    pool = (OctreePool*)malloc(sizeof(OctreePool));
    pool->nodes = malloc(OCTREE_CHILDREN_SIZE * numBodies * sizeof(OctreeNode));
    pool->max_nodes = 8 * numBodies;
    pool->next_free = 0;
}

/*
 * Resetto il pool per poter riutilizzare i nodi.
 * Utile per ricostruire l'albero perché evita di dover riallocare tutti i nodi da capo.
 * Quindi:
 * - pulisco i riferimenti ai nodi figli,
 * - pulisco il riferimento al body
 * - resetto l'id
 */
void resetOctreePool() { pool->next_free = 0; }

/*
 * Preleva un OctreeNode dal pool.
 * Effettua la realloc del pool se i nodi stanno esaurendosi
 *
 */
OctreeNode* newOctreeNode() {
    if (pool->next_free >= pool->max_nodes) {
        fprintf(stderr, "Errore: capacita massima OctreePool (%d nodi) raggiunta\n", pool->max_nodes);
        exit(EXIT_FAILURE);
    }
    OctreeNode* newNode = &pool->nodes[pool->next_free++];
    newNode->id = pool->next_free;
    newNode->mass = 0;
    newNode->cx = 0.0;
    newNode->cy = 0.0;
    newNode->cz = 0.0;
    newNode->body = NULL;
    int i;
    for (i = 0; i < OCTREE_CHILDREN_SIZE; i++) {
        newNode->children[i] = NULL;
    }

    return newNode;
}

/*
 * Libera la memoria occupata dal OctreePool
 */
void freeOctreePool() {
    if (pool != NULL) {
        free(pool->nodes);
        pool->nodes = NULL;
        pool->max_nodes = 0;
        pool->next_free = 0;
        free(pool);
        pool = NULL;
    }
}

/*
 * Verifica che un nodo sia una foglia.
 *
 */
int checkIfNodeIsLeaf(OctreeNode* node) {
    return node->children[0] == NULL;
}

/*
 * Inserisce un body all'interno dell'octree attraversando l'albero fino alle foglie.
 * Se necessario, scompone un nodo in 8 nodi figli ed effettua il reinserimento
 * dell'eventuale body che già era presente.
 *
 */
void insertBody(OctreeNode* root, Body* body) {
    if (root == NULL || body == NULL) {
        fprintf(stderr, "Riferimento NULLO per OctreeNode o body\n");
        return;
    }
    if (checkBodyOutsideOfOctreeNode(root, body)) {
        fprintf(stderr,
                "Il body si trova al di fuori dei confini spaziali del nodo:\n"
                "\t body: {%2.f,%.2f,%.2f,}\n}",
                body->x, body->y, body->z);
        return;
    }

    OctreeNode* curr = root;
    int depth = 0;

    while (depth < MAX_DEPTH) {
        if (curr->children[0] == NULL) {
            if (curr->body == NULL) {
                curr->body = body;
                return;
            }
            // è già presente un body, quindi scomponiamo il nodo
            divideNodeIntoOctree(curr);
        }

        float mid_x = 0.5f * (curr->bbox.min_x + curr->bbox.max_x);
        float mid_y = 0.5f * (curr->bbox.min_y + curr->bbox.max_y);
        float mid_z = 0.5f * (curr->bbox.min_z + curr->bbox.max_z);

        int octant = ((body->x > mid_x) ? 4 : 0) |
                     ((body->y > mid_y) ? 2 : 0) |
                     ((body->z > mid_z) ? 1 : 0);

        curr = curr->children[octant];
        depth++;
    }
}

/*
 * Scompone un nodo dividendolo in otto parti.
 * Effettua il reinserimento del body nei figli se necessario.
 *
 */
void divideNodeIntoOctree(OctreeNode* node) {
    if (node == NULL) {
        fprintf(stderr, "Riferimento NULLO per OctreeNode\n");
    }
    BoundingBox* box = &(node->bbox);

    // calcolo le dimensioni di ogni lato del cubo
    float xsize = (box->max_x - box->min_x) / 2;
    float ysize = (box->max_y - box->min_y) / 2;
    float zsize = (box->max_z - box->min_z) / 2;

    // Devo creare otto cubi da aggiungere ai figli di node
    int childrenCounter = 0;

#ifdef DEBUG
    printf("(%d) -> {\nmin_x=%.2f,\nmax_x=%.2f,\nmin_y=%.2f,\nmax_y=%.2f,\nmin_z=%.2f,\nmax_z=%.2f,\n]\n\n", node->id,
           node->bbox.min_x, node->bbox.max_x, node->bbox.min_y, node->bbox.max_y, node->bbox.min_z, node->bbox.max_z);
#endif

    // i è l'indice che gestisce lo spostamento sulle x
    int i;
    for (i = 1; i <= 2; i++) {
        // j è l'indice che gestisce lo spostamento sulle y
        int j;
        for (j = 1; j <= 2; j++) {
            // k è l'indice che gestisce lo spostamento sulle z
            int k;
            // generazione del cubo su ogni asse
            for (k = 1; k <= 2; k++) {
                OctreeNode* newNode = newOctreeNode();
                BoundingBox* newBbox = &(newNode->bbox);
                newBbox->min_x = box->min_x + (i - 1) * xsize;
                newBbox->min_y = box->min_y + (j - 1) * ysize;
                newBbox->min_z = box->min_z + (k - 1) * zsize;
                newBbox->max_x = box->min_x + i * xsize;
                newBbox->max_y = box->min_y + j * ysize;
                newBbox->max_z = box->min_z + k * zsize;
                node->children[childrenCounter++] = newNode;
#ifdef DEBUG
                printf(
                    "[(%d),%d,%d,%d] -> "
                    "{\nmin_x=%.2f,\nmax_x=%.2f,\nmin_y=%.2f,\nmax_y=%.2f,\nmin_z=%.2f,\nmax_z=%.2f,\n]\n\n",
                    node->id, i, j, k, newBbox->min_x, newBbox->max_x, newBbox->min_y, newBbox->max_y, newBbox->min_z,
                    newBbox->max_z);
                printf("children: %d\n", childrenCounter);
#endif
            }
        }
    }

    // reinserisco il body nel child corretto se serve
    if (node->body != NULL) {
        Body* body = node->body;
        node->body = NULL;
        insertBody(node, body);
    }
}

/*
 * Attraversa l'Octree mostrando info sulle foglie e sui bodies
 *
 */
void traverseOctree(OctreeNode* node, int depth) {
    if (node == NULL) {
        fprintf(stderr, "Riferimento NULLO per OctreeNode\n");
        return;
    }

    if (depth == 0)
        printf(
            "################################################\n"
            "\t\tPRINT\n"
            "################################################\n");

    printf("%d -> {\nmin_x=%.2f,\nmax_x=%.2f,\nmin_y=%.2f,\nmax_y=%.2f,\nmin_z=%.2f,\nmax_z=%.2f,\n]\n\n", node->id,
           node->bbox.min_x, node->bbox.max_x, node->bbox.min_y, node->bbox.max_y, node->bbox.min_z, node->bbox.max_z);

    if (checkIfNodeIsLeaf(node)) {
        printf("Leaf node #%d, depth %d\n", node->id, depth);
        if (node->body != NULL) {
            printf("\tFound body:[%d,%.2f,%.2f,%.2f]\n", node->id, node->body->x, node->body->y, node->body->z);
        }
        return;
    }

    int i;
    for (i = 0; i < OCTREE_CHILDREN_SIZE; i++) {
        traverseOctree(node->children[i], depth + 1);
    }
}

/*
 * Determina se un body ricade al di fuori dei confini di un Nodo
 *
 */
int checkBodyOutsideOfOctreeNode(OctreeNode* node, Body* body) {
    if (node == NULL) {
        fprintf(stderr, "Riferimento NULLO per OctreeNode\n");
        return 1;
    }
    if (body == NULL) {
        fprintf(stderr, "Riferimento NULLO per body\n");
        return 1;
    }

    BoundingBox bbox = node->bbox;
    if (body->x < bbox.min_x || body->x > bbox.max_x || body->y < bbox.min_y || body->y > bbox.max_y ||
        body->z < bbox.min_z || body->z > bbox.max_z) {
        return 1;
    }

    return 0;
}

OctreeNode* buildOctree(Body* bodies, int n) {
    resetOctreePool();
    OctreeNode* root = newOctreeNode();

    // 1. Trova le coordinate minime e massime assolute tra tutti i corpi
    float min_val = 1e9f;
    float max_val = -1e9f;

    int i;
    for (i = 0; i < n; i++) {
        if (bodies[i].x < min_val) min_val = bodies[i].x;
        if (bodies[i].y < min_val) min_val = bodies[i].y;
        if (bodies[i].z < min_val) min_val = bodies[i].z;

        if (bodies[i].x > max_val) max_val = bodies[i].x;
        if (bodies[i].y > max_val) max_val = bodies[i].y;
        if (bodies[i].z > max_val) max_val = bodies[i].z;
    }

    // 2. Determina la dimensione massima per assicurare che il Bounding Box sia un cubo perfetto
    // (necessario affinché il calcolo s/dist di Barnes-Hut sia spazialmente coerente)
    float max_abs = (max_val > -min_val) ? max_val : -min_val;
    float dimension = max_abs + 1.0f;  // + 1.0f di margine per sicurezza

    BoundingBox* bbox = &root->bbox;
    bbox->min_x = -dimension;
    bbox->min_y = -dimension;
    bbox->min_z = -dimension;
    bbox->max_x = dimension;
    bbox->max_y = dimension;
    bbox->max_z = dimension;

    // 3. Inserimento
    for (i = 0; i < n; i++) {
        insertBody(root, bodies + i);
    }

    return root;
}

void computeCentersOfMass() {
    // Scorriamo il memory pool al contrario: dai figli (ultimi creati) alla radice (primo creato)
    int i;
    for (i = pool->next_free - 1; i >= 0; i--) {
        OctreeNode* node = &pool->nodes[i];

        node->mass = 0.0f;
        node->cx = 0.0f;
        node->cy = 0.0f;
        node->cz = 0.0f;

        // Se è una foglia, prende i valori della particella (se presente)
        if (checkIfNodeIsLeaf(node)) {
            if (node->body != NULL) {
                node->mass = node->body->m;
                node->cx = node->body->x;
                node->cy = node->body->y;
                node->cz = node->body->z;
            }
        }
        // Se è un nodo interno, calcola il centro di massa basandosi sui figli
        else {
            int j;
            float total_mass = 0.0f;
            float weighted_x = 0.0f, weighted_y = 0.0f, weighted_z = 0.0f;

            for (j = 0; j < OCTREE_CHILDREN_SIZE; j++) {
                OctreeNode* child = node->children[j];
                if (child != NULL && child->mass > 0.0f) {
                    total_mass += child->mass;
                    weighted_x += child->cx * child->mass;
                    weighted_y += child->cy * child->mass;
                    weighted_z += child->cz * child->mass;
                }
            }

            node->mass = total_mass;

            // Evita la divisione per zero se il nodo non contiene particelle
            if (total_mass > 0.0f) {
                node->cx = weighted_x / total_mass;
                node->cy = weighted_y / total_mass;
                node->cz = weighted_z / total_mass;
            }
        }
    }
}

void calculateForce(OctreeNode* root, Body* target, float theta, float* fx, float* fy, float* fz) {
    // Stack array per evitare la ricorsione
    OctreeNode* stack[MAX_DEPTH];
    int stack_ptr = 0;

    // Inizializziamo l'accumulatore per l'accelerazione
    float acc_x = 0.0f;
    float acc_y = 0.0f;
    float acc_z = 0.0f;

    float tx = target->x;
    float ty = target->y;
    float tz = target->z;
    float theta_sq = theta * theta;

    // Push della radice
    stack[stack_ptr++] = root;

    while (stack_ptr > 0) {
        // Pop del nodo
        OctreeNode* node = stack[--stack_ptr];

        // Se il nodo è vuoto o non ha massa, lo ignoriamo
        if (node == NULL || node->mass == 0.0f) continue;

        int is_leaf = (node->children[0] == NULL);

        // Se il nodo è una foglia e contiene la particella stessa, saltiamo
        // (una particella non esercita gravità su se stessa)
        if (is_leaf && node->body == target) continue;

        // Calcolo delle distanze sui tre assi
        float dx = node->cx - tx;
        float dy = node->cy - ty;
        float dz = node->cz - tz;

        // Distanza al quadrato con parametro SOFTENING per evitare div/0 o forze infinite
        float dist_sq = dx * dx + dy * dy + dz * dz + SOFTENING * SOFTENING;

        // s: dimensione del nodo (supponendo nodi cubici, basta un solo asse)
        float s = node->bbox.max_x - node->bbox.min_x;

        // Condizione Barnes-Hut: (s / dist) < theta <==> (s * s) < (theta * theta * dist_sq)
        if (is_leaf || (s * s) < (theta_sq * dist_sq)) {
            // Il nodo è abbastanza lontano (o è un corpo singolo).
            // Calcoliamo l'interazione con il suo centro di massa complessivo.

            // Accelerazione = G * M / d^2.
            // Per trovare i vettori, moltiplichiamo per inv_dist^3 ottenendo (G * M / d^3)
            // e moltiplichiamo poi per dx, dy, dz.
            float inv_dist = 1.0f / sqrtf(dist_sq);
            float inv_dist3 = inv_dist * inv_dist * inv_dist;
            float a = (G * node->mass) * inv_dist3;

            acc_x += a * dx;
            acc_y += a * dy;
            acc_z += a * dz;
        } else {
            // Il nodo è troppo vicino e non è una foglia: dobbiamo esplorare i suoi figli.
            int i;
            for (i = 0; i < OCTREE_CHILDREN_SIZE; i++) {
                if (node->children[i] != NULL) {
                    if (stack_ptr < MAX_DEPTH) {
                        stack[stack_ptr++] = node->children[i];  // Push del figlio
                    } else {
                        fprintf(stderr, "Stack overflow durante calculateForce!\n");
                        return;
                    }
                }
            }
        }
    }

    // Restituiamo direttamente l'accelerazione (evitando * m e / m ridondanti)
    *fx = acc_x;
    *fy = acc_y;
    *fz = acc_z;
}

/*
 * Aggiorna la cinematica di tutti i corpi nella simulazione.
 * dt: delta time (intervallo di tempo per ogni step, es. 0.01)
 * theta: parametro di accuratezza del Barnes-Hut (es. 0.5)
 */
void updatePhysicsWithIndex(Body* bodies, int start_idx, int end_idx, OctreeNode* root, float theta, float dt) {
    if (root == NULL || bodies == NULL) return;

    int i;
#pragma omp parallel for private(i) schedule(guided)
    for (i = start_idx; i < end_idx; i++) {
        float ax = 0.0f, ay = 0.0f, az = 0.0f;

        // 1. Calcola l'accelerazione netta agente sulla particella i-esima
        calculateForce(root, &bodies[i], theta, &ax, &ay, &az);

        // 2. Aggiorna la velocità usando l'accelerazione corrente
        bodies[i].vx += ax * dt;
        bodies[i].vy += ay * dt;
        bodies[i].vz += az * dt;

        // 3. Aggiorna la posizione usando la NUOVA velocità (Eulero Simplettico)
        bodies[i].x += bodies[i].vx * dt;
        bodies[i].y += bodies[i].vy * dt;
        bodies[i].z += bodies[i].vz * dt;
    }
}

void updatePhysics(Body* bodies, int numBodies, OctreeNode* root, float theta, float dt) {
    updatePhysicsWithIndex(bodies, 0, numBodies, root, theta, dt);
}

// SOA

BodiesSOA createBodiesSOA(int n) {
    BodiesSOA b;
    b.numBodies = n;

    // Allineamento a 32 byte per AVX
    size_t align = 32;
    size_t size = n * sizeof(float);

    // posix_memalign restituisce 0 in caso di successo e popola il puntatore
    posix_memalign((void**)&b.x, align, size);
    posix_memalign((void**)&b.y, align, size);
    posix_memalign((void**)&b.z, align, size);
    posix_memalign((void**)&b.vx, align, size);
    posix_memalign((void**)&b.vy, align, size);
    posix_memalign((void**)&b.vz, align, size);
    posix_memalign((void**)&b.m, align, size);

    return b;
}

void calculateForceSOA(OctreeNodeSOA* root, BodiesSOA* bodies, int target_idx, float theta, float* fx, float* fy,
                       float* fz) {
    // Stack array per evitare la ricorsione
    OctreeNodeSOA* stack[MAX_DEPTH];
    int stack_ptr = 0;

    // Inizializziamo l'accumulatore per l'accelerazione
    float acc_x = 0.0f;
    float acc_y = 0.0f;
    float acc_z = 0.0f;

    // Estraiamo localmente i dati della particella bersaglio dalla struttura SoA
    // Questo evita dereferenziazioni ripetute durante l'attraversamento dell'albero
    float tx = bodies->x[target_idx];
    float ty = bodies->y[target_idx];
    float tz = bodies->z[target_idx];
    float theta_sq = theta * theta;

    // Push della radice
    stack[stack_ptr++] = root;

    while (stack_ptr > 0) {
        // Pop del nodo
        OctreeNodeSOA* node = stack[--stack_ptr];

        // Se il nodo è vuoto o non ha massa, lo ignoriamo
        if (node == NULL || node->mass == 0.0f) continue;

        int is_leaf = (node->children[0] == NULL);

        // Se il nodo è una foglia e contiene la particella stessa, saltiamo
        // (una particella non esercita gravità su se stessa)
        // NOTA: Qui usiamo 'body_id' al posto del vecchio puntatore 'body'
        if (is_leaf && node->body_id == target_idx) continue;

        // Calcolo delle distanze sui tre assi
        float dx = node->cx - tx;
        float dy = node->cy - ty;
        float dz = node->cz - tz;

        // Distanza al quadrato con parametro SOFTENING per evitare div/0 o forze infinite
        float dist_sq = dx * dx + dy * dy + dz * dz + SOFTENING * SOFTENING;

        // s: dimensione del nodo (supponendo nodi cubici, basta un solo asse)
        float s = node->bbox.max_x - node->bbox.min_x;

        // Condizione Barnes-Hut: (s / dist) < theta <==> (s * s) < (theta * theta * dist_sq)
        if (is_leaf || (s * s) < (theta_sq * dist_sq)) {
            // Il nodo è abbastanza lontano (o è un corpo singolo).
            // Calcoliamo l'interazione con il suo centro di massa complessivo.

            // Accelerazione = G * M / d^2.
            // Per trovare i vettori, moltiplichiamo per inv_dist^3 ottenendo (G * M / d^3)
            // e moltiplichiamo poi per dx, dy, dz.
            float inv_dist = 1.0f / sqrtf(dist_sq);
            float inv_dist3 = inv_dist * inv_dist * inv_dist;
            float a = (G * node->mass) * inv_dist3;

            acc_x += a * dx;
            acc_y += a * dy;
            acc_z += a * dz;
        } else {
            // Il nodo è troppo vicino e non è una foglia: dobbiamo esplorare i suoi figli.
            int i;
            for (i = 0; i < OCTREE_CHILDREN_SIZE; i++) {
                if (node->children[i] != NULL) {
                    if (stack_ptr < MAX_DEPTH) {
                        stack[stack_ptr++] = node->children[i];  // Push del figlio
                    } else {
                        fprintf(stderr, "Stack overflow durante calculateForceSOA!\n");
                        return;
                    }
                }
            }
        }
    }

    // Restituiamo direttamente l'accelerazione (evitando * m e / m ridondanti)
    *fx = acc_x;
    *fy = acc_y;
    *fz = acc_z;
}

void updatePhysicsWithIndexVectorized(BodiesSOA* bodies, int start_idx, int end_idx, OctreeNodeSOA* root, float theta,
                                      float dt, float* force_x, float* force_y, float* force_z) {
    if (root == NULL || bodies == NULL) return;

    int i;

    // 1. FASE DI CALCOLO FORZE (Multithreading sui rami dell'Octree)
#pragma omp parallel for private(i) schedule(guided)
    for (i = start_idx; i < end_idx; i++) {
        float ax = 0.0f, ay = 0.0f, az = 0.0f;
        calculateForceSOA(root, bodies, i, theta, &ax, &ay, &az);

        int idx = i - start_idx;
        force_x[idx] = ax;
        force_y[idx] = ay;
        force_z[idx] = az;
    }

    // 2. FASE DI INTEGRAZIONE (Multithreading + SIMD Vettorializzato)
#pragma omp parallel for schedule(static)
    for (i = start_idx; i < end_idx; i++) {
        int idx = i - start_idx;

        float ax = force_x[idx];
        float ay = force_y[idx];
        float az = force_z[idx];

        bodies->vx[i] += ax * dt;
        bodies->vy[i] += ay * dt;
        bodies->vz[i] += az * dt;

        bodies->x[i] += bodies->vx[i] * dt;
        bodies->y[i] += bodies->vy[i] * dt;
        bodies->z[i] += bodies->vz[i] * dt;
    }
}

// Variabili globali per il pool SoA
OctreeNodeSOA* pool_soa_nodes = NULL;
int pool_soa_max_nodes = 0;
int pool_soa_next_free = 0;

void initOctreePoolSOA(int numBodies) {
    pool_soa_nodes = (OctreeNodeSOA*)malloc(OCTREE_CHILDREN_SIZE * numBodies * sizeof(OctreeNodeSOA));
    pool_soa_max_nodes = OCTREE_CHILDREN_SIZE * numBodies;
    pool_soa_next_free = 0;
}

void resetOctreePoolSOA() {
    pool_soa_next_free = 0;
}

OctreeNodeSOA* newOctreeNodeSOA() {
    if (pool_soa_next_free >= pool_soa_max_nodes) {
        fprintf(stderr, "Errore: capacita massima OctreePoolSOA (%d nodi) raggiunta\n", pool_soa_max_nodes);
        exit(EXIT_FAILURE);
    }
    OctreeNodeSOA* newNode = &pool_soa_nodes[pool_soa_next_free++];
    newNode->id = pool_soa_next_free;
    newNode->body_id = -1;
    newNode->mass = 0.0f;
    newNode->cx = 0.0f;
    newNode->cy = 0.0f;
    newNode->cz = 0.0f;
    int i;
    for (i = 0; i < OCTREE_CHILDREN_SIZE; i++) {
        newNode->children[i] = NULL;
    }
    return newNode;
}

int checkIfNodeIsLeafSOA(OctreeNodeSOA* node) {
    return node->children[0] == NULL;
}

int checkBodyOutsideOfOctreeNodeSOA(OctreeNodeSOA* node, BodiesSOA* bodies, int body_idx) {
    BoundingBox bbox = node->bbox;
    float bx = bodies->x[body_idx];
    float by = bodies->y[body_idx];
    float bz = bodies->z[body_idx];

    if (bx < bbox.min_x || bx > bbox.max_x || by < bbox.min_y || by > bbox.max_y || bz < bbox.min_z ||
        bz > bbox.max_z) {
        return 1;
    }
    return 0;
}

// Prototipo necessario per la ricorsione incrociata
void insertBodySOA(OctreeNodeSOA* root, BodiesSOA* bodies, int body_idx);

void divideNodeIntoOctreeSOA(OctreeNodeSOA* node, BodiesSOA* bodies) {
    BoundingBox* box = &(node->bbox);
    float xsize = (box->max_x - box->min_x) / 2.0f;
    float ysize = (box->max_y - box->min_y) / 2.0f;
    float zsize = (box->max_z - box->min_z) / 2.0f;

    int childrenCounter = 0;
    int i, j, k;
    for (i = 1; i <= 2; i++) {
        for (j = 1; j <= 2; j++) {
            for (k = 1; k <= 2; k++) {
                OctreeNodeSOA* newNode = newOctreeNodeSOA();
                BoundingBox* newBbox = &(newNode->bbox);
                newBbox->min_x = box->min_x + (i - 1) * xsize;
                newBbox->min_y = box->min_y + (j - 1) * ysize;
                newBbox->min_z = box->min_z + (k - 1) * zsize;
                newBbox->max_x = box->min_x + i * xsize;
                newBbox->max_y = box->min_y + j * ysize;
                newBbox->max_z = box->min_z + k * zsize;
                node->children[childrenCounter++] = newNode;
            }
        }
    }

    // Se il nodo aveva un body assegnato, lo re-inseriamo nei figli
    if (node->body_id != -1) {
        int old_body_id = node->body_id;
        node->body_id = -1;  // Il nodo genitore non contiene più direttamente la particella
        insertBodySOA(node, bodies, old_body_id);
    }
}

void insertBodySOA(OctreeNodeSOA* root, BodiesSOA* bodies, int body_idx) {
    if (checkBodyOutsideOfOctreeNodeSOA(root, bodies, body_idx)) return;

    float bx = bodies->x[body_idx];
    float by = bodies->y[body_idx];
    float bz = bodies->z[body_idx];

    OctreeNodeSOA* curr = root;
    int depth = 0;

    while (depth < MAX_DEPTH) {
        if (curr->children[0] == NULL) {
            if (curr->body_id == -1) {
                curr->body_id = body_idx;
                return;
            }
            divideNodeIntoOctreeSOA(curr, bodies);
        }

        float mid_x = 0.5f * (curr->bbox.min_x + curr->bbox.max_x);
        float mid_y = 0.5f * (curr->bbox.min_y + curr->bbox.max_y);
        float mid_z = 0.5f * (curr->bbox.min_z + curr->bbox.max_z);

        int octant = ((bx > mid_x) ? 4 : 0) |
                     ((by > mid_y) ? 2 : 0) |
                     ((bz > mid_z) ? 1 : 0);

        curr = curr->children[octant];
        depth++;
    }
}

OctreeNodeSOA* buildOctreeSOA(BodiesSOA* bodies, int n) {
    resetOctreePoolSOA();
    OctreeNodeSOA* root = newOctreeNodeSOA();

    float min_val = 1e9f;
    float max_val = -1e9f;

    int i;
    for (i = 0; i < n; i++) {
        if (bodies->x[i] < min_val) min_val = bodies->x[i];
        if (bodies->y[i] < min_val) min_val = bodies->y[i];
        if (bodies->z[i] < min_val) min_val = bodies->z[i];

        if (bodies->x[i] > max_val) max_val = bodies->x[i];
        if (bodies->y[i] > max_val) max_val = bodies->y[i];
        if (bodies->z[i] > max_val) max_val = bodies->z[i];
    }

    float max_abs = (max_val > -min_val) ? max_val : -min_val;
    float dimension = max_abs + 1.0f;

    root->bbox.min_x = -dimension;
    root->bbox.min_y = -dimension;
    root->bbox.min_z = -dimension;
    root->bbox.max_x = dimension;
    root->bbox.max_y = dimension;
    root->bbox.max_z = dimension;

    for (i = 0; i < n; i++) {
        insertBodySOA(root, bodies, i);
    }

    return root;
}

void computeCentersOfMassSOA(BodiesSOA* bodies) {
    int i;
    for (i = pool_soa_next_free - 1; i >= 0; i--) {
        OctreeNodeSOA* node = &pool_soa_nodes[i];

        node->mass = 0.0f;
        node->cx = 0.0f;
        node->cy = 0.0f;
        node->cz = 0.0f;

        if (checkIfNodeIsLeafSOA(node)) {
            if (node->body_id != -1) {
                int b_id = node->body_id;
                node->mass = bodies->m[b_id];
                node->cx = bodies->x[b_id];
                node->cy = bodies->y[b_id];
                node->cz = bodies->z[b_id];
            }
        } else {
            int j;
            float total_mass = 0.0f;
            float weighted_x = 0.0f, weighted_y = 0.0f, weighted_z = 0.0f;

            for (j = 0; j < OCTREE_CHILDREN_SIZE; j++) {
                OctreeNodeSOA* child = node->children[j];
                if (child != NULL && child->mass > 0.0f) {
                    total_mass += child->mass;
                    weighted_x += child->cx * child->mass;
                    weighted_y += child->cy * child->mass;
                    weighted_z += child->cz * child->mass;
                }
            }

            node->mass = total_mass;
            if (total_mass > 0.0f) {
                node->cx = weighted_x / total_mass;
                node->cy = weighted_y / total_mass;
                node->cz = weighted_z / total_mass;
            }
        }
    }
}

void randomizeBodiesSOA(BodiesSOA* bodies, int numBodies) {
    if (bodies == NULL) return;

    int i;
    for (i = 0; i < numBodies; i++) {
        // Genera posizioni, velocità e masse con la stessa distribuzione di randomizeBodies (AoS)
        bodies->x[i] = (2.0f * (rand() / (float)RAND_MAX) - 1.0f) * 1000.0f;
        bodies->y[i] = (2.0f * (rand() / (float)RAND_MAX) - 1.0f) * 1000.0f;
        bodies->z[i] = (2.0f * (rand() / (float)RAND_MAX) - 1.0f) * 1000.0f;
        bodies->vx[i] = (2.0f * (rand() / (float)RAND_MAX) - 1.0f) * 1000.0f;
        bodies->vy[i] = (2.0f * (rand() / (float)RAND_MAX) - 1.0f) * 1000.0f;
        bodies->vz[i] = (2.0f * (rand() / (float)RAND_MAX) - 1.0f) * 1000.0f;
        bodies->m[i] = (rand() / (float)RAND_MAX) * 99.0f + 1.0f;
    }
}

/*
 * Libera la memoria occupata dal pool dei nodi SoA
 */
void freeOctreePoolSOA() {
    if (pool_soa_nodes != NULL) {
        free(pool_soa_nodes);
        pool_soa_nodes = NULL;
    }
    pool_soa_max_nodes = 0;
    pool_soa_next_free = 0;
}

/*
 * Libera la memoria allineata allocata per i corpi
 */
void freeBodiesSOA(BodiesSOA* bodies) {
    if (bodies == NULL) return;

    free(bodies->x);
    free(bodies->y);
    free(bodies->z);
    free(bodies->vx);
    free(bodies->vy);
    free(bodies->vz);
    free(bodies->m);
}