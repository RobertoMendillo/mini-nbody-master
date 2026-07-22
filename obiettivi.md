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


Weak Scaling: Aumenti $N$ proporzionalmente al numero di nodi/core per mantenere il carico di lavoro per core costante.

Strong Scaling: Tieni $N$ fisso (es. $N = 50.000$) e aumenti i thread/core (1, 2, 4, 8, 16, 32) per misurare lo Speedup ($S_p = \frac{T_1}{T_p}$) e
l'Efficienza Parallela.

Punto di Saturazione: Mostra dove la banda di memoria o la comunicazione di rete (in MPI) diventa il collo di bottiglia e ferma lo speedup (Legge di Amdahl).

Aggiunte
======

✅ I corpi hanno massa costante unitaria, aggiungere una massa variabile




Istruzioni
===========

mpicc: per compilare MPI
mpirun: per eseguire programmi MPI
-I<percorso>: indica al linker dove prendere i file -h
