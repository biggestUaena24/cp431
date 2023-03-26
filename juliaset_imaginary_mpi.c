#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <string.h>
#include <png.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "mpi.h"

#define WIDTH 5120
#define HEIGHT 5120
#define MAX_ITER 1000
#define OUT_DIR "imaginary_interval"

int mandelbrot_iterations(double complex c, double complex z) {
    int n = 0;
    while (cabs(z) <= 2 && n < MAX_ITER) {
        z = z * z + c;
        n++;
    }
    return n;
}

void create_image(int global_index, double complex c) {
    char filename[256];
    sprintf(filename, "%s/mandelbrot_frame_%06d.png", OUT_DIR, global_index, cimag(c));
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        printf("Error: Unable to open file %s.\n", filename);
        exit(1);
    }

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        printf("Error: Unable to create PNG write struct.\n");
        exit(1);
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, NULL);
        printf("Error: Unable to create PNG info struct.\n");
        exit(1);
    }

    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, WIDTH, HEIGHT, 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png_ptr, info_ptr);

    double complex z;
    int i, j, iter;
    png_bytep row = (png_bytep)malloc(3 * WIDTH * sizeof(png_byte));
    for (j = 0; j < HEIGHT; j++) {
        for (i = 0; i < WIDTH; i++) {
            double x = 3.0 * (i - WIDTH / 2) / (WIDTH / 2);
            double y = 3.0 * (j - HEIGHT / 2) / (HEIGHT / 2);
            z = x + y * I;
            iter = mandelbrot_iterations(c, z);
            row[i] = (iter * 32) % 256;
        }
        png_write_row(png_ptr, row);
    }

    png_write_end(png_ptr, NULL);
    fclose(fp);
    png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
    png_destroy_write_struct(&png_ptr, NULL);
    free(row);
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        struct stat st = {0};
        if (stat(OUT_DIR, &st) == -1) {
            mkdir(OUT_DIR, 0700);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    double start = -2.0;
    double end = 2.0;
    double step = 0.01;
    int total_images = (end - start) / step;

    int images_per_process = total_images / size;
    int remaining_images = total_images % size;

    double imaginary_part = start + rank * images_per_process * step;
    int images_to_generate = (rank < remaining_images) ? images_per_process + 1 : images_per_process;

    int global_offset = rank * images_per_process + (rank < remaining_images ? rank : remaining_images);

    for (int i=0;i<images_to_generate;i++) {
        double complex c = (imaginary_part + i * step) * I;
        int global_index = global_offset + i;
        create_image(global_index, c);
    }

    MPI_Finalize();

    return 0;
}
