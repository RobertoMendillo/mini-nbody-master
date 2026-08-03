#!/bin/bash
OUTPUT_FILE="results_parallel.data"

printf "Esecuzione parallela\n" > $OUTPUT_FILE

mpicc -O3 parallelized_nbody/nbody_mpi.c ./papi/papi_helper.c -lm -I./papi/ -I. -fopenmp /usr/local/lib/libpapi.a -o nbody_mpi.out

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

printf "\n100_000 corpi tcpip/ethernet\n" >> $OUTPUT_FILE
mpirun --mca btl self,tcp --mca btl_tcp_if_include em2 -machinefile machinefile_p.txt nbody_mpi.out 100000 >> $OUTPUT_FILE 2>&1

printf "\n100_000 corpi tcpip/infiniband\n" >> $OUTPUT_FILE
mpirun --mca btl self,tcp --mca btl_tcp_if_include ib0 -machinefile machinefile_p.txt nbody_mpi.out 100000 >> $OUTPUT_FILE 2>&1

printf "\n100_000 corpi native/infiniband\n" >> $OUTPUT_FILE
mpirun --mca btl self,openib -machinefile machinefile_p.txt nbody_mpi.out 100000 >> $OUTPUT_FILE 2>&1

printf "\n\n================ 200_0000 ================\n\n" >> $OUTPUT_FILE 2>&1

printf "\n200_000 corpi tcpip/ethernet\n" >> $OUTPUT_FILE
mpirun --mca btl self,tcp --mca btl_tcp_if_include em2 -machinefile machinefile_p.txt nbody_mpi.out 200000 >> $OUTPUT_FILE 2>&1

printf "\n200_000 corpi tcpip/infiniband\n" >> $OUTPUT_FILE
mpirun --mca btl self,tcp --mca btl_tcp_if_include ib0 -machinefile machinefile_p.txt nbody_mpi.out 200000 >> $OUTPUT_FILE 2>&1

printf "\n200_000 corpi native/infiniband\n" >> $OUTPUT_FILE
mpirun --mca btl self,openib -machinefile machinefile_p.txt nbody_mpi.out 200000 >> $OUTPUT_FILE 2>&1

printf "\n\n================ 400_0000 ================\n\n" >> $OUTPUT_FILE 2>&1

printf "\n400_000 corpi tcpip/ethernet\n" >> $OUTPUT_FILE
mpirun --mca btl self,tcp --mca btl_tcp_if_include em2 -machinefile machinefile_p.txt nbody_mpi.out 400000 >> $OUTPUT_FILE 2>&1

printf "\n400_000 corpi tcpip/infiniband\n" >> $OUTPUT_FILE
mpirun --mca btl self,tcp --mca btl_tcp_if_include ib0 -machinefile machinefile_p.txt nbody_mpi.out 400000 >> $OUTPUT_FILE 2>&1

printf "\n400_000 corpi native/infiniband\n" >> $OUTPUT_FILE
mpirun --mca btl self,openib -machinefile machinefile_p.txt nbody_mpi.out 400000 >> $OUTPUT_FILE 2>&1

printf "\n\n================ 600_0000 ================\n\n" >> $OUTPUT_FILE 2>&1

printf "\n600_000 corpi tcpip/ethernet\n" >> $OUTPUT_FILE
mpirun --mca btl self,tcp --mca btl_tcp_if_include em2 -machinefile machinefile_p.txt nbody_mpi.out 600000 >> $OUTPUT_FILE 2>&1

printf "\n600_000 corpi tcpip/infiniband\n" >> $OUTPUT_FILE
mpirun --mca btl self,tcp --mca btl_tcp_if_include ib0 -machinefile machinefile_p.txt nbody_mpi.out 600000 >> $OUTPUT_FILE 2>&1

printf "\n600_000 corpi native/infiniband\n" >> $OUTPUT_FILE
mpirun --mca btl self,openib -machinefile machinefile_p.txt nbody_mpi.out 600000 >> $OUTPUT_FILE 2>&1

printf "\n\n================ 800_0000 ================\n\n" >> $OUTPUT_FILE 2>&1

printf "\n800_000 corpi tcpip/ethernet\n" >> $OUTPUT_FILE
mpirun --mca btl self,tcp --mca btl_tcp_if_include em2 -machinefile machinefile_p.txt nbody_mpi.out 800000 >> $OUTPUT_FILE 2>&1

printf "\n800_000 corpi tcpip/infiniband\n" >> $OUTPUT_FILE
mpirun --mca btl self,tcp --mca btl_tcp_if_include ib0 -machinefile machinefile_p.txt nbody_mpi.out 800000 >> $OUTPUT_FILE 2>&1

printf "\n800_000 corpi native/infiniband\n" >> $OUTPUT_FILE
mpirun --mca btl self,openib -machinefile machinefile_p.txt nbody_mpi.out 800000 >> $OUTPUT_FILE 2>&1

printf "\n\n================ 1_000_0000 ================\n\n" >> $OUTPUT_FILE 2>&1

printf "\n1_000_000 corpi tcpip/ethernet\n" >> $OUTPUT_FILE
mpirun --mca btl self,tcp --mca btl_tcp_if_include em2 -machinefile machinefile_p.txt nbody_mpi.out 1000000 >> $OUTPUT_FILE 2>&1

printf "\n1_000_000 corpi tcpip/infiniband\n" >> $OUTPUT_FILE
mpirun --mca btl self,tcp --mca btl_tcp_if_include ib0 -machinefile machinefile_p.txt nbody_mpi.out 1000000 >> $OUTPUT_FILE 2>&1

printf "\n1_000_000 corpi native/infiniband\n" >> $OUTPUT_FILE
mpirun --mca btl self,openib -machinefile machinefile_p.txt nbody_mpi.out 1000000 >> $OUTPUT_FILE 2>&1