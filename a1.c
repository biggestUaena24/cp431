#include <stdio.h>
#include "mpi.h"


int is_prime(int n) {
    if (n <= 1) return 0;
    if (n <= 3) return 1;
    if (n % 2 == 0 || n % 3 == 0) return 0;
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0)
            return 0;
    }
    return 1;
}

int main(int argc, char** argv) {
    unsigned long int range = 1000000000;
    int rank, p;
    int prev_prime = 0, curr_prime = 0;
    int max_gap = 0, curr_gap = 0;
    int first_prime = 0, second_prime = 0;
    int data[3];
    MPI_Status status;
    MPI_Init(&argc, &argv);
    MPI_Barrier(MPI_COMM_WORLD);
    double time = MPI_Wtime();
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    int local_start = rank * range / p + 1;
    int local_end = (rank+1) * range / p;
    for (int i = local_start; i <= local_end; i++) {
        if (is_prime(i)) {
            curr_prime = i;
            if (prev_prime != 0) {
                curr_gap = curr_prime - prev_prime;
                if (curr_gap > max_gap) {
                    max_gap = curr_gap;
                    data[0] = max_gap;
                    data[1] = prev_prime;
                    data[2] = curr_prime;
                }
            }
            prev_prime = i;
        }
    }
    if(rank != 0){
        printf("%d processor sending prime pairs %d %d with gap %d\n", rank, data[1], data[2], data[0]);
        MPI_Send(data, 3, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    if(rank == 0){
        int global_max_gap = max_gap;
        int global_first_prime = prev_prime;
        int global_second_prime = curr_prime;
        for(int i = 1; i < p; i++){
            int recv[3];
            MPI_Recv(recv, 3, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
            if(recv[0] > global_max_gap)
            {
                global_max_gap = recv[0];
                global_first_prime = recv[1];
                global_second_prime = recv[2];
            }
        }
        time += MPI_Wtime();
        printf("The largest gap between prime numbers is: %d\n", global_max_gap);
        printf("The prime number pairs with the largest gap are: %d, %d\n", global_first_prime, global_second_prime);
        printf("The time it has been running is: %f\n", time);
    }
    MPI_Finalize();
    return 0;
}
