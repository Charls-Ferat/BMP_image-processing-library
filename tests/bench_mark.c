#include "../include/image.h"
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_IMAGES  10
#define NUM_THREADS 5
#define BLUR_KERNEL 9   // Same size as the kernel in transformaciones.h

typedef int (*TransformFn)(Image*, ImageFormat);

typedef struct {
    TransformFn fn;
    ImageFormat format;
    const char* name;
} Transform;

static int blur_wrapper(Image* image, ImageFormat format){
    return image_blur(image, format, BLUR_KERNEL);
}

static const Transform TRANSFORMS[] = {
    { image_flip_horizontal, IMAGE_GRAYSCALE, "HorizontalGray" },
    { image_flip_horizontal, IMAGE_RGB,       "HorizontalRGB"  },
    { image_flip_vertical,   IMAGE_GRAYSCALE, "VerticalGray"   },
    { image_flip_vertical,   IMAGE_RGB,       "VerticalRGB"    },
    { image_derX,            IMAGE_GRAYSCALE, "DerXGray"       },
    { image_derX,            IMAGE_RGB,       "DerXRGB"        },
    { image_derY,            IMAGE_GRAYSCALE, "DerYGray"       },
    { image_derY,            IMAGE_RGB,       "DerYRGB"        },
    { blur_wrapper,          IMAGE_GRAYSCALE, "BlurGray"       },
    { blur_wrapper,          IMAGE_RGB,       "BlurRGB"        }
};

#define NUM_TRANSFORMS (int)(sizeof(TRANSFORMS) / sizeof(TRANSFORMS[0]))

static int imgcpy(const Image* org, Image* mpty) {
    if (org == NULL || mpty == NULL || org->data == NULL) return 0;

    size_t channels = (org->channels == IMAGE_RGB) ? 3 : 1;
    size_t pixel_size = (size_t)org->width * (size_t)org->height * channels;

    mpty->data = malloc(pixel_size);
    if (mpty->data == NULL) return 0;

    mpty->width = org->width;
    mpty->height = org->height;
    mpty->channels = org->channels;
    memcpy(mpty->data, org->data, pixel_size);
    return 1;
}

static void process_task(const Image* original, int image_id, int transformation) {
    Image output;
    if (!imgcpy(original, &output)) {
        printf("Copy failed: image %d\n", image_id);
        return;
    }

    char filename[150];
    snprintf(filename, sizeof(filename), "images/output/Test%d_%s.bmp",
             image_id, TRANSFORMS[transformation].name);

    if (TRANSFORMS[transformation].fn(&output, TRANSFORMS[transformation].format)) {
        if (!bmp_write(&output, filename))
            printf("Failed to save: %s\n", filename);
    } else {
        printf("Transformation failed: image %d, task %d\n", image_id, transformation);
    }

    image_free(&output);
}

int main(void) {
    omp_set_num_threads(NUM_THREADS);
    double start = omp_get_wtime();

    // One image per thread at a time -> bounded memory.
    // dynamic, 1: a thread that finishes early takes the next image
    #pragma omp parallel for schedule(dynamic, 1)
    for (int i = 0; i < NUM_IMAGES; i++) {
        char input[100];
        snprintf(input, sizeof(input), "images/input/img%d.bmp", i);

        Image original;
        if (!bmp_read(&original, input)) {
            printf("Could not read image %d\n", i);
            continue;
        }

        for (int t = 0; t < NUM_TRANSFORMS; t++)
            process_task(&original, i, t);

        image_free(&original);
    }

    printf("Total time: %f seconds\n", omp_get_wtime() - start);
    return 0;
}
