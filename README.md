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
* gui/ — Qt 6 desktop application that uses the library.
* images/ — Input and generated output images.
* tests/ — Test programs.
* docs/ — Technical documentation.
* CMakeLists.txt — Builds the library and the desktop application.

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
|Derivatives |	Applies vertical or horizontal pixel differences|
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

## Desktop Application ##
A Qt 6 Widgets application (`gui/`) that processes BMP images in batches. All image
processing is done by the C library; the application only calls its functions.

* Add BMP files by drag and drop or with a file dialog, and manage them in a queue
  (per-image status, remove selected, clear all).
* Preview the original image in a separate window (double-click a row).
* Choose any of the library transformations: horizontal flip, vertical flip, blur,
  X derivative and Y derivative. Selected ones are applied in the order listed.
* Set the blur kernel size (default 5) and the output color mode (RGB or grayscale).
  The color mode column shows whether each image is RGB (color) or grayscale (B&W).
* Choose the destination folder and a filename suffix (default `_processed`). Existing
  files are never overwritten: if the name is taken, `_1`, `_2`, ... is appended.
* Each image reports its own status. A failure on one image does not stop the others.
* Files the library cannot read are flagged with the reason (only uncompressed 24-bit
  BMP files are supported).

### Requirements ###
* CMake 3.16 or newer
* Qt 6 (Widgets module)
* A C11 and C++17 compiler (GCC, Clang or MSVC)

### Build ###
From the project root:
```bash
cmake -S . -B build-gui -DCMAKE_BUILD_TYPE=Release
cmake --build build-gui
```
If CMake cannot find Qt, point it to your Qt installation:
```bash
cmake -S . -B build-gui -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/<kit>
```

### Run ###
```bash
./build-gui/BMPImageProcessor             # Linux
open build-gui/BMPImageProcessor.app      # macOS
build-gui\Release\BMPImageProcessor.exe   # Windows (MSVC / Visual Studio generator)
```

### Windows setup ###
1. Install Visual Studio 2022 Build Tools with the "Desktop development with C++" workload.
2. Install CMake from cmake.org (tick "Add CMake to the system PATH").
3. Install Qt with the Qt Online Installer from qt.io (free Qt account required). Select
   Qt 6.x > "MSVC 2022 64-bit".
4. In "x64 Native Tools Command Prompt for VS 2022", from the project root:
```bat
cmake -S . -B build-gui -DCMAKE_PREFIX_PATH=C:\Qt\6.x.x\msvc2022_64
cmake --build build-gui --config Release
C:\Qt\6.x.x\msvc2022_64\bin\windeployqt.exe build-gui\Release\BMPImageProcessor.exe
build-gui\Release\BMPImageProcessor.exe
```
`windeployqt` copies the Qt DLLs next to the executable so it runs outside the Qt tools.
Replace `6.x.x` with the installed version.

### Other platforms ###
* Debian/Ubuntu: `sudo apt install cmake build-essential qt6-base-dev`
* macOS: `brew install cmake qt`, then pass `-DCMAKE_PREFIX_PATH="$(brew --prefix qt)"`.

### Notes ###
* The library opens files with `fopen`, so on Windows, paths with characters outside the
  system code page are rejected with an error message.
* The blur uses a radius of `kernel size / 2`: an even size gives the same result as the
  next odd size, and a size of 1 leaves the image unchanged.
* The library has no standalone color conversion, so the output color mode is applied
  through the last selected transformation; at least one must be selected.
