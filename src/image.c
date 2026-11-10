#include "../include/image.h"

static void write_u16_duo(unsigned char *p, uint16_t v){
    p[0] = (uint16_t)(v);
    p[1] = (uint16_t)(v >> 8);
}

static void write_u32_quartet(unsigned char *p, uint32_t v){
    p[0] = (uint32_t)(v); 
    p[1] = (uint32_t)(v >> 8);
    p[2] = (uint32_t)(v >> 16);
    p[3] = (uint32_t)(v >> 24);
}

static uint32_t read_u32_quartet(const unsigned char* p){
    return (uint32_t)p[0]
        | ((uint32_t)p[1] << 8)
        | ((uint32_t)p[2] << 16)
        | ((uint32_t)p[3] << 24);
}

// Checa el struct de la imagen 
static int image_valid(const Image* image){
    return image != NULL && image -> data != NULL &&
           image -> width > 0 && image -> height > 0 &&
           (image -> channels == IMAGE_RGB || image -> channels == IMAGE_GRAYSCALE);
}

// Convierte UN pixel de x canales a y canales (copia, gris o replica)
static inline void convert_pixel(const unsigned char* s, size_t sc,
                                 unsigned char* d, size_t dc){
    if(sc == dc){
        for(size_t c = 0; c < dc; c++) d[c] = s[c];
    } else if(sc == 3){
        d[0] = (unsigned char)(0.21 * s[0] + 0.72 * s[1] + 0.07 * s[2]);
    } else {
        d[0] = d[1] = d[2] = s[0];
    }
}

// Cambia el buffer de la imagen por el nuevo
static void image_replace(Image* image, unsigned char* data, ImageFormat format){
    free(image -> data);
    image -> data = data;
    image -> channels = format;
}

// Pasada horizontal del blur sobre filas [y0, y1)
static void blur_horizontal_rows(const unsigned char* src, unsigned char* dst,
                                 size_t w, size_t h, size_t ch, long k,
                                 size_t y0, size_t y1)
{
    (void)h;
    for(size_t y = y0; y < y1; y++){
        for(size_t x = 0; x < w; x++){
            for(size_t c = 0; c < ch; c++){
                long sum = 0, count = 0;
                for(long dx = -k; dx <= k; dx++){
                    long nx = (long)x + dx;
                    if(nx >= 0 && nx < (long)w){
                        sum += src[(y * w + (size_t)nx) * ch + c];
                        count++;
                    }
                }
                dst[(y * w + x) * ch + c] = (unsigned char)(sum / count);
            }
        }
    }
}

// Pasada vertical del blur sobre filas [y0, y1)
static void blur_vertical_rows(const unsigned char* src, unsigned char* dst,
                               size_t w, size_t h, size_t sc, size_t dc, long k,
                               size_t y0, size_t y1)
{
    unsigned char avg[3];
    for(size_t y = y0; y < y1; y++){
        for(size_t x = 0; x < w; x++){
            for(size_t c = 0; c < sc; c++){
                long sum = 0, count = 0;
                for(long dy = -k; dy <= k; dy++){
                    long ny = (long)y + dy;
                    if(ny >= 0 && ny < (long)h){
                        sum += src[((size_t)ny * w + x) * sc + c];
                        count++;
                    }
                }
                avg[c] = (unsigned char)(sum / count);
            }
            convert_pixel(avg, sc, &dst[(y * w + x) * dc], dc);
        }
    }
}

// Helper para multiplicar por 3 o 1 
static inline size_t format_channels(ImageFormat f){
    return (f == IMAGE_RGB) ? 3 : 1;
}   // Me dio flojera :(

/*
 * Image Free:
 * Frees the internal image representation
 * array from dynamic memory
 */
extern int image_free(Image* image){
    if(image == NULL) return 0;
    free(image -> data);
    image -> data = NULL;
    return 1;
}


/*
 * bmp_read:
 * Reads an image from a .bmp file and
 * writes it into a memory tight representation
 */
extern int bmp_read(Image* image, const char* input_path) {

    if (image == NULL || input_path == NULL) return 0;

    FILE* img = fopen(input_path, "rb");

    if (img == NULL) {
        perror("Cannot open BMP");
        return 0;
    }

    unsigned char header[54];

    if (fread(header, 1, 54, img) != 54) {
        fclose(img);
        puts("Invalid BMP header");
        return 0;
    }

    if (header[0] != 'B' || header[1] != 'M') {
        fclose(img);
        puts("Not a BMP file");
        return 0;
    }

    uint32_t fileSz = read_u32_quartet(&header[2]);
    uint32_t offset = read_u32_quartet(&header[10]);
    uint32_t headerSz = read_u32_quartet(&header[14]);
    uint32_t uWidth = read_u32_quartet(&header[18]);
    uint32_t uHeight = read_u32_quartet(&header[22]);

    uint16_t bpp = (uint16_t)header[28] |
                   ((uint16_t)header[29] << 8);

    uint32_t compression = read_u32_quartet(&header[30]);

    // Validate BMP format
    if (headerSz < 40 ||
        uWidth == 0 || uWidth > INT_MAX ||
        uHeight == 0 || uHeight == 0x80000000u ||
        bpp != 24 ||
        compression != 0 ||
        offset < 54) {

        fclose(img);
        puts("Unsupported or invalid BMP");
        return 0;
    }

    // Interpret height as signed
    int64_t signed_height;

    if (uHeight <= INT32_MAX) {
        signed_height = (int64_t)uHeight;
    } else {
        signed_height = -(int64_t)(~uHeight + 1u);
    }

    int top_down = (signed_height < 0);

    size_t width = (size_t)uWidth;
    size_t height = (size_t)(top_down ? -signed_height : signed_height);

    printf("Tamano de archivo: %u\n", fileSz);
    printf("Pixel Offset: %u\n", offset);
    printf("Header Size: %u\n", headerSz);
    printf("Ancho de imagen: %zu\n", width);
    printf("Alto de imagen: %zu\n", height);
    printf("Bits por pixel: %u\n", bpp);
    printf("Tipo de compresion: %u\n", compression);

    // Calculate row size and padding
    size_t row_size = width * 3;

    size_t padding = (4 - (row_size % 4)) % 4;

    size_t pixel_size = width * height * 3;

    // Allocate memory
    unsigned char* data = malloc(pixel_size);

    if (data == NULL) {
        fclose(img);
        puts("Cannot allocate image");
        return 0;
    }

    if (fseek(img, (long)offset, SEEK_SET) != 0) {
        free(data);
        fclose(img);
        puts("Cannot read file");
        return 0;
    }

    // Read pixels
    for (size_t y = 0; y < height; y++) {

        // Keep internal representation bottom-up
        size_t dest_y = top_down ? (height - 1 - y) : y;

        for (size_t x = 0; x < width; x++) {

            int b = getc(img);
            int g = getc(img);
            int r = getc(img);

            if (b == EOF || g == EOF || r == EOF) {
                free(data);
                fclose(img);
                puts("Unexpected end of BMP");
                return 0;
            }

            size_t index = (dest_y * width + x) * 3;

            data[index]     = (unsigned char)r;
            data[index + 1] = (unsigned char)g;
            data[index + 2] = (unsigned char)b;
        }

        // Skip row padding
        for (size_t p = 0; p < padding; p++) {
            if (getc(img) == EOF) {
                free(data);
                fclose(img);
                puts("Unexpected end of BMP padding");
                return 0;
            }
        }
    }

    image->data = data;
    image->width = (long)width;
    image->height = (long)height;
    image->channels = IMAGE_RGB;

    fclose(img);

    return 1;
}


/*
 * bmp_write:
 * Writes to file and image representation
 * from memory
 */
extern int bmp_write(Image* image, const char* outputPath){

    if(image == NULL || outputPath == NULL) return 0;

    // Prepara el archivo
    FILE* outputImage = fopen(outputPath, "wb");

    if(outputImage == NULL){
        perror("Could not open output file");
        return 0;
    }

    // Size validation
    if(image -> width <= 0 || image -> height <= 0 || image -> data == NULL){
        fclose(outputImage);
        puts("Non valid image sizes to write");
        return 0;
    }

    // Validar canales
    if(image -> channels != IMAGE_RGB && 
        image -> channels != IMAGE_GRAYSCALE){
        fclose(outputImage);
        puts("Unsupported number of channels");
        return 0;
    }

    // Sizes
    size_t channels = format_channels(image -> channels);
    uint64_t height_ = (uint64_t)image -> height;
    uint64_t width_ = (uint64_t)image -> width;
    uint64_t row_bytes = width_ * (uint64_t)channels;
    uint64_t stride = (row_bytes + 3) & ~(uint64_t)3;
    uint64_t padding = stride - row_bytes;
    uint64_t palette_size = (image -> channels == IMAGE_GRAYSCALE) ? 1024 : 0;
    uint64_t offset = 54 + palette_size;
    uint64_t image_size = stride * height_;
    uint64_t file_size = 54 + image_size;

    // File Sz validation
    if(image_size > INT32_MAX || file_size > INT32_MAX){
        fclose(outputImage);
        puts("File oversize");
        return 0;
    }

    // Escribir el header
    unsigned char header[54] = {0};

    // BITMAP file header
    header[0] = 'B';
    header[1] = 'M';
    write_u32_quartet(&header[2], (uint32_t)file_size);
    write_u16_duo(&header[6], 0);
    write_u16_duo(&header[8], 0);
    write_u32_quartet(&header[10], (uint32_t)offset);

    // BITMAP info header   
    write_u32_quartet(&header[14], 40);
    write_u32_quartet(&header[18], (uint32_t)width_);
    write_u32_quartet(&header[22], (uint32_t)height_);
    write_u16_duo(&header[26], 1);
    write_u16_duo(&header[28], (image -> channels == IMAGE_GRAYSCALE) ? 8 : 24);   // bits por pixel
    write_u32_quartet(&header[30], 0);
    write_u32_quartet(&header[34], (uint32_t)image_size);
    write_u32_quartet(&header[38], 0);
    write_u32_quartet(&header[42], 0);
    write_u32_quartet(&header[46], (image -> channels == IMAGE_GRAYSCALE) ? 256 : 0);   // Palet
    write_u32_quartet(&header[50], 0);

    // Guardar el header
    if(fwrite(header, sizeof(header[0]), sizeof(header), outputImage) != sizeof(header)){
        fclose(outputImage);
        puts("Writing failure on header write");
        return 0;
    }

    // Guardar la imagen
    //Imagenes en grises
    if(image -> channels == IMAGE_GRAYSCALE){
        // Paleta: el indice i corresponde al gris i (B, G, R, reservado)
        for(int i = 0; i < 256; i++){
            putc(i, outputImage);
            putc(i, outputImage);
            putc(i, outputImage);
            putc(0, outputImage);
        }

        for(size_t y = 0; y < (size_t)height_; y++){
            for(size_t x = 0; x < (size_t)width_; x++){
                putc(image -> data[y * (size_t)width_ + x], outputImage);
            }
            for(size_t p = 0; p < (size_t)padding; p++){
                putc(0, outputImage);
            }
        }
    }
    
    // Imagenes a color
    if(image -> channels == IMAGE_RGB){
        for(size_t y = 0; y < (size_t)height_; y++){
            for(size_t x = 0; x < (size_t)width_; x++){
                size_t i = (y * (size_t)width_ + x) * 3;
                putc(image -> data[i + 2], outputImage);   // B
                putc(image -> data[i + 1], outputImage);   // G
                putc(image -> data[i],     outputImage);   // R
            }
            for(size_t p = 0; p < (size_t)padding; p++){
                putc(0, outputImage);
            }
        }
    }

    if(ferror(outputImage)){
        fclose(outputImage);
        puts("Writing failure on pixel data");
        return 0;
    }

    fclose(outputImage);
    return 1;
}

/*
 * Image Horizontal Flip:
 * Inverts an image in the horizontal sense
 */
extern int image_flip_horizontal(Image* image, ImageFormat format){
    if(!image_valid(image)) return 0;

    size_t w = (size_t)image -> width;  
    size_t h = (size_t)image -> height;
    size_t sc = format_channels(image -> channels); //Source channel
    size_t dc = format_channels(format);    // Destin channel

    unsigned char* out = malloc(w * h * dc);
    if(out == NULL) return 0;

    for(size_t y = 0; y < h; y++){
        for(size_t x = 0; x < w; x++){
            convert_pixel(&image -> data[(y * w + (w - 1 - x)) * sc], sc,
                          &out[(y * w + x) * dc], dc);
        }
    }

    image_replace(image, out, format);
    return 1;
}

/*
 * Image Vertical Flip:
 * Flips a bmp memory representation
 * in the vertical sense
 */
extern int image_flip_vertical(Image* image, ImageFormat format){
    if(!image_valid(image)) return 0;

    size_t w = (size_t)image -> width;  
    size_t h = (size_t)image -> height;
    size_t sc = format_channels(image -> channels); //Source channel
    size_t dc = format_channels(format);    // Destin channel

    unsigned char* out = malloc(w * h * dc);
    if(out == NULL) return 0;

    for(size_t y = 0; y < h; y++){
        const unsigned char* src_row = &image -> data[(h - 1 - y) * w * sc];
        unsigned char* dst_row = &out[y * w * dc];

        if(sc == dc){
            memcpy(dst_row, src_row, w * sc);   // mismo formato: fila completa
        } else {
            for(size_t x = 0; x < w; x++){
                convert_pixel(&src_row[x * sc], sc, &dst_row[x * dc], dc);
            }
        }
    }

    image_replace(image, out, format);
    return 1;
}

/*
 * Image Blur:
 * Blurs an image with a kernel of
 * n size
 */
extern int image_blur(Image* image, ImageFormat format, int kernel_size){
    if(kernel_size < 1 || !image_valid(image)) return 0;

    size_t w = (size_t)image -> width;  
    size_t h = (size_t)image -> height;
    size_t sc = format_channels(image -> channels); //Source channel
    size_t dc = format_channels(format);    // Destin channel
    long k = kernel_size / 2;

    unsigned char* temp = malloc(w * h * sc);
    unsigned char* out = malloc(w * h * dc);
    if(temp == NULL || out == NULL){
        free(temp);
        free(out);
        return 0;
    }

    blur_horizontal_rows(image -> data, temp, w, h, sc, k, 0, h);
    blur_vertical_rows(temp, out, w, h, sc, dc, k, 0, h);

    free(temp);
    image_replace(image, out, format);
    return 1;

    // Wohooo
    // I got my head checked
    // By a jumbo jet
    // -- Blur(Song 2)
}
