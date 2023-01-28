#include <stdio.h>
#include "mpi.h"


// Implementaion of our algorithm of checking a prime number
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
    // Declare all the variables that we are going to use
    unsigned long int range = 1000000000; // The range of our prime numbers (1 to 1000000000)
    int rank, p; // The rank of current processor and number of total processors
    int prev_prime = 0, curr_prime = 0; // Variable used to record previous prime number and current prime number in later section
    int max_gap = 0, curr_gap = 0; // previous recoreded max gap and current gap of current prim pairs
    int first_prime = 0, second_prime = 0; // prime number pairs recorded
    int data[3]; // An array that we are going to store our recorded value and value we are sending to master processor
    MPI_Status status; // Status of MPI
    MPI_Init(&argc, &argv); // Initiating the MPI program
    MPI_Barrier(MPI_COMM_WORLD); // Using the method we are taught in class so we can properly record time
    double time = MPI_Wtime(); // Starting to record run time of the process
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Getting the rank of local processor
    MPI_Comm_size(MPI_COMM_WORLD, &p); // Getting how many processors are in use
    int local_start = rank * range / p + 1; // Calculating the local start number for checking prime number
    int local_end = (rank+1) * range / p; // Calculating the local end number for checking prime number
    // Algorithm of checking prime number pairs, max gap
    for (int i = local_start; i <= local_end; i++) {
        if (is_prime(i)) { // If the number we are currently checking is prime nunber
            curr_prime = i; // Record the number we are checking now
            if (prev_prime != 0) { // If it is not the first prime number we find in this processor
                curr_gap = curr_prime - prev_prime; // Calculate the gap between last prime number and this prime number
                if (curr_gap > max_gap) { // If the gap is larger
                    // Update the variables that we are going to send to master processor
                    max_gap = curr_gap;
                    data[0] = max_gap;
                    data[1] = prev_prime;
                    data[2] = curr_prime;
                    first_prime = prev_prime;
                    second_prime = curr_prime;
                }
            }
            // Set previous prime number to current prime number
            prev_prime = i;
        }
    }
    if(rank != 0){ // If it is not the master processor
        // Send the result of calculation to master processor
        MPI_Send(data, 3, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    if(rank == 0){// If it is the master processor
        // Variables to record the global gaps and prime number pairs
        int global_max_gap = max_gap;
        int global_first_prime = first_prime;
        int global_second_prime = second_prime;
        // A for loop to check the result that are sent from different processors
        for(int i = 1; i < p; i++){
            int recv[3]; // The data we are receiving
            MPI_Recv(recv, 3, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
            if(recv[0] > global_max_gap){ // If the data we are receving are bigger than the previous ones
                // Update the result
                global_max_gap = recv[0];
                global_first_prime = recv[1];
                global_second_prime = recv[2];
            }
        }
        // The time that it finished the calculation
        time = MPI_Wtime() - time;
        // Print out the result we got in our calculation
        printf("The largest gap between prime numbers is: %d\n", global_max_gap);
        printf("The prime number pairs with the largest gap are: %d, %d\n", global_first_prime, global_second_prime);
        printf("The time it has been running is: %f\n", time);
    }
    // Closing the MPI program
    MPI_Finalize();
    return 0;
}
