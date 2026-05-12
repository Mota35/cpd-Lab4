#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#define N 100000

// ======================================================
// MY_BCAST -> ARVORE BINARIA
// ======================================================

void my_bcast(void *buf,
              int count,
              MPI_Datatype type,
              int root,
              MPI_Comm comm)
{
    int id, p;

    MPI_Comm_rank(comm, &id);
    MPI_Comm_size(comm, &p);

    int step = 1;

    while(step < p){

        // processos que enviam
        if(id < step){

            int dest = id + step;

            if(dest < p){

                MPI_Send(buf,
                         count,
                         type,
                         dest,
                         0,
                         comm);
            }
        }

        // processos que recebem
        else if(id < 2 * step){

            int source = id - step;

            MPI_Recv(buf,
                     count,
                     type,
                     source,
                     0,
                     comm,
                     MPI_STATUS_IGNORE);
        }

        step *= 2;
    }
}

// ======================================================
// MY_SCATTER
// ======================================================

void my_scatter(void *sendbuf,
                int sendcount,
                MPI_Datatype sendtype,
                void *recvbuf,
                int recvcount,
                MPI_Datatype recvtype,
                int root,
                MPI_Comm comm)
{
    int id, p;

    MPI_Comm_rank(comm, &id);
    MPI_Comm_size(comm, &p);

    int typesize;

    MPI_Type_size(sendtype, &typesize);

    if(id == root){

        // envia para todos
        for(int dest = 0; dest < p; dest++){

            if(dest == root){

                memcpy(recvbuf,
                       (char*)sendbuf + dest * sendcount * typesize,
                       sendcount * typesize);
            }

            else{

                MPI_Send((char*)sendbuf + dest * sendcount * typesize,
                         sendcount,
                         sendtype,
                         dest,
                         0,
                         comm);
            }
        }
    }

    else{

        MPI_Recv(recvbuf,
                 recvcount,
                 recvtype,
                 root,
                 0,
                 comm,
                 MPI_STATUS_IGNORE);
    }
}

// ======================================================
// MY_SCATTERV
// ======================================================

void my_scatterv(void *sendbuf,
                 int *sendcounts,
                 int *displs,
                 MPI_Datatype sendtype,
                 void *recvbuf,
                 int recvcount,
                 MPI_Datatype recvtype,
                 int root,
                 MPI_Comm comm)
{
    int id, p;

    MPI_Comm_rank(comm, &id);
    MPI_Comm_size(comm, &p);

    int typesize;

    MPI_Type_size(sendtype, &typesize);

    if(id == root){

        for(int dest = 0; dest < p; dest++){

            if(dest == root){

                memcpy(recvbuf,
                       (char*)sendbuf + displs[dest] * typesize,
                       sendcounts[dest] * typesize);
            }

            else{

                MPI_Send((char*)sendbuf + displs[dest] * typesize,
                         sendcounts[dest],
                         sendtype,
                         dest,
                         0,
                         comm);
            }
        }
    }

    else{

        MPI_Recv(recvbuf,
                 recvcount,
                 recvtype,
                 root,
                 0,
                 comm,
                 MPI_STATUS_IGNORE);
    }
}

// ======================================================
// MAIN
// ======================================================

int main(int argc, char *argv[])
{
    int id, p;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    int *array = malloc(N * sizeof(int));

    int local_n = N / p;

    int *recvbuf = malloc(local_n * sizeof(int));

    if(id == 0){

        for(int i = 0; i < N; i++){

            array[i] = i;
        }
    }

    double t1, t2;

    // ==================================================
    // TESTE MY_BCAST
    // ==================================================

    MPI_Barrier(MPI_COMM_WORLD);

    t1 = -MPI_Wtime();

    my_bcast(array,
             N,
             MPI_INT,
             0,
             MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);

    t1 += MPI_Wtime();

    // ==================================================
    // TESTE MPI_BCAST
    // ==================================================

    MPI_Barrier(MPI_COMM_WORLD);

    t2 = -MPI_Wtime();

    MPI_Bcast(array,
              N,
              MPI_INT,
              0,
              MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);

    t2 += MPI_Wtime();

    if(id == 0){

        printf("\n====================================\n");
        printf("BCAST COMPARISON\n");
        printf("====================================\n");

        printf("my_bcast    : %f ms\n", t1 * 1000);

        printf("MPI_Bcast   : %f ms\n", t2 * 1000);
    }

    // ==================================================
    // TESTE MY_SCATTER
    // ==================================================

    MPI_Barrier(MPI_COMM_WORLD);

    t1 = -MPI_Wtime();

    my_scatter(array,
               local_n,
               MPI_INT,
               recvbuf,
               local_n,
               MPI_INT,
               0,
               MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);

    t1 += MPI_Wtime();

    // ==================================================
    // TESTE MPI_SCATTER
    // ==================================================

    MPI_Barrier(MPI_COMM_WORLD);

    t2 = -MPI_Wtime();

    MPI_Scatter(array,
                local_n,
                MPI_INT,
                recvbuf,
                local_n,
                MPI_INT,
                0,
                MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);

    t2 += MPI_Wtime();

    if(id == 0){

        printf("\n====================================\n");
        printf("SCATTER COMPARISON\n");
        printf("====================================\n");

        printf("my_scatter  : %f ms\n", t1 * 1000);

        printf("MPI_Scatter : %f ms\n", t2 * 1000);
    }

    // ==================================================
    // TESTE MY_SCATTERV
    // ==================================================

    int *sendcounts = malloc(p * sizeof(int));

    int *displs = malloc(p * sizeof(int));

    for(int i = 0; i < p; i++){

        sendcounts[i] = local_n;

        displs[i] = i * local_n;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    t1 = -MPI_Wtime();

    my_scatterv(array,
                sendcounts,
                displs,
                MPI_INT,
                recvbuf,
                local_n,
                MPI_INT,
                0,
                MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);

    t1 += MPI_Wtime();

    // ==================================================
    // TESTE MPI_SCATTERV
    // ==================================================

    MPI_Barrier(MPI_COMM_WORLD);

    t2 = -MPI_Wtime();

    MPI_Scatterv(array,
                 sendcounts,
                 displs,
                 MPI_INT,
                 recvbuf,
                 local_n,
                 MPI_INT,
                 0,
                 MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);

    t2 += MPI_Wtime();

    if(id == 0){

        printf("\n====================================\n");
        printf("SCATTERV COMPARISON\n");
        printf("====================================\n");

        printf("my_scatterv : %f ms\n", t1 * 1000);

        printf("MPI_Scatterv: %f ms\n", t2 * 1000);

        printf("====================================\n");
    }

    free(array);
    free(recvbuf);
    free(sendcounts);
    free(displs);

    MPI_Finalize();

    return 0;
}
