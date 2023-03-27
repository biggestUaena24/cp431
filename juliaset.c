#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <png.h>
#include <time.h>

void write_png_file(char* filename, int* buffer, int width, int height) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Error: Could not open file %s for writing\n", filename);
        return;
    }

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fprintf(stderr, "Error: Could not create PNG write struct\n");
        fclose(fp);
        return;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        fprintf(stderr, "Error: Could not create PNG info struct\n");
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        fclose(fp);
        return;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        fprintf(stderr, "Error: Error during PNG file write\n");
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        return;
    }

    png_init_io(png_ptr, fp);

    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(png_ptr, info_ptr);

    png_bytep row_pointer[height];
    for (int y = 0; y < height; y++) {
        row_pointer[y] = (png_bytep)(buffer + y * width);
    }
    png_write_image(png_ptr, row_pointer);

    png_write_end(png_ptr, NULL);

    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: ./juliaset [width] [height]\n");
        exit(1);
    }

    int width = atoi(argv[1]);
    int height = atoi(argv[2]);


    clock_t start_time, end_time;
    double total_time;
    double cx = -0.8;
    double cy = 0.156;
    double xmin = -1.5;
    double xmax = 1.5;
    double ymin = -1.5;
    double ymax = 1.5;
    int maxiter = 1000;

    int *image = malloc(sizeof(int) * width * height);
    printf("here\n");

    start_time = clock();

    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            double x = xmin + i * (xmax - xmin) / width;
            double y = ymin + j * (ymax - ymin) / height;
            int iter = 0;
            double zx = x;
            double zy = y;
            while (iter < maxiter && zx * zx + zy * zy < 4.0) {
                double tmp = zx * zx - zy * zy + cx;
                zy = 2.0 * zx * zy + cy;
                zx = tmp;
                iter++;
            }
            image[j * width + i] = iter-1;
        }
    }

    end_time = clock();
    total_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    printf("Time taken: %f seconds\n", total_time);

    write_png_file("juliaset.png", image, width, height);

    return 0;
}

