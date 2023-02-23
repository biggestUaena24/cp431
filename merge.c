#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

#define N 10

void merge(int *arr, int l, int m, int r) {
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;
 
    int L[n1], R[n2];
 
    for (i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[m + 1 + j];
 
    i = 0;
    j = 0;
    k = l;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        }
        else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }
 
    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }
 
    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }
}

void parallel_merge(int* A, int* B, int* C, int n, int m) {
    int comm_sz, my_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    int p = comm_sz;
    int local_n = n / p;
    int local_m = m / p;
    int *local_A = (int*)malloc(local_n * sizeof(int));
    int *local_B = (int*)malloc(local_m * sizeof(int));
    int *local_C = (int*)malloc((local_n + local_m) * sizeof(int));

    MPI_Scatter(A, local_n, MPI_INT, local_A, local_n, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(B, local_m, MPI_INT, local_B, local_m, MPI_INT, 0, MPI_COMM_WORLD);

    int i, j, k;
    for (i = 0; i < local_n; i++) {
        local_C[i] = local_A[i];
    }
    for (j = 0; j < local_m; j++) {
        local_C[i + j] = local_B[j];
    }

    for (i = 0; i < p; i++) {
        int sendcount = (i + 1) * local_n < n ? local_n : n - i * local_n;
        int recvcount = (i + 1) * local_n < n ? local_n : n - i * local_n;
        int sendcount_B = (i + 1) * local_m < m ? local_m : m - i * local_m;
        int recvcount_B = (i + 1) * local_m < m ? local_m : m - i * local_m;

        int *recv_B = (int*)malloc(recvcount_B * sizeof(int));
        MPI_Sendrecv(local_A, sendcount, MPI_INT, (my_rank + 1) % p, 0, recv_B, recvcount_B, MPI_INT, (my_rank + p - 1) % p, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        merge(local_C, 0, sendcount - 1, sendcount + recvcount - 1);

        int m = sendcount_B + recvcount_B;
        int *temp = (int*)malloc(m * sizeof(int));
        int idx = 0;
        i = j = 0;
        while (i < sendcount && j < recvcount_B) {
            if (local_C[i] < recv_B[j]) {
                temp[idx++] = local_C[i++];
            }
            else {
                temp[idx++] = recv_B[j++];
            }
        }
        while (i < sendcount) {
            temp[idx++] = local_C[i++];
        }
        while (j < recvcount_B) {
            temp[idx++] = recv_B[j++];
        }

        for (i = 0; i < sendcount; i++) {
            local_C[i] = temp[i];
        }
        for (j = 0; j < recvcount_B; j++) {
            local_C[sendcount + j] = temp[sendcount + j];
        }

        free(recv_B);
        free(temp);
    }

    MPI_Gather(local_C, local_n + local_m, MPI_INT, C, local_n + local_m, MPI_INT, 0, MPI_COMM_WORLD);

    free(local_A);
    free(local_B);
    free(local_C);
}

int main(int argc, char **args) {
    int A[N] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    int B[N] = {1, 3, 5, 7, 9, 11, 13, 15, 17, 19};
    int C[2*N];
    int n = N;
    int m = N;
    int my_rank, comm_sz;
    MPI_Init(&argc, args);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

    double start_time = MPI_Wtime();

    parallel_merge()

    double end_time = MPI_Wtime();
    double total_time = end_time - start_time;

    printf("time taken: %d\n", totla_time);

    int i;
    for (i=0;i<n+m;i++) {
        printf("%d ", C[i]);
    }
    printf("\n");

    return 0;
}
