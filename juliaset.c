#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <png.h>
#include <time.h>

typedef struct {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} Pixel;

void generate_julia_set(Pixel *image, double complex c, int max_iter, int width, int height, int red_mod, int blue_mod, int green_mod) {
    for (int y=0;y<height;y++) {
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
    if (argc < 9) {
        printf("Usage: ./gen_juliaset [width] [height] [c1] [c2] [iter] [red] [blue] [green]\n");
        exit(1);
    }

    // image dimension
    int width = atoi(argv[1]);
    int height = atoi(argv[2]);

    // c constant, real + imag
    char *endptr;
    double c1 = strtod(argv[3], &endptr);
    double c2 = strtod(argv[4], &endptr);
    double complex c = c1 + c2 * I;

    int max_iter = atoi(argv[5]);

    // colour modifiers
    int red_mod = atoi(argv[6]);
    int blue_mod = atoi(argv[7]);
    int green_mod = atoi(argv[8]);

    // for benchmark
    clock_t start_time, end_time;
    double total_time;

    Pixel *image = malloc(sizeof(Pixel) * width * height);

    start_time = clock();

    generate_julia_set(image, c, max_iter, width, height, red_mod, blue_mod, green_mod);

    end_time = clock();
    total_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    printf("Time taken: %f seconds\n", total_time);

    char filename[100];
    sprintf(filename, "juliaset_%.6lf_%.6lf.png", c1, c2);
    write_png_file(filename, width, height, image);

    return 0;
}

