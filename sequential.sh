echo "Esecuzione sequenziale"
echo ""
mpicc -O0 nbody.c ./papi/papi_helper.c -lm -I./papi/ /usr/local/lib/libpapi.a -o nbody.out
mpirun -np 1 -machinefile machinefile.txt nbody.out
