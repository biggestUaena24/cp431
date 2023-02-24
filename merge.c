#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

void merge(int *a, int *b, int *c, int n, int m)
{
    int i = 0, j = 0, k = 0;

    while (i < n && j < m) {
        if (a[i] < b[j])
            c[k++] = a[i++];
        else
            c[k++] = b[j++];
    }

    while (i < n)
        c[k++] = a[i++];

    while (j < m)
        c[k++] = b[j++];
}

int main(int argc, char **argv)
{
    int rank, size, i, j, n, m, k, *a, *b, *c, *temp;
    double start_time, end_time;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    n = 1000000;
    m = 1000000;
    k = n / size;

    a = (int *) malloc(n * sizeof(int));
    b = (int *) malloc(m * sizeof(int));
    c = (int *) malloc((n + m) * sizeof(int));

    // generate random sorted arrays
    srand(12345);
    for (i = 0; i < n; i++)
        a[i] = rand();

    for (i = 0; i < m; i++)
        b[i] = rand();

    // partition the arrays
    int *as = (int *) malloc(k * sizeof(int));
    int *bs = (int *) malloc(k * sizeof(int));
    int *cs = (int *) malloc(2 * k * sizeof(int));

    MPI_Scatter(a, k, MPI_INT, as, k, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(b, k, MPI_INT, bs, k, MPI_INT, 0, MPI_COMM_WORLD);

    start_time = MPI_Wtime();

    // perform merging locally
    merge(as, bs, cs, k, k);

    // merge results
    for (i = 0; i < k; i++)
        c[i + rank * k] = cs[i];

    for (i = 1; i < size; i++) {
        if (rank == i) {
            MPI_Send(cs, k, MPI_INT, 0, 0, MPI_COMM_WORLD);
        } else if (rank == 0) {
            MPI_Recv(cs, k, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            for (j = 0; j < k; j++)
                c[j + i * k] = cs[j];
        }
    }

    // perform final merging
    if (rank == 0) {
        temp = (int *) malloc((n + m) * sizeof(int));

        merge(c, &c[n/size], temp, n/size, n - n/size);

        for (i = 1; i < size; i++) {
            MPI_Recv(&temp[i * k], k, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        merge(c, temp, temp + n, n, m);

        end_time = MPI_Wtime();

        printf("Time taken = %lf seconds\n", end_time - start_time);
    }

    MPI_Finalize();
    return 0;
}
