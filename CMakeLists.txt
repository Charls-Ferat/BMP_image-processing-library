cmake_minimum_required(VERSION 3.16)
project(BMPImageProcessor LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(OpenMP REQUIRED COMPONENTS CXX)

# Original C library, built untouched from src/ and include/
add_library(bmp_image STATIC src/image.c)
target_include_directories(bmp_image PUBLIC include)

# Qt desktop application
add_executable(bmp_gui WIN32 MACOSX_BUNDLE
    gui/main.cpp
    gui/MainWindow.cpp
    gui/MainWindow.h
    gui/ImageProcessor.cpp
    gui/ImageProcessor.h
)
set_target_properties(bmp_gui PROPERTIES OUTPUT_NAME "BMPImageProcessor")
target_link_libraries(bmp_gui PRIVATE bmp_image Qt6::Widgets OpenMP::OpenMP_CXX)
