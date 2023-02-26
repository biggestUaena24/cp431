#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define ARRAY_SIZE 16


int cmp(const void* a, const void* b) {
    return (*(int*)a - *(int*)b);
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int n = ARRAY_SIZE; // length of first array
    int m = ARRAY_SIZE; // length of second array
    int n_per_proc = log2(n); // number of elements per process

    int *arr1 = NULL;
    int *arr2 = NULL;
    int *merged = NULL;
    int *sub_arr1 = NULL;
    int *sub_arr2 = NULL;

    //Create sorted arrays for arr1 and arr2
    for (int i = 0; i < n; i++) {
        arr1[i] = rand() % 100;
    }
    for (int i =0; i < m; i++) {
        arr2[i] = rand() % 100;
    }
    qsort(arr1, n, sizeof(int), cmp);
    qsort(arr2, m, sizeof(int), cmp);

    double time = MPI_Wtime();

    sub_arr1 = malloc(n_per_proc * sizeof(int)); // Memory allocation for the sub array

    //Scatter the array to all processes
    MPI_Scatter(arr1, n_per_proc, MPI_INT, sub_arr1, n_per_proc, MPI_INT, 0, MPI_COMM_WORLD);

    //The biggest element of the first array is the last element of the sub array
    int pivot1 = sub_arr1[n_per_proc - 1];

    //Find the index of starting point and the end point of the second array
    int start = 0;
    int end = m - 1;
    int mid = (start + end) / 2;
    while (start <= end) {
        if (arr2[mid] < pivot1) {
            start = mid + 1;
        } else if (arr2[mid] > pivot1) {
            end = mid - 1;
        } else {
            break;
        }
        mid = (start + end) / 2;
    }

    //Merge the first array and the second array from the starting point to the end point to the merged array
    int i = 0;
    int j = 0;
    int k = 0;
    while (i < n_per_proc && j < end - start + 1) {
        if (sub_arr1[i] < arr2[j]) {
            merged[k] = sub_arr1[i];
            i++;
        } else {
            merged[k] = arr2[j];
            j++;
        }
        k++;
    }
    while (i < n_per_proc) {
        merged[k] = sub_arr1[i];
        i++;
        k++;
    }
    while (j < end - start + 1) {
        merged[k] = arr2[j];
        j++;
        k++;
    }

    //Send the merged array to the root process
    MPI_Gather(merged, n_per_proc, MPI_INT, arr1, n_per_proc, MPI_INT, 0, MPI_COMM_WORLD);

    //The root process merges the first array and the second array from the end point to the starting point to the merged array
    if (rank == 0) {
        i = n_per_proc - 1;
        j = end;
        k = n_per_proc - 1;
        while (i >= 0 && j >= start) {
            if (arr1[i] > arr2[j]) {
                merged[k] = arr1[i];
                i--;
            } else {
                merged[k] = arr2[j];
                j--;
            }
            k--;
        }
        while (i >= 0) {
            merged[k] = arr1[i];
            i--;
            k--;
        }
        while (j >= start) {
            merged[k] = arr2[j];
            j--;
            k--;
        }
        //Print the time that is taken to run the program
        time = MPI_Wtime() - time;
        printf("Time taken: %f", time);

        //Print the merged array
        for (int i = 0; i < n + m; i++) {
            printf("%d ", merged[i]);
        }
    }
    //Free the memory
    free(arr1);
    free(arr2);
    free(merged);
    free(sub_arr1);
    free(sub_arr2);
    MPI_Finalize();

    return 0;

}
