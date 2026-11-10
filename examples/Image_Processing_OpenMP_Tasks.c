#include "../include/image.h"
#include <omp.h>

int imgcpy(const Image* org, Image* mpty) {

    if (org == NULL || mpty == NULL || org->data == NULL) {
        return 0;
    }

    mpty->width = org->width;
    mpty->height = org->height;
    mpty->channels = org->channels;

    size_t channels = (org->channels == IMAGE_RGB) ? 3 : 1;

    size_t pixel_size = (size_t)org->width *
                        (size_t)org->height *
                        channels;

    mpty->data = malloc(pixel_size);

    if (mpty->data == NULL) {
        return 0;
    }

    memcpy(mpty->data, org->data, pixel_size);

    printf("Copy success!! Bytes copied: %zu\n", pixel_size);

    return 1;
}


// Each task owns its output image.
void process_task(Image *original, int image_id, int transformation) {

    Image output;

    if (!imgcpy(original, &output)) {
        printf("Copy failed: image %d\n", image_id);
        return;
    }

    char filename[150];
    int result = 0;

    switch (transformation) {

        case 0:
            result = image_flip_horizontal(&output, IMAGE_GRAYSCALE);
            sprintf(filename,
                    "./images/output/Test%d_HorizontalGray.bmp", image_id);
            break;

        case 1:
            result = image_flip_horizontal(&output, IMAGE_RGB);
            sprintf(filename,
                    "./images/output/Test%d_HorizontalRGB.bmp", image_id);
            break;

        case 2:
            result = image_flip_vertical(&output, IMAGE_GRAYSCALE);
            sprintf(filename,
                    "./images/output/Test%d_VerticalGray.bmp", image_id);
            break;

        case 3:
            result = image_flip_vertical(&output, IMAGE_RGB);
            sprintf(filename,
                    "./images/output/Test%d_VerticalRGB.bmp", image_id);
            break;
    }

    if (result) {
        bmp_write(&output, filename);
    }

    image_free(&output);
}

int main() {

    double start = omp_get_wtime();

    #pragma omp parallel
    {
        #pragma omp single
        {
            for (int i = 0; i < 10; i++) {

                #pragma omp task firstprivate(i)
                {
                    char input[100];
                    sprintf(input, "./images/input/img%d.bmp", i);

                    Image original;

                    if (!bmp_read(&original, input)) {
                        printf("Could not read image %d\n", i);
                    }
                    else {

                        for (int t = 0; t < 4; t++) {

                            #pragma omp task firstprivate(i, t) \
                                              shared(original)
                            {
                                process_task(&original, i, t);
                            }
                        }

                        // Wait for all transformations.
                        #pragma omp taskwait

                        image_free(&original);
                    }
                }
            }

            // Wait for all tasks.
            #pragma omp taskwait
        }
    }

    double end = omp_get_wtime();

    printf("Total time: %f seconds\n", end - start);

    return 0;
}
