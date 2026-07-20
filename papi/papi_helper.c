#include "papi_helper.h"

#include <stdio.h>
#include <stdlib.h>

// Array statico contenente i codici degli eventi da monitorare
static int event_codes[NUM_EVENTS] = {
    PAPI_L1_DCM,   // Livello 1 Data Cache Misses
    PAPI_L2_DCM,   // Livello 2 Data Cache Misses
    PAPI_RES_STL,  // Cicli di stall sulle risorse
    PAPI_FP_INS    // Floating point instructions
};

int papi_helper_init(Papi_Monitor* monitor) {
    int retval;

    // 1. Inizializza la libreria PAPI
    retval = PAPI_library_init(PAPI_VER_CURRENT);
    if (retval != PAPI_VER_CURRENT && retval > 0) {
        fprintf(stderr, "PAPI library version mismatch!\n");
        return -1;
    } else if (retval < 0) {
        fprintf(stderr, "PAPI library init error: %s\n", PAPI_strerror(retval));
        return -1;
    }

    // 2. Inizializza l'EventSet a PAPI_NULL
    monitor->event_set = PAPI_NULL;

    // 3. Crea l'EventSet
    if ((retval = PAPI_create_eventset(&(monitor->event_set))) != PAPI_OK) {
        fprintf(stderr, "Errore nella creazione dell'EventSet: %s\n", PAPI_strerror(retval));
        return -1;
    }

    // 4. Aggiungi tutti gli eventi definiti nell'array all'unico EventSet
    for (int i = 0; i < NUM_EVENTS; i++) {
        retval = PAPI_add_event(monitor->event_set, event_codes[i]);
        if (retval != PAPI_OK) {
            fprintf(stderr, "Errore nell'aggiunta dell'evento %d: %s\n", i, PAPI_strerror(retval));
            return -1;
        }

        // Azzera il valore iniziale
        monitor->values[i] = 0;
    }

    return 0;  // Successo
}

int papi_helper_start(Papi_Monitor* monitor) {
    int retval = PAPI_start(monitor->event_set);
    if (retval != PAPI_OK) {
        fprintf(stderr, "Errore nell'avvio di PAPI: %s\n", PAPI_strerror(retval));
        return -1;
    }
    return 0;
}

int papi_helper_stop(Papi_Monitor* monitor) {
    // PAPI_stop legge i contatori e li scrive nell'array di output
    int retval = PAPI_stop(monitor->event_set, monitor->values);
    if (retval != PAPI_OK) {
        fprintf(stderr, "Errore nell'arresto di PAPI: %s\n", PAPI_strerror(retval));
        return -1;
    }
    return 0;
}

void papi_helper_print(Papi_Monitor* monitor) {
    printf("\n=== METRICHE PAPI ===\n");
    printf("L1 Data Cache Misses (billions): %.4f\n", monitor->values[L1_CACHE_MISS_INDEX] / 1e9);
    printf("L2 Data Cache Misses (billions): %.4f\n", monitor->values[L2_CACHE_MISS_INDEX] / 1e9);
    printf("Resource Cycle Stalls (billions): %.4f\n", monitor->values[CYCLE_STALLS_INDEX] / 1e9);
    printf("Floating point instructions (billions): %.4f\n",
           monitor->values[FP_ISTRUCTIONS_INDEX] / 1e9);
    printf("======================\n");
}
