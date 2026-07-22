#ifndef PAPI_HELPER_H
#define PAPI_HELPER_H

#include <papi.h>

// Definiamo quanti eventi vogliamo tracciare
#define NUM_EVENTS 5

// Indici comodi per accedere ai risultati
#define L1_CACHE_MISS_INDEX 0
#define L2_CACHE_MISS_INDEX 1
#define SP_SIMD_ISTR_INDEX 2
#define DP_SIMD_ISTR_INDEX 3
#define FP_ISTRUCTIONS_INDEX 4

// Struttura per gestire i risultati dei counter
typedef struct {
    long_long values[NUM_EVENTS];  // PAPI usa long_long (64-bit interi) per i contatori
    int event_set;                 // Un unico EventSet per tutti gli eventi
} Papi_Monitor;

// Funzioni pubbliche del modulo
int papi_helper_init(Papi_Monitor* monitor);
int papi_helper_start(Papi_Monitor* monitor);
int papi_helper_stop(Papi_Monitor* monitor);
void papi_helper_print(Papi_Monitor* monitor);

#endif  // PAPI_HELPER_H
