#pragma once

#include <QList>
#include <QString>

// Transformations offered by the C library (include/image.h)
enum class Transformation {
    FlipHorizontal,     // image_flip_horizontal
    FlipVertical,       // image_flip_vertical
    Blur,               // image_blur
    DerivativeX,        // image_derX
    DerivativeY         // image_derY
};

// Maps to the library's ImageFormat (IMAGE_RGB / IMAGE_GRAYSCALE)
enum class ColorMode {
    Rgb,
    Grayscale
};

struct ProcessingOptions {
    QList<Transformation> transformations;  // applied in this order
    ColorMode outputMode = ColorMode::Rgb;
    int blurKernelSize = 5;                 // only used by Transformation::Blur
};

struct ProcessingResult {
    bool ok = false;
    QString error;          // set when ok is false
    QString outputPath;     // set when ok is true
    ColorMode mode = ColorMode::Rgb;
};

// Bridge between the Qt UI and the C library. It only calls the library
// functions; every transformation is done by the library itself.
class ImageProcessor {
public:
    // Checks the file against what bmp_read() accepts. Returns an empty
    // string if the library can read it, otherwise a message with the reason.
    static QString validateBmp(const QString& path);

    // Reads inputPath, applies the transformations and writes
    // <outputDir>/<name><suffix>.bmp. It never overwrites an existing file:
    // if the name is taken, _1, _2, ... is appended.
    static ProcessingResult process(const QString& inputPath,
                                    const QString& outputDir,
                                    const QString& suffix,
                                    const ProcessingOptions& options);

    static QString transformationName(Transformation t);
    static QString colorModeName(ColorMode mode);
};
