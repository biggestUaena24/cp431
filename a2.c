#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define ARRAY_SIZE 1000000000

int cmp(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

// function to merge 2 sorted arrays into a third array
void merge(int *a, int *b, int *c, int n, int m) {
    int i = 0, j = 0, k = 0;
    while (i < n && j < m) {
        if (a[i] < b[j]) {
            c[k++] = a[i++];
        } else {
            c[k++] = b[j++];
        }
    }
    while (i < n) {
        c[k++] = a[i++];
    }
    while (j < m) {
        c[k++] = b[j++];
    }
}

int main(int argc, char** argv) {
    int rank, size;
    int *arr1, *arr2, *merged_arr;
    int i, j;
    double start_time, end_time;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    arr1 = (int*) malloc(sizeof(int) * ARRAY_SIZE);
    arr2 = (int*) malloc(sizeof(int) * ARRAY_SIZE);
    merged_arr = (int*) malloc(sizeof(int) * ARRAY_SIZE * 2);

    // generate random sorted arrays
    srand(1234);
    for (i = 0; i < ARRAY_SIZE; i++) {
        arr1[i] = rand() % 100;
        if (i > 0 && arr1[i] < arr1[i-1]) {
            arr1[i] += arr1[i-1];
            arr1[i-1] = arr1[i] - arr1[i-1];
            arr1[i] -= arr1[i-1];
            i = (i == 1) ? 0 : i - 2;
        }
    }

    for (i = 0; i < ARRAY_SIZE; i++) {
        arr2[i] = rand() % 100;
        if (i > 0 && arr2[i] < arr2[i-1]) {
            arr2[i] += arr2[i-1];
            arr2[i-1] = arr2[i] - arr2[i-1];
            arr2[i] -= arr2[i-1];
            i = (i == 1) ? 0 : i - 2;
        }
    }

    // start the timer
    start_time = MPI_Wtime();

    // scatter the arrays to all processes
    int local_size = ARRAY_SIZE / size;
    int *local_arr1 = (int*) malloc(sizeof(int) * local_size);
    int *local_arr2 = (int*) malloc(sizeof(int) * local_size);
    MPI_Scatter(arr1, local_size, MPI_INT, local_arr1, local_size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(arr2, local_size, MPI_INT, local_arr2, local_size, MPI_INT, 0, MPI_COMM_WORLD);

    // perform the merge in parallel
    int *local_result = (int*) malloc(sizeof(int) * local_size * 2);
    merge(local_arr1, local_arr2, local_result, local_size, local_size);

    // gather the results
    MPI_Gather(local_result, local_size*2, MPI_INT, merged_arr, local_size*2, MPI_INT, 0, MPI_COMM_WORLD);

    // end the timer
    end_time = MPI_Wtime();

    if (rank == 0) {
        printf("Time taken: %f seconds\n", end_time - start_time);
        printf("Merged array:\n");
        for (int i = 0; i < 2 * ARRAY_SIZE; i++) {
            printf("%d ", merged_arr[i]);
        }
        printf("\n");

        free(arr1);
        free(arr2);
        free(merged_arr);
    }

    free(local_arr1);
    free(local_arr2);

    MPI_Finalize();

    return 0;
}
