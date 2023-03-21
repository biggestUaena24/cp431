#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <string.h>
#include "mpi.h"

#define WIDTH 4096 
#define HEIGHT 4096 
#define MAX_ITER 10000 
#define RED_MOD 2 
#define GREEN_MOD 1 
#define BLUE_MOD 1

typedef struct {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} Pixel;

int min(int a, int b) {
    return (a < b) ? a : b;
}

void generate_julia_set_section(Pixel *image, int start_row, int end_row, double complex c) {
    for (int y=start_row;y<end_row;y++) {
        for (int x=0;x<WIDTH;x++) {
            double complex z = (x - WIDTH / 2.0) * 4.0 / WIDTH + (y - HEIGHT / 2.0) * 4.0 / HEIGHT * I;
            int iter = 0;

            while (abs(z) < 2.0 && iter < MAX_ITER) {
                z = z * z + c;
                iter++;
            }

            Pixel *pixel = &image[y * WIDTH + x];
            pixel->red = (iter * RED_MOD) % 256;
            pixel->green = (iter * GREEN_MOD) % 256;
            pixel->blue = (iter * BLUE_MOD) % 256;
        }
    }
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_Datatype MPI_PIXEL;
    MPI_Datatype types[3] = { MPI_UNSIGNED_CHAR, MPI_UNSIGNED_CHAR, MPI_UNSIGNED_CHAR };
    int block_lengths[3] = { 1, 1, 1 };
    MPI_Aint offsets[3];

    offsets[0] = offsetof(Pixel, red);
    offsets[1] = offsetof(Pixel, green);
    offsets[2] = offsetof(Pixel, blue);

    MPI_Type_create_struct(3, block_lengths, offsets, types, &MPI_PIXEL);
    MPI_Type_commit(&MPI_PIXEL);

    // MPI_Win win;
    // MPI_Aint image_size = WIDTH * HEIGHT * sizeof(Pixel);
    Pixel *image = malloc(WIDTH * HEIGHT * sizeof(Pixel));

//    MPI_Win_allocate_shared(image_size, sizeof(Pixel), MPI_INFO_NULL, MPI_COMM_WORLD, &image, &win);

    double complex c = -0.8 + 0.156 * I;
    int rows_per_process = HEIGHT / size;
    int remainder_rows = HEIGHT % size;
    int start_row = rank * rows_per_process + min(rank, remainder_rows);
    int end_row = start_row + rows_per_process + (rank < remainder_rows ? 1 : 0);

    generate_julia_set_section(image, start_row, end_row, c);

    // MPI_Barrier(MPI_COMM_WORLD);

    printf("rank %d finished\n", rank);

    if (rank == 0) {
        printf("waiting for image data...\n");
        Pixel *temp = malloc(WIDTH * rows_per_process * sizeof(Pixel));

        for (int i=1;i<size;i++) {
            int local_start_row = i * rows_per_process + min(i, remainder_rows);
            int local_end_row = local_start_row + rows_per_process + (i < remainder_rows ? 1 : 0);
            MPI_Recv(temp, (local_end_row - local_start_row) * WIDTH, MPI_PIXEL, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            memcpy(&image[local_start_row * WIDTH], temp, (local_end_row - local_start_row) * WIDTH * sizeof(Pixel));
        }

        free(temp);

        FILE *fp = fopen("julia_set_mpi_shared.ppm", "wb");
        fprintf(fp, "P6\n%d %d\n255\n", WIDTH, HEIGHT);
        fwrite(image, sizeof(Pixel), WIDTH * HEIGHT, fp);
        fclose(fp);

        printf("finished writing image...\n");
    } else {
        MPI_Send(&image[start_row * WIDTH], (end_row - start_row) * WIDTH, MPI_PIXEL, 0, 0, MPI_COMM_WORLD);
    }

    // MPI_Win_free(&win);
    free(image);
    MPI_Finalize();

    return 0;
}
