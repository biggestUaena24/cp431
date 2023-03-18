#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <png.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <complex.h>

#define ARRAY_SIZE 10
#define WIDTH 4096 
#define HEIGHT 4096 
#define MAX_ITER 1000
#define NUM_THREADS 8
#define RED_MOD 2 
#define GREEN_MOD 1 
#define BLUE_MOD 1 

typedef struct {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} Pixel;

typedef struct {
    Pixel *image;
    int start_row;
    int end_row;
    double complex c;
} ThreadData;

void *generate_julia_set_section(void *args) {
    ThreadData *data = (ThreadData *)args;
    Pixel *image = data->image;
    int start_row = data->start_row;
    int end_row = data->end_row;
    double complex c = data->c;

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

    return NULL;
}

int main(int argc, char **argv) {
    int fd = shm_open("/julia_image", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    ftruncate(fd, sizeof(Pixel) * WIDTH * HEIGHT);
    Pixel *image = mmap(NULL, sizeof(Pixel) * WIDTH * HEIGHT, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    pthread_t threads[NUM_THREADS];
    ThreadData thread_data[NUM_THREADS];
    double complex c = -0.8 + 0.156 * I;

    for (int i=0;i<NUM_THREADS;i++) {
        thread_data[i].image = image;
        thread_data[i].start_row = i * HEIGHT / NUM_THREADS;
        thread_data[i].end_row = (i + 1) * HEIGHT / NUM_THREADS;
        thread_data[i].c = c;
        pthread_create(&threads[i], NULL, generate_julia_set_section, &thread_data[i]);
    }

    for (int i=0;i<NUM_THREADS;i++) {
        pthread_join(threads[i], NULL);
    }

    FILE *fp = fopen("julia_set.ppm", "wb");
    fprintf(fp, "P6\n%d %d\n255\n", WIDTH, HEIGHT);
    fwrite(image, sizeof(Pixel), WIDTH * HEIGHT, fp);
    fclose(fp);

    munmap(image, sizeof(Pixel) * WIDTH * HEIGHT);
    shm_unlink("/julia_image");

    return 0;
}
