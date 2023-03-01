#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

int cmp(const void *a, const void *b) {
    return (*(int *)a - *(int *)b);
}

int* generate_sorted_array(int size) {
    int *arr = malloc(sizeof(int) * size);
    for (int i=0;i<size;i++) {
        arr[i] = rand() % 1000;
    }

    qsort(arr, size, sizeof(int), cmp);

    return arr;
}

int* merge(int* arr1, int size_a, int* arr2, int size_b) {

    int *merged = malloc(sizeof(int) * (size_a + size_b));
    int i = 0, j = 0, k = 0;
    while (i<size_a && j <size_b)
    {
        if (arr1[i] < arr2[j])
            merged[k++] = arr1[i++];
        else
            merged[k++] = arr2[j++];
    }
  
    // Store remaining elements of first array
    while (i < size_a)
        merged[k++] = arr1[i++];
  
    // Store remaining elements of second array
    while (j < size_b)
        merged[k++] = arr2[j++];

    return merged;
}

int binary_search(int target, int *arr, int n, int start) {
    int l = start, r = n - 1, m;
    while (l<=r) {
        m = l + (r - l) / 2;
        if (target > arr[m]) l = m + 1;
        else if (target < arr[m]) r = m - 1;
        else return m;
    }

    return l;
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    const long long ARRAY_SIZE = 500000000;
    MPI_Status status;
    double start_array_generation_time, end_array_generation_time;
    double start_all_merge, end_all_merge;
    double start_global = MPI_Wtime(), end_global;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // size - 1 because rank 0 is not really merging the arrays
    int el_per_process = ARRAY_SIZE / (size - 1);
    int *A, *B, *C, **partition_B, **partition_A;

    if (rank == 0) {
        start_array_generation_time = MPI_Wtime();
        A = generate_sorted_array(ARRAY_SIZE);
        B = generate_sorted_array(ARRAY_SIZE);
        end_array_generation_time = MPI_Wtime();

        partition_A = malloc(sizeof(int *) * (size));
        long long k = 0;
        for (int i=0;i<size;i++) {
            partition_A[i] = malloc(sizeof(int) * el_per_process); 
            for (long long j=0;j<el_per_process;j++) {
                partition_A[i][j] = A[k++];
            }
            MPI_Send(partition_A[i], el_per_process, MPI_INT, i, 0, MPI_COMM_WORLD);
        }

        // find break points for B
        partition_B = malloc(sizeof(int *) * (size));
        // linear search
        int i = 0, target, index_b, total_size;
        k = 0;
        while (i < size - 1) {
            target = partition_A[i][el_per_process - 1];
            index_b = binary_search(target, B, ARRAY_SIZE, k);
            total_size = index_b - k;
            partition_B[i] = malloc(sizeof(int) * (total_size + 1));
            
            for (int j=0;j<total_size;j++) {
                partition_B[i][j] = B[k++];    
            }

            partition_B[i][total_size] = -1;

            MPI_Send(&total_size, sizeof(int), MPI_INT, i, 1, MPI_COMM_WORLD);
            MPI_Send(partition_B[i], total_size + 1, MPI_INT, i, 2, MPI_COMM_WORLD);

            i++;
        }

        total_size = ARRAY_SIZE - k;
        partition_B[size - 1] = malloc(sizeof(int) * total_size + 1);
        for (int j=0;j<total_size;j++) {
            partition_B[size - 1][j] = B[k++];
        }
        partition_B[size - 1][total_size] = -1;
        MPI_Send(&total_size, sizeof(int), MPI_INT, size - 1, 1, MPI_COMM_WORLD);
        MPI_Send(partition_B[size - 1], total_size + 1, MPI_INT, size - 1, 2, MPI_COMM_WORLD);
    } 

    int *recv_buff_a = malloc(sizeof(int) * el_per_process), *recv_buff_b, b_size;
    MPI_Recv(recv_buff_a, el_per_process, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
    MPI_Recv(&b_size, sizeof(int), MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
    recv_buff_b = malloc(sizeof(int) * (b_size + 1));
    MPI_Recv(recv_buff_b, b_size + 1, MPI_INT, 0, 2, MPI_COMM_WORLD, &status);
    
    double start_merge_time = MPI_Wtime();
    int *merged = merge(recv_buff_a, el_per_process, recv_buff_b, b_size);
    double end_merge_time = MPI_Wtime();
    printf("Rank %d merge time: %f\n", rank, end_merge_time - start_merge_time);

    int merged_size = el_per_process + b_size;
    MPI_Send(&merged_size, sizeof(int), MPI_INT, 0, 4, MPI_COMM_WORLD);
    MPI_Send(merged, el_per_process + b_size, MPI_INT, 0, 5, MPI_COMM_WORLD);

    if (rank == 0) {
        C = malloc(sizeof(int) * ARRAY_SIZE * 2);
        int *recv_buff_c, c_size, index = 0;
        start_all_merge = MPI_Wtime();
        for (int i=0;i<size;i++) {
            MPI_Recv(&c_size, sizeof(int), MPI_INT, i, 4, MPI_COMM_WORLD, &status);
            recv_buff_c = malloc(sizeof(int) * c_size);
            MPI_Recv(recv_buff_c, c_size, MPI_INT, i, 5, MPI_COMM_WORLD, &status);
            for (int j=0;j<c_size;j++) {
                C[index++] = recv_buff_c[j];
            }
            free(recv_buff_c);
        }
        end_all_merge = MPI_Wtime();

        printf("Final merged:\n");
        int max_size = ARRAY_SIZE * 2 > 50 ? 50 : ARRAY_SIZE * 2; 
        for (int i=0;i<max_size;i++) {
            printf("%d ", C[i]);
        }
        printf("\n");
    }

    free(recv_buff_a);
    free(recv_buff_b);

    if (rank == 0) {
        free(A);
        free(B);
        free(C);
        free(partition_A);
        free(partition_B);
    }

    end_global = MPI_Wtime();

    if (rank == 0) {
        printf("Total runtime: %f\n", end_global - start_global);
        printf("Time taken to generate arrays: %f\n", end_array_generation_time - start_array_generation_time);
        printf("Total time to merge all partitions: %f\n", end_all_merge - start_all_merge);
    }

    MPI_Finalize();

    return 0;
}
