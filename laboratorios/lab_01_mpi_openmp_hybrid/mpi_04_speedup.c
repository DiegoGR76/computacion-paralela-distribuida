#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#define N 1000000

int main(int argc, char** argv) {
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int chunk = N / size;

    // ── Versión paralela (MPI + OpenMP) ──────────────────────
    long long *arr = NULL;
    if (rank == 0) {
        arr = (long long*) malloc(N * sizeof(long long));
        for (int i = 0; i < N; i++) arr[i] = i;
    }

    long long *local = (long long*) malloc(chunk * sizeof(long long));

    MPI_Barrier(MPI_COMM_WORLD);          // sincronizar antes de medir
    double t_inicio = MPI_Wtime();

    MPI_Scatter(arr, chunk, MPI_LONG_LONG,
                local, chunk, MPI_LONG_LONG,
                0, MPI_COMM_WORLD);

    long long suma_local = 0;
    #pragma omp parallel for reduction(+:suma_local)
    for (int i = 0; i < chunk; i++)
        suma_local += local[i];

    long long suma_total = 0;
    MPI_Reduce(&suma_local, &suma_total, 1,
               MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);
    double t_fin = MPI_Wtime();

    // ── Versión secuencial (solo rank 0, para comparar) ──────
    if (rank == 0) {
        printf("Suma total  = %lld\n", suma_total);
        printf("Esperado    = %lld\n", (long long)N*(N-1)/2);
        printf("Tiempo paralelo: %.4f seg\n", t_fin - t_inicio);

        // TODO: for secuencial de referencia
        long long suma_seq = 0;
        double ts = MPI_Wtime();
        for (int i = 0; i < N; i++)
            suma_seq += (long long)i;
        double te = MPI_Wtime();

        double t_seq = te - ts;
        double t_par = t_fin - t_inicio;

        printf("Tiempo secuencial: %.4f seg\n", t_seq);
        printf("Speedup: %.2fx\n", t_seq / t_par);

        free(arr);
    }

    free(local);
    MPI_Finalize();
    return 0;
}