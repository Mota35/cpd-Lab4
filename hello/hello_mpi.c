#include <stdio.h>
#include <mpi.h>

int main(int argc, char *argv[]) {
    int id, p;
    char hostname[256];
    int namelen;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    MPI_Get_processor_name(hostname, &namelen);

    printf("O Processo %d envia cumprimentos a partir da maquina %s!\n",
           id, hostname);

    MPI_Finalize();
    return 0;
}
