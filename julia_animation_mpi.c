#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include <math.h>
#include <complex.h>
#include "mpi.h"

#define MAX_ITER 1000

int mandelbrot_iterations(double complex c, double complex z, int max_iter) {
    int n = 0;
    while (cabs(z) <= 2 && n < max_iter) {
        z = z * z + c;
        n++;
    }
    return n;
}

void create_image(char *filename, double complex c, int width, int height) {
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
    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png_ptr, info_ptr);

    double complex z;
    int i, j, iter;
    png_bytep row = (png_bytep)malloc(3 * width * sizeof(png_byte));
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            double x = 3.0 * (i - width / 2) / (width / 2);
            double y = 3.0 * (j - height / 2) / (height / 2);
            z = x + y * I;
            iter = mandelbrot_iterations(c, z, MAX_ITER);

            row[i * 3] = (iter * 32) % 256; // R
//            row[i * 3 + 1] = (iter * 16) % 256; // G
            row[i * 3 + 1] = 0;
            row[i * 3 + 2] = (iter * 16) % 256;  // B
        }
        png_write_row(png_ptr, row);
    }

    png_write_end(png_ptr, NULL);
    fclose(fp);
    png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
    png_destroy_write_struct(&png_ptr, NULL);
    free(row);
}

void create_images(int start_frame, int end_frame, int num_frames, double radius, int width, int height) {
    for (int frame = start_frame; frame < end_frame; frame++) {
        double angle = 2.0 * M_PI * frame / num_frames;
        double real = radius * cos(angle);
        double imag = radius * sin(angle);
        double complex c = real + imag * I;

        char filename[64];
        snprintf(filename, sizeof(filename), "animation/frame_%04d.png", frame);
        
        // Call create_image function here
        create_image(filename, c, width, height);
        printf("Image frame created: %s\n", filename);
    }
}

int main(int argc, char **argv) {

    if (argc < 5) {
        printf("Did not provide radius and frame\n");
        exit(1);
    }

    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    
    double radius = strtod(argv[1], NULL);
    int num_frames = atoi(argv[2]);
    int width = atoi(argv[3]);
    int height = atoi(argv[4]);

    int frames_per_thread = num_frames / size;
    int start_frame = rank * frames_per_thread;
    int end_frame = (rank == size - 1) ? num_frames : (rank + 1) * frames_per_thread;

    create_images(start_frame, end_frame, num_frames, radius, width, height);

    MPI_Finalize();

    return 0;
}