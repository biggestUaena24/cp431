#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include <math.h>
#include <complex.h>
#include <pthread.h>

#define WIDTH 1024
#define HEIGHT 1024
#define MAX_ITER 1000
#define NUM_THREADS 8

double complex c_start = -2.0 - 2.0 * I;
double complex c_end = 2.0 + 2.0 * I;

typedef struct {
    int start_frame;
    int end_frame;
    int num_frames;
    double radius;
} ThreadData;

int mandelbrot_iterations(double complex c, double complex z, int max_iter) {
    int n = 0;
    while (cabs(z) <= 2 && n < max_iter) {
        z = z * z + c;
        n++;
    }
    return n;
}

void create_image(char *filename, double complex c) {
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
    png_set_IHDR(png_ptr, info_ptr, WIDTH, HEIGHT, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png_ptr, info_ptr);

    double complex z;
    int i, j, iter;
    png_bytep row = (png_bytep)malloc(3 * WIDTH * sizeof(png_byte));
    for (j = 0; j < HEIGHT; j++) {
        for (i = 0; i < WIDTH; i++) {
            double x = 3.0 * (i - WIDTH / 2) / (WIDTH / 2);
            double y = 3.0 * (j - HEIGHT / 2) / (HEIGHT / 2);
            z = x + y * I;
            iter = mandelbrot_iterations(c, z, MAX_ITER);

            row[i * 3] = (iter * 32) % 256; // R
//            row[i * 3 + 1] = (iter * 16) % 256; // G
            row[i * 3 + 1] = 0;
            row[i * 3 + 2] = (iter * 8) % 256;  // B
        }
        png_write_row(png_ptr, row);
    }

    png_write_end(png_ptr, NULL);
    fclose(fp);
    png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
    png_destroy_write_struct(&png_ptr, NULL);
    free(row);
}

void *create_images(void *arg) {
    ThreadData *thread_arg = (ThreadData *)arg;
    int start_frame = thread_arg->start_frame;
    int end_frame = thread_arg->end_frame;
    int num_frames = thread_arg->num_frames;

    for (int frame = start_frame; frame < end_frame; frame++) {
        double t = (double)(frame - start_frame) / num_frames;
        double complex c = c_start + t * (c_end - c_start);

        char filename[64];
        snprintf(filename, sizeof(filename), "animation/frame_%04d.png", frame);
        
        // Call create_image function here
        create_image(filename, c);
        printf("Image frame created: %s\n", filename);
    }

    return NULL;
}

int main(int argc, char **argv) {
    int num_frames = atoi(argv[2]);
    double radius = strtod(argv[1], NULL);
    pthread_t threads[NUM_THREADS];
    ThreadData thread_args[NUM_THREADS];

    int frames_per_thread = num_frames / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_args[i].start_frame = i * frames_per_thread;
        thread_args[i].end_frame = (i == NUM_THREADS - 1) ? num_frames : (i + 1) * frames_per_thread;
        thread_args[i].radius = radius;
        thread_args[i].num_frames = num_frames;
        pthread_create(&threads[i], NULL, create_images, (void *)&thread_args[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Animation frames generated. Use video editing software or tools like FFmpeg to create a video or animated GIF.\n");

    return 0;
}
