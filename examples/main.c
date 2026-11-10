#include "../include/image.h"
#include <omp.h>

int main(){

    // Image initialization
    Image na1, na2, na3;
    int res1, res2, res3;

    // Time
    double start, end;
    start = omp_get_wtime();

    // Cosas
    printf("Codigo de Charly :) \n");
    
    if(!bmp_read(&na1, "./images/input/img0.bmp")){
        printf("Main: Could NOT read image");
        return 1;
    }

    if(!bmp_read(&na2, "./images/input/img0.bmp")){
        printf("Main: Could NOT read image");
        return 1;
    }

    if(!bmp_read(&na3, "./images/input/img0.bmp")){
        printf("Main: Could NOT read image");
        return 1;
    }

    // Paralelizar
    #pragma omp parallel num_threads(10)
    {
        #pragma omp sections
        {
            #pragma omp section
            res1 = image_flip_horizontal(&na1, IMAGE_GRAYSCALE);

            #pragma omp section
            res2 = image_flip_vertical(&na2, IMAGE_RGB);

            #pragma omp section
            res3 = image_flip_horizontal(&na3, IMAGE_RGB);  
        }
    }
    
    // Check success
    if (!res1 || !res2 || !res3) {
        printf("Error processing images\n");
        image_free(&na1);
        image_free(&na2);
        image_free(&na3);
        return 1;
    }
    
    // Image write
    if(!bmp_write(&na1, "./images/output/Test1.bmp")){
        printf("Main: Failure to write image to file");
        return 1;
    }
    if(!bmp_write(&na2, "./images/output/Test2.bmp")){
        printf("Main: Failure to write image to file");
        return 1;
    }
    if(!bmp_write(&na3, "./images/output/Test3.bmp")){
        printf("Main: Failure to write image to file");
        return 1;
    }

    end = omp_get_wtime();  // End time
    
    // Mem free
    image_free(&na1);    
    image_free(&na2);    
    image_free(&na3);    
    
    printf("Transformation time: %f seconds\n", end - start);
    return 0;
}
