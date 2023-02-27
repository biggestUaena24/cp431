#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int n = 10; // length of first array
    int m = 5; // length of second array
    int max_val = 50; // maximum value in first array

    int *arr1 = NULL;
    int *arr2 = NULL;
    int *merged = NULL;

    if (rank == 0) {
        // create and fill the first array with sorted values
        arr1 = (int *)malloc(n * sizeof(int));
        for (int i = 0; i < n; i++) {
            arr1[i] = rand() % max_val;
        }
        qsort(arr1, n, sizeof(int), cmpfunc);

        // create and fill the second array with sorted values
        arr2 = (int *)malloc(m * sizeof(int));
        for (int i = 0; i < m; i++) {
            arr2[i] = rand() % max_val;
        }
        qsort(arr2, m, sizeof(int), cmpfunc);

        // print the arrays
        printf("Array 1: ");
        for (int i = 0; i < n; i++) {
            printf("%d ", arr1[i]);
        }
        printf("\n");

        printf("Array 2: ");
        for (int i = 0; i < m; i++) {
            printf("%d ", arr2[i]);
        }
        printf("\n");

        // allocate memory for the merged array
        merged = (int *)malloc((n + m) * sizeof(int));
    }

    // scatter the indices of the first array across the processes
    int *recvcounts = (int *)malloc(size * sizeof(int));
    int *displs = (int *)malloc(size * sizeof(int));
    for (int i = 0; i < size; i++) {
        recvcounts[i] = n / size;
        if (i < n % size) {
            recvcounts[i]++;
        }
        if (i == 0) {
            displs[i] = 0;
        } else {
            displs[i] = displs[i-1] + recvcounts[i-1];
        }
    }
    int *local_arr1 = (int *)malloc(recvcounts[rank] * sizeof(int));
    MPI_Scatterv(arr1, recvcounts, displs, MPI_INT, local_arr1, recvcounts[rank], MPI_INT, 0, MPI_COMM_WORLD);

    // create a boolean array to indicate which indices of the second array to include
    int *include = (int *)malloc(m * sizeof(int));
    for (int i = 0; i < m; i++) {
        if (arr2[i] <= arr1[n-1]) {
            include[i] = 1;
        } else {
            include[i] = 0;
        }
    }

    // gather the included indices of the second array to the root process
    int *recvcounts2 = (int *)malloc(size * sizeof(int));
    int *displs2 = (int *)malloc(size * sizeof(int));
// determine the number of included indices for each process
int local_count2 = 0;
for (int i = 0; i < m; i++) {
    if (include[i]) {
        local_count2++;
    }
}
MPI_Gather(&local_count2, 1, MPI_INT, recvcounts2, 1, MPI_INT, 0, MPI_COMM_WORLD);

// calculate the displacements for the gathered indices
int total_count2 = 0;
int *included_indices = NULL;
if (rank == 0) {
    for (int i = 0; i < size; i++) {
        if (i == 0) {
            displs2[i] = 0;
        } else {
            displs2[i] = displs2[i-1] + recvcounts2[i-1];
        }
        total_count2 += recvcounts2[i];
    }
    included_indices = (int *)malloc(total_count2 * sizeof(int));
}

// gather the included indices of the second array to the root process
MPI_Gatherv(include, m, MPI_INT, included_indices, recvcounts2, displs2, MPI_INT, 0, MPI_COMM_WORLD);

// merge the two arrays
if (rank == 0) {
    int i = 0, j = 0, k = 0;
    while (i < n && j < total_count2) {
        if (arr1[i] <= arr2[included_indices[j]]) {
            merged[k] = arr1[i];
            i++;
        } else {
            merged[k] = arr2[included_indices[j]];
            j++;
        }
        k++;
    }
    while (i < n) {
        merged[k] = arr1[i];
        i++;
        k++;
    }
    while (j < total_count2) {
        merged[k] = arr2[included_indices[j]];
        j++;
        k++;
    }

    // print the merged array
    printf("Merged Array: ");
    for (int i = 0; i < n + total_count2; i++) {
        printf("%d ", merged[i]);
    }
    printf("\n");

    // free the memory
    free(arr1);
    free(arr2);
    free(merged);
    free(included_indices);
}

free(recvcounts);
free(displs);
free(recvcounts2);
free(displs2);
free(local_arr1);
free(include);

MPI_Finalize();
return 0;
}