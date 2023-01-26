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
    int rank, num_procs;
    int prev_prime = 0, curr_prime = 0;
    int max_gap = 0, curr_gap = 0;
    int first_prime = 0, second_prime = 0;
    MPI_Status status;
    MPI_Init(&argc, &argv);
    MPI_Barrier(MPI_COMM_WORLD);
    double time = MPI_Wtime();
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    int local_start = rank * range / num_procs + 1;
    int local_end = (rank+1) * range / num_procs;
    for (int i = local_start; i <= local_end; i++) {
        if (is_prime(i)) {
            curr_prime = i;
            if (prev_prime != 0) {
                curr_gap = curr_prime - prev_prime;
                if (curr_gap > max_gap) {
                    max_gap = curr_gap;
                    first_prime = prev_prime;
                    second_prime = curr_prime;
                }
            }
            prev_prime = i;
        }
    }
    if(rank != 0){
        int data[3] = {max_gap, first_prime, second_prime};
        MPI_Send(data, 3, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    if(rank == 0){
        int global_max_gap = 0;
        int global_first_prime = 0;
        int global_second_prime = 0;
        for(int i = 1; i < num_procs; i++){
            int data[3];
            MPI_Recv(data, 3, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
            if(data[0] > global_max_gap)
            {
                global_max_gap = data[0];
                global_first_prime = data[1];
                global_second_prime = data[2];
            }
        }
        time += MPI_Wtime();
        printf("The largest gap between prime numbers is: %d\n", global_max_gap);
        printf("The prime number pairs with the largest gap are: %d, %d\n", global_first_prime, global_second_prime);
        printf("The time it has been running is: %f", time);
    }
    MPI_Finalize();
    return 0;
}
