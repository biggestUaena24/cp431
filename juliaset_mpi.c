#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <string.h>
#include "mpi.h"

#define MAX_ITER 10000
#define RED_MOD 1 
#define GREEN_MOD 1
#define BLUE_MOD 1

typedef struct {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} Pixel;

void generate_julia_set_section(Pixel *image, int start_row, int end_row, double complex c, int rank, int width, int height) {
    for (int y=start_row;y<end_row;y++) {
        for (int x=0;x<width;x++) {
            double complex z = (x - width / 2.0) * 4.0 / width + (y - height / 2.0) * 4.0 / height * I;
            int iter = 0;

            while (abs(z) < 2.0 && iter < MAX_ITER) {
                z = z * z + c;
                iter++;
            }

            Pixel *pixel = &image[y * width + x];
            pixel->red = (iter * RED_MOD) % 256;
            pixel->green = (iter * GREEN_MOD) % 256;
            pixel->blue = (iter * BLUE_MOD) % 256;
        }
    }
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    if (argc < 3) {
        printf("Usage: ./gen_juliaset [width] [height]\n");
        return 0;
    }

    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    int rank, size;
    double start_time, end_time, total_time;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    Pixel *image = malloc(width * height * sizeof(Pixel));

    MPI_Datatype MPI_PIXEL;
    MPI_Datatype types[3] = { MPI_UNSIGNED_CHAR, MPI_UNSIGNED_CHAR, MPI_UNSIGNED_CHAR };
    int block_lengths[3] = { 1, 1, 1 };
    MPI_Aint offsets[3];

    offsets[0] = offsetof(Pixel, red);
    offsets[1] = offsetof(Pixel, green);
    offsets[2] = offsetof(Pixel, blue);

    MPI_Type_create_struct(3, block_lengths, offsets, types, &MPI_PIXEL);
    MPI_Type_commit(&MPI_PIXEL);
    char *endptr;
    double c1 = strtod(argv[1], &endptr);
    double c2 = strtod(argv[2], &endptr);
    double complex c = c1 + c2 * I;
    int rows_per_process = height / size;
    int start_row = rank * rows_per_process;
    int end_row = (rank + 1) * rows_per_process;

    // start the timer
    start_time = MPI_Wtime();

    generate_julia_set_section(image, start_row, end_row, c, rank, width, height);

    // end the timer
    end_time = MPI_Wtime();

    total_time = end_time - start_time;

    if (rank == 0) {
        printf("waiting for image data...\n");
        double *processing_time_per_rank = malloc(sizeof(double) * size);
        processing_time_per_rank[0] = total_time;
        for (int i=1;i<size;i++) {
            Pixel *temp = malloc(width * rows_per_process * sizeof(Pixel));
            start_row = i * rows_per_process;
            end_row = (i + 1) * rows_per_process;
            MPI_Recv(temp, rows_per_process * width, MPI_PIXEL, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&processing_time_per_rank[i], 1, MPI_DOUBLE, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            memcpy(&image[start_row * width], temp, rows_per_process * width * sizeof(Pixel));
            free(temp);
        }

        char filename[100];
        sprintf(filename, "output/juliaset_%lf_%lf.ppm", c1, c2);
        FILE *fp = fopen(filename, "wb");
        fprintf(fp, "P6\n%d %d\n255\n", width, height);
        fwrite(image, sizeof(Pixel), width * height, fp);
        fclose(fp);

        printf("finished writing image...\n");
        double sum = 0.0;
        for (int i=0;i<size;i++) {
            printf("rank %d processing time: %f\n", i, processing_time_per_rank[i]);
            sum += processing_time_per_rank[i];
        }

        double avg = sum / size;
        printf("average processing time: %f\n", avg);
        free(processing_time_per_rank);  
    } 
    else {
        MPI_Send(&image[start_row * width], (end_row - start_row) * width, MPI_PIXEL, 0, 0, MPI_COMM_WORLD);
        MPI_Send(&total_time, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
    }

    free(image);
    MPI_Finalize();

    return 0;
}
