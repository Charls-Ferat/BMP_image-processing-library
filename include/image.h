#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stddef.h>
// holaaa charly
// holaaaa

typedef enum {
    IMAGE_RGB,
    IMAGE_GRAYSCALE
} ImageFormat;

typedef struct {
    long width;
    long height;
    ImageFormat channels;       // 3 colors, 1 grayscale
    unsigned char* data;
} Image;

// Internas
static void write_u16_duo(unsigned char *p, uint16_t v);
static void write_u32_quartet(unsigned char *p, uint32_t v);
static uint32_t read_u32_quartet(const unsigned char* p);
static int image_valid(const Image* image);
static inline void convert_pixel(const unsigned char* s, size_t sc,
                                 unsigned char* d, size_t dc);
static void image_replace(Image* image, unsigned char* data, ImageFormat channels);
static void blur_horizontal_rows(const unsigned char* src, unsigned char* dst,
                                 size_t w, size_t h, size_t ch, long k,
                                 size_t y0, size_t y1);
static void blur_vertical_rows(const unsigned char* src, unsigned char* dst,
                               size_t w, size_t h, size_t sc, size_t dc, long k,
                               size_t y0, size_t y1);
static inline size_t format_channels(ImageFormat f);

// Usaurio
extern int image_free(Image* image);
extern int bmp_read(Image* image, const char* inputPath);
extern int bmp_write(Image* image, const char* outputPath);
extern int image_flip_horizontal(Image* image, ImageFormat format);
extern int image_flip_vertical(Image* image, ImageFormat format);
extern int image_blur(Image* image, ImageFormat format, int kernel_size);
