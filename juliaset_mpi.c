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

void map_colour(Pixel *pixel, int iter, int max_iter, int red_mod, int green_mod, int blue_mod, int mode) {
    if (mode == 0) {
        // black and white
        if (iter < max_iter) {
            // bounded points colour them white
            pixel->red = 255;
            pixel->green = 255;
            pixel->blue = 255;
        } else {
            // points that escape to infinity colour them black
            pixel->red = 0;
            pixel->green = 0;
            pixel->blue = 0;
        }
    } else if (mode == 1) {
        // gray scale
        pixel->red = iter % 256;
        pixel->green = iter % 256;
        pixel->blue = iter % 256;
    } else {
        // colour full
        if (iter < max_iter) {
            // bounded points colour them black
            pixel->red = (iter * red_mod) % 256;
            pixel->green = (iter * green_mod) % 256;
            pixel->blue = (iter * blue_mod) % 256;
        } else {
            // points that escape to infinity colour them black
            pixel->red = 0;
            pixel->green = 0;
            pixel->blue = 0;
        }
    }
}

void generate_julia_set_section(Pixel *image, int start_row, int end_row, double complex c, int max_iter, int width, int height, int red_mod, int green_mod, int blue_mod, int mode, double zoom_factor) {
    for (int y=start_row;y<end_row;y++) {
        for (int x=0;x<width;x++) {
            double complex z = ((x - width / 2.0) / width * zoom_factor) + ((y - height / 2.0) / height * zoom_factor) * I;
            int iter = 0;

            while (cabs(z) < 2.0 && iter < max_iter) {
                z = z * z + c;
                iter++;
            }

            Pixel *pixel = &image[y * width + x];
            map_colour(pixel, iter, max_iter, red_mod, green_mod, blue_mod, mode);
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
    for (int y = height - 1; y >= 0; y--) {
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

    if (argc < 12) {
        printf("Usage: ./gen_juliaset [filename] [width] [height] [c1] [c2] [iter] [red] [blue] [green] [mode] [zoom_factor]\n");
        MPI_Finalize();
        return 0;
    }

    char filename[100];
    sprintf(filename, "%s.png", argv[1]);

    char log_filename[100];
    sprintf(log_filename, "%s.txt", argv[1]);

    // image dimension
    int width = atoi(argv[2]);
    int height = atoi(argv[3]);

    // c constant, real + imag
    char *endptr;
    double c1 = strtod(argv[4], &endptr);
    double c2 = strtod(argv[5], &endptr);
    double complex c = c1 + c2 * I;
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int max_iter = atoi(argv[6]);

    // colour modifiers
    int red_mod = atoi(argv[7]);
    int green_mod = atoi(argv[8]);
    int blue_mod = atoi(argv[9]);
    int colour_mode = atoi(argv[10]);

    // zoom factor, bigger values => more zoom out
    double zoom_factor = strtod(argv[11], &endptr);
    
    int rows_per_process = height / size;
    int start_row = rank * rows_per_process;
    int end_row = (rank + 1) * rows_per_process;

    // for benchmark
    double start_time, end_time, total_time;

    Pixel *image = malloc(width * height * sizeof(Pixel));

    // Custom MPI Data Type for Pixel Struct
    MPI_Datatype MPI_PIXEL;
    MPI_Datatype types[3] = { MPI_UNSIGNED_CHAR, MPI_UNSIGNED_CHAR, MPI_UNSIGNED_CHAR };
    int block_lengths[3] = { 1, 1, 1 };
    MPI_Aint offsets[3];

    offsets[0] = offsetof(Pixel, red);
    offsets[1] = offsetof(Pixel, green);
    offsets[2] = offsetof(Pixel, blue);

    MPI_Type_create_struct(3, block_lengths, offsets, types, &MPI_PIXEL);
    MPI_Type_commit(&MPI_PIXEL);

    // start the timer
    start_time = MPI_Wtime();

    generate_julia_set_section(image, start_row, end_row, c, max_iter, width, height, red_mod, green_mod, blue_mod, colour_mode, zoom_factor);

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
        
        write_png_file(filename, width, height, image);

        FILE *log_fp = fopen(log_filename, "w");

        if (log_fp == NULL) {
            printf("Could not open log file.\n");
        }

        double sum = 0.0;
        for (int i=0;i<size;i++) {
            if (log_fp != NULL) {
                fprintf(log_fp, "%05lf\n", processing_time_per_rank[i]);
            }
            printf("rank %d processing time: %f\n", i, processing_time_per_rank[i]);
            sum += processing_time_per_rank[i];
        }

        double avg = sum / size;
        printf("average processing time: %f\n", avg);

        if (log_fp != NULL) {
            fprintf(log_fp, "%05lf\n", avg);
            fclose(log_fp);
        }

        free(processing_time_per_rank);  
    } else {
        MPI_Send(&image[start_row * width], (end_row - start_row) * width, MPI_PIXEL, 0, 0, MPI_COMM_WORLD);
        MPI_Send(&total_time, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
    }

    free(image);
    MPI_Finalize();

    return 0;
}
