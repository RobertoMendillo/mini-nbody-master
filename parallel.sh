#!/bin/bash
OUTPUT_FILE="results_parallel.data"

printf "Esecuzione parallela\n" > $OUTPUT_FILE

mpicc -O0 parallelized_nbody/nbody_mpi.c ./papi/papi_helper.c -lm -I./papi/ -I. /usr/local/lib/libpapi.a -o nbody_mpi.out

printf "\n\n================ 30_0000 ================\n\n" >> $OUTPUT_FILE 2>&1

printf "\n30_000 corpi tcpip/ethernet\n" >> $OUTPUT_FILE
mpirun --mca btl self,tcp --mca btl_tcp_if_include em2 -machinefile machinefile_p.txt nbody_mpi.out >> $OUTPUT_FILE 2>&1

printf "\n30_000 corpi tcpip/infiniband\n" >> $OUTPUT_FILE
mpirun --mca btl self,tcp --mca btl_tcp_if_include ib0 -machinefile machinefile_p.txt nbody_mpi.out >> $OUTPUT_FILE 2>&1

printf "\n30_000 corpi native/infiniband\n" >> $OUTPUT_FILE
mpirun --mca btl self,openib -machinefile machinefile_p.txt nbody_mpi.out >> $OUTPUT_FILE 2>&1

printf "\n\n================ 50_0000 ================\n\n" >> $OUTPUT_FILE 2>&1

printf "\n50_000 corpi tcpip/ethernet\n" >> $OUTPUT_FILE
mpirun --mca btl self,tcp --mca btl_tcp_if_include em2 -machinefile machinefile_p.txt nbody_mpi.out 50000 >> $OUTPUT_FILE 2>&1

printf "\n50_000 corpi tcpip/infiniband\n" >> $OUTPUT_FILE
mpirun --mca btl self,tcp --mca btl_tcp_if_include ib0 -machinefile machinefile_p.txt nbody_mpi.out 50000 >> $OUTPUT_FILE 2>&1

printf "\n50_000 corpi native/infiniband\n" >> $OUTPUT_FILE
mpirun --mca btl self,openib -machinefile machinefile_p.txt nbody_mpi.out 50000 >> $OUTPUT_FILE 2>&1

printf "\n\n================ 100_0000 ================\n\n" >> $OUTPUT_FILE 2>&1

printf "\n50_000 corpi tcpip/ethernet\n" >> $OUTPUT_FILE
mpirun --mca btl self,tcp --mca btl_tcp_if_include em2 -machinefile machinefile_p.txt nbody_mpi.out 10000 >> $OUTPUT_FILE 2>&1

printf "\n50_000 corpi tcpip/infiniband\n" >> $OUTPUT_FILE
mpirun --mca btl self,tcp --mca btl_tcp_if_include ib0 -machinefile machinefile_p.txt nbody_mpi.out 10000 >> $OUTPUT_FILE 2>&1

printf "\n50_000 corpi native/infiniband\n" >> $OUTPUT_FILE
mpirun --mca btl self,openib -machinefile machinefile_p.txt nbody_mpi.out 10000 >> $OUTPUT_FILE 2>&1

printf "\n\n================ 200_0000 ================\n\n" >> $OUTPUT_FILE 2>&1

printf "\n50_000 corpi tcpip/ethernet\n" >> $OUTPUT_FILE
mpirun --mca btl self,tcp --mca btl_tcp_if_include em2 -machinefile machinefile_p.txt nbody_mpi.out 200000 >> $OUTPUT_FILE 2>&1

printf "\n50_000 corpi tcpip/infiniband\n" >> $OUTPUT_FILE
mpirun --mca btl self,tcp --mca btl_tcp_if_include ib0 -machinefile machinefile_p.txt nbody_mpi.out 200000 >> $OUTPUT_FILE 2>&1

printf "\n50_000 corpi native/infiniband\n" >> $OUTPUT_FILE
mpirun --mca btl self,openib -machinefile machinefile_p.txt nbody_mpi.out 200000 >> $OUTPUT_FILE 2>&1