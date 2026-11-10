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

int main(){

    // Image initialization
    Image img1 = {0}, img2 = {0}, img3 = {0}, img4 = {0};

    int res1, res2, res3, res4;
    
    double start, end;
    
    printf("Codigo de Charly :)\n");
    
    start = omp_get_wtime();

    for(int i = 0; i < 10; i++){

        char input[100];
        char output1[100];
        char output2[100];
        char output3[100];
        char output4[100];

        // Generate filenames
        sprintf(input, "./images/input/img%d.bmp", i);

        sprintf(output1, "./images/output/Test%d_HorizontalGray.bmp", i);
        sprintf(output2, "./images/output/Test%d_HorizontalRGB.bmp", i);
        sprintf(output3, "./images/output/Test%d_VerticalGray.bmp", i);
        sprintf(output4, "./images/output/Test%d_VerticalRGB.bmp", i);

        // Make copies
        int read1 = bmp_read(&img1, input);
        
        if (!read1) {
            printf("Could not read image: %s\n", input);
            return 1;
        }
        
        int read2 = imgcpy(&img1, &img2);
        int read3 = imgcpy(&img1, &img3);
        int read4 = imgcpy(&img1, &img4);
        
        if (!read2 || !read3 || !read4) {
            printf("Could NOT create image copies\n");
        
            image_free(&img1);
            image_free(&img2);
            image_free(&img3);
            image_free(&img4);
        
            return 1;
        }

        // Make transformations
        #pragma omp parallel num_threads(10)
        {
            #pragma omp sections
            {
                #pragma omp section
                res1 = image_flip_horizontal(&img1, IMAGE_GRAYSCALE);
                
                #pragma omp section
                res2 = image_flip_horizontal(&img2, IMAGE_RGB);  
    
                #pragma omp section
                res3 = image_flip_vertical(&img3, IMAGE_GRAYSCALE);
    
                #pragma omp section
                res4 = image_flip_vertical(&img4, IMAGE_RGB);
    
            }
        }

        // Check succes
        if (!res1 || !res2 || !res3 || !res4) {
            printf("Error processing image %d\n", i);
            return 1;
        }

        // Save results
        if (!bmp_write(&img1, output1) ||
            !bmp_write(&img2, output2) ||
            !bmp_write(&img3, output3) ||
            !bmp_write(&img4, output4)) {

            printf("Error saving image %d\n", i);
            return 1;
        }
        
        // Free memory
        image_free(&img1);
        image_free(&img2);
        image_free(&img3);
        image_free(&img4);

        printf("Image %d processed successfully\n", i);   
    }
    end = omp_get_wtime();  // End time

    printf("\nTotal transformation time: %f seconds\n", end - start);
    
    return 0;
}
