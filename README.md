# C Image Processing Library #

A lightweight image processing library developed in C for handling and manipulating raster images.

The project implements a memory-based image representation and a collection of image processing operations, with support for uncompressed BMP file input and output.

## Features ##
* BMP image reading and writing.
* RGB and grayscale image representations.
* Pixel format conversion.
* Horizontal and vertical image flipping.
* Box blur using a separable filtering approach.
* Dynamic memory management for image data.
## Technologies ##
* C 
* Standard C Library
* GCC / Clang
* Open MP

## Project Organization ##
* include/ — Public library headers.
* src/ — Library implementation.
* examples/ — Example programs demonstrating library functionality.
* images/ — Input and generated output images.
* tests/ — Test programs.
* docs/ — Technical documentation.

## Supported Image Formats ##


|Format	|Reading	|Writing|
|-------|---------|---------|
|BMP |(24-bit RGB)|	Yes	| Yes|
|BMP | (8-bit grayscale) |	No | Yes|


## Image Processing Operations ##

|Operation	| Description|
|-----------|------------|
|Horizontal | Flip	Reverses pixel order along the horizontal axis.|
|Vertical | Flip	Reverses pixel order along the vertical axis.|
|Box Blur |	Applies a two-pass spatial averaging filter.|
|Format Conversion	| Converts between RGB and grayscale representations.|


## Compilation ##
```bash
gcc -fopenmp .\examples\main.c .\src\image.c
```

## Usage ##
```c
#include "image.h"

Image image = {0};

if (bmp_read(&image, "images/input/image1.bmp")) {
    image_flip_horizontal(&image, IMAGE_RGB);
    image_blur(&image, IMAGE_RGB, 5);

    bmp_write(&image, "images/output/result.bmp");

    image_free(&image);
}
```
