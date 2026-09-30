//
// Created by Roberto on 28/09/2026.
//
//
// Created by Roberto on 26/09/2026.
//

#ifndef MINI_NBODY_MASTER_BURNESHUT_FUNCTIONS_C
#define MINI_NBODY_MASTER_BURNESHUT_FUNCTIONS_C
#endif //MINI_NBODY_MASTER_BURNESHUT_FUNCTIONS_C
#include "burneshut_functions.h"

#include <math.h>
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
    pool->next_free=0;
}

/*
 * Resetto il pool per poter riutilizzare i nodi.
 * Utile per ricostruire l'albero perché evita di dover riallocare tutti i nodi da capo.
 * Quindi:
 * - pulisco i riferimenti ai nodi figli,
 * - pulisco il riferimento al body
 * - resetto l'id
 */
void resetOctreePool() {
    int i;
    for (i=0; i<pool->max_nodes; i++) {
        OctreeNode* node = pool->nodes+i;
        node->id = 0;
        node->body = NULL;
        int j;
        for (j=0; j<OCTREE_CHILDREN_SIZE; j++) {
            node->children[j] = NULL;
        }
    }
}

/*
 * Preleva un OctreeNode dal pool.
 * Effettua la realloc del pool se i nodi stanno esaurendosi
 *
 */
OctreeNode* newOctreeNode() {
    if (pool->next_free == pool->max_nodes-1) {
        pool->max_nodes *= 2;
        pool->nodes = realloc(pool->nodes, pool->max_nodes * sizeof(OctreeNode));
        if (pool->nodes == NULL) {
            fprintf(stderr, "Errore di reallocazione per OctreePool\n");
            exit(EXIT_FAILURE);
        }
    }
    OctreeNode* newNode = &pool->nodes[pool->next_free++];
    newNode->id = pool->next_free;
    newNode->mass = 0;
    newNode->cx = 0.0;
    newNode->cy = 0.0;
    newNode->cz = 0.0;
    newNode->body = NULL;
    int i;
    for (i=0; i<OCTREE_CHILDREN_SIZE; i++) {
        newNode->children[i] = NULL;
    }

    return newNode;
}


/*
 * Libera la memoria occupata dal OctreePool
 */
void freeOctreePool() {
    free(pool->nodes);
    pool->nodes = NULL;
    pool->max_nodes = 0;
    pool->next_free = 0;
}

/*
 * Verifica che un nodo sia una foglia.
 *
 */
int checkIfNodeIsLeaf(OctreeNode* node) {
    int children = 0;
    int i;
    for (i=0; i<OCTREE_CHILDREN_SIZE; i++) {
        if (node->children[i] != NULL) children++;
    }

    return children==0;
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
        fprintf(stderr, "Il body si trova al di fuori dei confini spaziali del nodo:\n"
                        "\t body: {%2.f,%.2f,%.2f,}\n}", body->x, body->y, body->z);
        return;
    }
    int i;

    OctreeNode* prevNode = root;
    BoundingBox currentBbox = root->bbox;
    int isLeaf = checkIfNodeIsLeaf(prevNode);
#ifdef DEBUG
    printf("Is leaf: %d\n",isLeaf);
    printf("body:[%d,%.2f,%.2f,%.2f]\n", prevNode->id, body->x, body->y, body->z);
#endif

    if (isLeaf) {
        if (prevNode->body == NULL){ prevNode->body = body;}
        else {
            // è già presente un body, quindi scomponiamo il nodo
            divideNodeIntoOctree(prevNode);
            // non è più una foglia
            isLeaf = 0;
        }
    }

    OctreeNode* currentNode = NULL;
    while (!isLeaf) {
        // il nodo ha dei figli
        for (i=0; i<OCTREE_CHILDREN_SIZE; i++) {
            currentNode = prevNode->children[i];
            if (currentNode == NULL) {continue;}
            currentBbox = currentNode->bbox;

            int bodyShouldBeInsideNode = body->x >= currentBbox.min_x && body->x < currentBbox.max_x &&
                    body->y >= currentBbox.min_y && body->y < currentBbox.max_y &&
                    body->z >= currentBbox.min_z && body->z < currentBbox.max_z;

            if (bodyShouldBeInsideNode ) {
                isLeaf = checkIfNodeIsLeaf(currentNode);
                if (isLeaf) {
                    if (currentNode->body != NULL){
                        divideNodeIntoOctree(currentNode);
                        prevNode = currentNode;
                        isLeaf = 0;
                        break;
                    }
                    else {
                        currentNode->body = body;
                        isLeaf=1;
                        break;
                    }
                }else {
                    prevNode = currentNode;
                    break;
                }

            }

        }
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
    printf("(%d) -> {\nmin_x=%.2f,\nmax_x=%.2f,\nmin_y=%.2f,\nmax_y=%.2f,\nmin_z=%.2f,\nmax_z=%.2f,\n]\n\n",
        node->id, node->bbox.min_x, node->bbox.max_x,node->bbox.min_y,node->bbox.max_y,node->bbox.min_z,node->bbox.max_z);
#endif

    // i è l'indice che gestisce lo spostamento sulle x
    int i;
    for (i=1; i<=2; i++) {
        // j è l'indice che gestisce lo spostamento sulle y
        int j;
        for (j=1; j<=2; j++) {
            // k è l'indice che gestisce lo spostamento sulle z
            int k;
            // generazione del cubo su ogni asse
            for (k=1; k<=2; k++) {
                OctreeNode* newNode = newOctreeNode();
                BoundingBox* newBbox = &(newNode->bbox);
                newBbox->min_x = box->min_x + (i-1) * xsize;
                newBbox->min_y = box->min_y + (j-1) * ysize;
                newBbox->min_z = box->min_z + (k-1) * zsize;
                newBbox->max_x = box->min_x + i * xsize;
                newBbox->max_y = box->min_y + j * ysize;
                newBbox->max_z = box->min_z + k * zsize;
                node->children[childrenCounter++] = newNode;
#ifdef DEBUG
                printf("[(%d),%d,%d,%d] -> {\nmin_x=%.2f,\nmax_x=%.2f,\nmin_y=%.2f,\nmax_y=%.2f,\nmin_z=%.2f,\nmax_z=%.2f,\n]\n\n",
                    node->id, i, j, k, newBbox->min_x, newBbox->max_x,newBbox->min_y,newBbox->max_y,newBbox->min_z,newBbox->max_z);
                printf("children: %d\n",childrenCounter);
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

    if (depth == 0)    printf("################################################\n"
        "\t\tPRINT\n"
           "################################################\n");

    printf("%d -> {\nmin_x=%.2f,\nmax_x=%.2f,\nmin_y=%.2f,\nmax_y=%.2f,\nmin_z=%.2f,\nmax_z=%.2f,\n]\n\n",
    node->id, node->bbox.min_x, node->bbox.max_x,node->bbox.min_y,node->bbox.max_y,node->bbox.min_z,node->bbox.max_z);


    if (checkIfNodeIsLeaf(node)) {
        printf("Leaf node #%d, depth %d\n", node->id, depth);
        if (node->body != NULL) {
            printf("\tFound body:[%d,%.2f,%.2f,%.2f]\n", node->id,node->body->x, node->body->y, node->body->z);
        }
        return;
    }

    int i;
    for (i=0; i<OCTREE_CHILDREN_SIZE; i++) {
        traverseOctree(node->children[i], depth+1);
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

    if (body->x < node->bbox.min_x || body->x > node->bbox.max_x ||
    body->y < node->bbox.min_y || body->y > node->bbox.max_y ||
    body->z < node->bbox.min_z || body->z > node->bbox.max_z
    ) {
        return 1;
    }

    return 0;
}

OctreeNode* buildOctree(Body* bodies, int n) {
    resetOctreePool();
    OctreeNode* root = newOctreeNode();

    BoundingBox* bbox = &root->bbox;
    float dimension = 1000;
    bbox->min_x = -dimension;
    bbox->min_y = -dimension ;
    bbox->min_z =-dimension ;
    bbox->max_x =dimension;
    bbox->max_y = dimension;
    bbox->max_z = dimension;

    int i;
    for (i=0; i<n; i++) {
        insertBody(root, bodies+i);
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

    // Push della radice
    stack[stack_ptr++] = root;

    while (stack_ptr > 0) {
        // Pop del nodo
        OctreeNode* node = stack[--stack_ptr];

        // Se il nodo è vuoto o non ha massa, lo ignoriamo
        if (node == NULL || node->mass == 0.0f) continue;

        // Se il nodo è una foglia e contiene la particella stessa, saltiamo
        // (una particella non esercita gravità su se stessa)
        if (checkIfNodeIsLeaf(node) && node->body == target) continue;

        // Calcolo delle distanze sui tre assi
        float dx = node->cx - target->x;
        float dy = node->cy - target->y;
        float dz = node->cz - target->z;

        // Distanza al quadrato con parametro SOFTENING per evitare div/0 o forze infinite
        float dist_sq = dx*dx + dy*dy + dz*dz + SOFTENING*SOFTENING;
        float dist = sqrtf(dist_sq);

        // s: dimensione del nodo (supponendo nodi cubici, basta un solo asse)
        float s = node->bbox.max_x - node->bbox.min_x;

        // Condizione Barnes-Hut
        if (checkIfNodeIsLeaf(node) || (s / dist) < theta) {
            // Il nodo è abbastanza lontano (o è un corpo singolo).
            // Calcoliamo l'interazione con il suo centro di massa complessivo.

            // Accelerazione = G * M / d^2.
            // Per trovare i vettori, dividiamo per 'dist' ottenendo (G * M / d^3)
            // e moltiplichiamo poi per dx, dy, dz.
            float a = (G * node->mass) / (dist_sq * dist);

            acc_x += a * dx;
            acc_y += a * dy;
            acc_z += a * dz;
        } else {
            // Il nodo è troppo vicino e non è una foglia: dobbiamo esplorare i suoi figli.
            int i;
            for (i = 0; i < OCTREE_CHILDREN_SIZE; i++) {
                if (node->children[i] != NULL) {
                    if (stack_ptr < MAX_DEPTH) {
                        stack[stack_ptr++] = node->children[i]; // Push del figlio
                    } else {
                        fprintf(stderr, "Stack overflow durante calculateForce!\n");
                        return;
                    }
                }
            }
        }
    }

    // F = m * a
    *fx = target->m * acc_x;
    *fy = target->m * acc_y;
    *fz = target->m * acc_z;
}


/*
 * Aggiorna la cinematica di tutti i corpi nella simulazione.
 * dt: delta time (intervallo di tempo per ogni step, es. 0.01)
 * theta: parametro di accuratezza del Barnes-Hut (es. 0.5)
 */
void updatePhysics(Body* bodies, int numBodies, OctreeNode* root, float theta, float dt) {
    if (root == NULL || bodies == NULL) return;

    int i;
    for (i = 0; i < numBodies; i++) {
        float fx = 0.0f, fy = 0.0f, fz = 0.0f;

        // 1. Calcola la forza netta agente sulla particella i-esima
        calculateForce(root, &bodies[i], theta, &fx, &fy, &fz);

        // 2. Ricava l'accelerazione (a = F / m)
        float ax = fx / bodies[i].m;
        float ay = fy / bodies[i].m;
        float az = fz / bodies[i].m;

        // 3. Aggiorna la velocità usando l'accelerazione corrente
        bodies[i].vx += ax * dt;
        bodies[i].vy += ay * dt;
        bodies[i].vz += az * dt;

        // 4. Aggiorna la posizione usando la NUOVA velocità (Eulero Simplettico)
        bodies[i].x += bodies[i].vx * dt;
        bodies[i].y += bodies[i].vy * dt;
        bodies[i].z += bodies[i].vz * dt;
    }
}