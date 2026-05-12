

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define N 100000

int main(int argc, char *argv[]) {

    int id, p;
    int *array;
    double t_bcast, t_send;
    MPI_Status status;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    // Alocar memória
    array = (int*) malloc(N * sizeof(int));

    // Processo raiz inicializa o array
    if (id == 0) {
        for (int i = 0; i < N; i++) {
            array[i] = i;
        }
    }

    // =========================
    // Estratégia A: MPI_Bcast
    // =========================

    MPI_Barrier(MPI_COMM_WORLD);

    t_bcast = -MPI_Wtime();

    MPI_Bcast(array, N, MPI_INT, 0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);

    t_bcast += MPI_Wtime();

    // ================================
    // Estratégia B: MPI_Send individual
    // ================================

    MPI_Barrier(MPI_COMM_WORLD);

    t_send = -MPI_Wtime();

    if (id == 0) {

        for (int dest = 1; dest < p; dest++) {

            MPI_Send(array, N, MPI_INT,
                     dest, 0, MPI_COMM_WORLD);
        }

    } else {

        MPI_Recv(array, N, MPI_INT,
                 0, 0, MPI_COMM_WORLD, &status);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    t_send += MPI_Wtime();

    // Mostrar resultados
    if (id == 0) {

        printf("\n====================================\n");
        printf("Comparacao: MPI_Bcast vs MPI_Send\n");
        printf("Numero de processos: %d\n", p);
        printf("Tamanho do array: %d inteiros\n", N);

        printf("\nTempo MPI_Bcast         = %f segundos\n", t_bcast);

        printf("Tempo Send individual   = %f segundos\n", t_send);

        printf("====================================\n");
    }

    free(array);

    MPI_Finalize();

    return 0;
}
