#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>
 
/* Número de repetições por tamanho de mensagem.
   Mais repetições = resultado mais estável. */
#define ROUNDS 1000
 
/* Tamanhos de mensagem a testar (em bytes) */
int tamanhos[] = {
    1,          /*   1 B  — mede a latência pura          */
    64,         /*  64 B                                   */
    512,        /* 512 B                                   */
    4096,       /*   4 KB                                  */
    16384,      /*  16 KB                                  */
    65536,      /*  64 KB                                  */
    262144,     /* 256 KB                                  */
    1048576     /*   1 MB — mede a largura de banda pura   */
};
 
#define N_TAMANHOS (sizeof(tamanhos) / sizeof(tamanhos[0]))
 
int main(int argc, char *argv[]) {
 
    int id, p, i, s;
    MPI_Status status;
    double t_inicio, t_fim, t_total;
    double t_medio_us;     /* tempo médio de round-trip em microssegundos */
    double t_unidirec_us;  /* tempo unidireccional = round-trip / 2       */
    double bw_mbps;        /* largura de banda em MB/s                    */
 
    /* Estimativas finais */
    double latencia_us = 0.0;
    double bw_estimada = 0.0;
 
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
 
    /* Este programa requer exactamente 2 processos */
    if (p != 2) {
        if (id == 0)
            printf("ERRO: Este programa deve ser executado com exactamente 2 processos.\n");
            printf("      Use: mpirun -np 2 ./latencia_bw\n");
        MPI_Finalize();
        return 1;
    }
 
    /* Cabeçalho — só o processo 0 imprime */
    if (id == 0) {
        printf("\n");
        printf("========================================================\n");
        printf("   Medição de Latência e Largura de Banda MPI\n");
        printf("   Rounds por tamanho: %d\n", ROUNDS);
        printf("========================================================\n");
        printf("%-15s %-20s %-20s %-20s\n",
               "Tamanho (B)",
               "Round-trip (µs)",
               "Unidirec. (µs)",
               "Largura banda (MB/s)");
        printf("%-15s %-20s %-20s %-20s\n",
               "---------------",
               "--------------------",
               "--------------------",
               "--------------------");
    }
 
    /* ----------------------------------------------------------------
     * Loop principal: testa cada tamanho de mensagem
     * ---------------------------------------------------------------- */
    for (s = 0; s < N_TAMANHOS; s++) {
 
        int tam = tamanhos[s];
 
        /* Aloca o buffer de envio/recepção */
        char *buf = (char *) malloc(tam);
        if (!buf) {
            fprintf(stderr, "Processo %d: erro ao alocar %d bytes\n", id, tam);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        /* Inicializa o buffer com um valor qualquer */
        memset(buf, id, tam);
 
        /* Sincroniza todos os processos antes de medir */
        MPI_Barrier(MPI_COMM_WORLD);
 
        t_inicio = MPI_Wtime();
 
        /* ---- Ping-Pong: ROUNDS vezes ---- */
        for (i = 0; i < ROUNDS; i++) {
 
            if (id == 0) {
                /* Processo 0: envia e espera a resposta */
                MPI_Send(buf, tam, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
                MPI_Recv(buf, tam, MPI_CHAR, 1, 0, MPI_COMM_WORLD, &status);
            } else {
                /* Processo 1: recebe e devolve */
                MPI_Recv(buf, tam, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);
                MPI_Send(buf, tam, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
            }
        }
 
        t_fim = MPI_Wtime();
        t_total = t_fim - t_inicio;
 
        /* Cálculos — só o processo 0 apresenta */
        if (id == 0) {
 
            /* Tempo médio de round-trip em microssegundos */
            t_medio_us    = (t_total / ROUNDS) * 1e6;
 
            /* Tempo unidireccional (uma direcção só) */
            t_unidirec_us = t_medio_us / 2.0;
 
            /* Largura de banda: bytes transferidos por segundo → MB/s
               Nota: num round-trip transferem-se tam bytes em cada direcção */
            bw_mbps = (tam / (t_unidirec_us * 1e-6)) / (1024.0 * 1024.0);
 
            printf("%-15d %-20.3f %-20.3f %-20.3f\n",
                   tam, t_medio_us, t_unidirec_us, bw_mbps);
 
            /* Guarda a latência (medida com a mensagem de 1 byte) */
            if (s == 0)
                latencia_us = t_unidirec_us;
 
            /* Guarda a BW estimada com a mensagem maior */
            if (s == N_TAMANHOS - 1)
                bw_estimada = bw_mbps;
        }
 
        free(buf);
 
    } /* fim do loop de tamanhos */
 
    /* ----------------------------------------------------------------
     * Sumário final
     * ---------------------------------------------------------------- */
    if (id == 0) {
        printf("========================================================\n");
        printf("\nRESUMO:\n");
        printf("  Latência estimada (L) = %.3f µs\n", latencia_us);
        printf("  Largura de banda (BW) = %.2f MB/s\n", bw_estimada);
        printf("\n  Modelo: T(n) = %.3f µs + n / %.2f MB/s\n",
               latencia_us, bw_estimada);
        printf("\n  Verificação do modelo para 64 KB:\n");
        double n = 65536.0;
        double t_modelo = latencia_us + (n / (1024.0*1024.0)) / bw_estimada * 1e6;
        printf("    T_modelo(64KB) = %.3f µs\n", t_modelo);
        printf("========================================================\n\n");
    }
 
    MPI_Finalize();
    return 0;
}
