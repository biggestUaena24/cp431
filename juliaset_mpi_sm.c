#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <png.h>
#include "mpi.h"

typedef struct {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} Pixel;

void generate_julia_set_section(Pixel *image, int start_row, int end_row, double complex c, int max_iter, int width, int height, int red_mod, int blue_mod, int green_mod) {
    for (int y=start_row;y<end_row;y++) {
        for (int x=0;x<width;x++) {
            double complex z = ((x - width / 2.0) / width * 4.0) + ((y - height / 2.0) / height * 4.0) * I;
            int iter = 0;

            while (cabs(z) < 2.0 && iter < max_iter) {
                z = z * z + c;
                iter++;
            }

            Pixel *pixel = &image[y * width + x];
            pixel->red = (iter * red_mod) % 256;
            pixel->green = (iter * green_mod) % 256;
            pixel->blue = (iter * blue_mod) % 256;
        }
    }
}

void write_png_file(const char *filename, int width, int height, Pixel *image) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Could not open file %s for writing\n", filename);
        return;
    }

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fprintf(stderr, "Could not allocate write struct\n");
        fclose(fp);
        return;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        fprintf(stderr, "Could not allocate info struct\n");
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        fclose(fp);
        return;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        fprintf(stderr, "Error during png creation\n");
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        return;
    }

    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, width, height,
                 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png_ptr, info_ptr);

    png_bytep row = (png_bytep)malloc(3 * width * sizeof(png_byte));
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            row[x * 3] = image[y * width + x].red;
            row[x * 3 + 1] = image[y * width + x].green;
            row[x * 3 + 2] = image[y * width + x].blue;
        }
        png_write_row(png_ptr, row);
    }

    png_write_end(png_ptr, NULL);
    fclose(fp);

    if (png_ptr && info_ptr)
        png_destroy_write_struct(&png_ptr, &info_ptr);
    if (row)
        free(row);
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    if (argc < 9) {
        printf("Usage: ./gen_juliaset [width] [height] [c1] [c2] [iter] [red] [blue] [green]\n");
        MPI_Finalize();
        return 0;
    }

    // image dimension
    int width = atoi(argv[1]);
    int height = atoi(argv[2]);

    // c constant, real + imag
    char *endptr;
    double c1 = strtod(argv[3], &endptr);
    double c2 = strtod(argv[4], &endptr);
    double complex c = c1 + c2 * I;
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int max_iter = atoi(argv[5]);

    // colour modifiers
    int red_mod = atoi(argv[6]);
    int blue_mod = atoi(argv[7]);
    int green_mod = atoi(argv[8]);
    
    int rows_per_process = height / size;
    int start_row = rank * rows_per_process;
    int end_row = (rank + 1) * rows_per_process;

    // for benchmark
    double start_time, end_time, total_time;

    MPI_Win window;
    Pixel *image; 
    MPI_Win_allocation(sizeof(Pixel) * width * height, sizeof(Pixel), MPI_INFO_NULL, MPI_COMM_WORLD, &image, &window);

    MPI_Win processing_time_window;
    double *processing_time_per_rank;
    MPI_Win_allocation(sizeof(double) * size, sizeof(double), MPI_INFO_NULL, MPI_COMM_WORLD, &processing_time_per_rank, &processing_time_window)

    // start the timer
    start_time = MPI_Wtime();

    generate_julia_set_section(image, start_row, end_row, c, max_iter, width, height, red_mod, blue_mod, green_mod);

    // end the timer
    end_time = MPI_Wtime();

    total_time = end_time - start_time;

    processing_time_per_rank[rank] = total_time;

    if (rank == 0) {
        printf("waiting for image data...\n");
        char filename[100];
        sprintf(filename, "juliaset_%.6lf_%.6lf.png", c1, c2);
        write_png_file(filename, width, height, image);
        printf("finished writing image...\n");
        double sum = 0.0;
        for (int i=0;i<size;i++) {
            printf("rank %d processing time: %f\n", i, processing_time_per_rank[i]);
            sum += processing_time_per_rank[i];
        }

        double avg = sum / size;
        printf("average processing time: %f\n", avg);
    } 

    MPI_Win_free(&processing_time_window);
    MPI_Win_free(&window);
    MPI_Finalize();

    return 0;
}
