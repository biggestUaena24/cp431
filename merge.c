#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define ARRAY_SIZE 16 

int compare(const void *a, const void *b) {
    return (*(int *)a - *(int *)b);
}

int* generate_sorted_array(int size) {
   int *arr = (int *) malloc(size * sizeof(int));
    for (int i=0;i<size;i++) {
        arr[i] = rand() % 1000;
    }

    qsort(arr, size, sizeof(int), compare);

    return arr;
}

int binary_search(int target, int *B) {
    // find the index in array B
    int l = 0;
    int r = ARRAY_SIZE - 1;
    int m;
    while (l<=r) {
        m = l + (r - l) / 2;
        if (B[m] < target) {
            l = m + 1;
        } else if (B[m] > target) {
            r = m - 1;
        } else {
            return m;
        }
    }

    return l;
}

int main(int argc, char **argv) {


    srand(time(NULL));
    int n = ARRAY_SIZE;
    int *A = generate_sorted_array(n);
    int *B = generate_sorted_array(n);
    int *C = (int *) malloc(n * 2 * sizeof(int));

    for (int i=0;i<n;i++) {
        printf("%d ", A[i]);
    }
    printf("\n");

    for (int i=0;i<n;i++) {
        printf("%d ", B[i]);
    }
    printf("\n");


    // partitioning
    int el_per_process = 4;
    
    double start_time = MPI_Wtime();

    int *local_parition = (int *) malloc(el_per_process * sizeof(int));

    int index = binary_search(A[el_per_process - 1], B);
    printf("%d\n", index);

     

    // free all dynamically allocated memory
    free(local_parition);
    free(A);
    free(B);

    return 0;
}
