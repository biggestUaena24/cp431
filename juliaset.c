#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <png.h>

#define WIDTH 1024
#define HEIGHT 1024

void write_png_file(char* filename, int* buffer) {
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

    png_set_IHDR(png_ptr, info_ptr, WIDTH, HEIGHT, 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(png_ptr, info_ptr);

    png_bytep row_pointer[HEIGHT];
    for (int y = 0; y < HEIGHT; y++) {
        row_pointer[y] = (png_bytep)(buffer + y * WIDTH);
    }
    png_write_image(png_ptr, row_pointer);

    png_write_end(png_ptr, NULL);

    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
}

int main() {
    double cx = -0.8;
    double cy = 0.156;
    double xmin = -1.5;
    double xmax = 1.5;
    double ymin = -1.5;
    double ymax = 1.5;
    int maxiter = 1000;

    int image[WIDTH * HEIGHT];

    for (int j = 0; j < HEIGHT; j++) {
        for (int i = 0; i < WIDTH; i++) {
            double x = xmin + i * (xmax - xmin) / WIDTH;
            double y = ymin + j * (ymax - ymin) / HEIGHT;
            int iter = 0;
            double zx = x;
            double zy = y;
            while (iter < maxiter && zx * zx + zy * zy < 4.0) {
                double tmp = zx * zx - zy * zy + cx;
                zy = 2.0 * zx * zy + cy;
                zx = tmp;
                iter++;
            }
            image[j * WIDTH + i] = iter-1;
        }
    }

    write_png_file("juliaset.png", image);

    return 0;
}

