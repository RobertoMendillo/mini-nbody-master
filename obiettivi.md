Obiettivi
=========

1. Rendere il codice parallelo sfruttando tutti i processori del singolo nodo
2. Rendere il codice parallelo sfruttando tutti i 4 nodi del cluster
3. Sfruttare al meglio le cache per migliorare le prestazioni (AoSoA). 
4. Utilizzare istruzioni SIMD e valutarne le prestazioni
5. Misurare lo speedup per ogni versione e metterli a confronto
6. Misurare i cache miss con papi e confrontare i casi di AoS e SoA.

7. Creare uno script di benchmark in cui vengono testate diverse configurazioni di:
  - Corpi
  - Iterazioni
  - delta time
  + tecnologia di interconnessione

  Raccogliere i risultati:
  - tempi di esecuzione
  - cache miss
  - latenza di rete


Aggiunte
======

✅ I corpi hanno massa costante unitaria, aggiungere una massa variabile




Istruzioni
===========

mpicc: per compilare MPI
mpirun: per eseguire programmi MPI
-I<percorso>: indica al linker dove prendere i file -h
