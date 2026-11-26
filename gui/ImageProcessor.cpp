#include "ImageProcessor.h"

#include <QMutex>
#include <QMutexLocker>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <climits>
#include <cstdint>

#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
extern "C" {
    #include "image.h"
}

namespace {

uint32_t readU32(const unsigned char* p) {
    return static_cast<uint32_t>(p[0])
        | (static_cast<uint32_t>(p[1]) << 8)
        | (static_cast<uint32_t>(p[2]) << 16)
        | (static_cast<uint32_t>(p[3]) << 24);
}

// The library opens files with fopen(), which takes an 8-bit path. Paths that
// do not survive that conversion cannot be opened by it.
bool pathUsableByLibrary(const QString& path) {
    return QString::fromLocal8Bit(QFile::encodeName(path)) == path;
}

QString compressionName(uint32_t compression) {
    switch (compression) {
    case 1:  return "RLE 8-bit";
    case 2:  return "RLE 4-bit";
    case 3:  return "bitfields";
    case 4:  return "embedded JPEG";
    case 5:  return "embedded PNG";
    default: return QString("code %1").arg(compression);
    }
}

ImageFormat toLibraryFormat(ColorMode mode) {
    return (mode == ColorMode::Grayscale) ? IMAGE_GRAYSCALE : IMAGE_RGB;
}

// Never returns the path of an existing file, so nothing is overwritten.
// Returns an empty string if the file cannot be created.
QString reserveOutputPath(const QString& inputPath, const QString& outputDir,
                          const QString& suffix) {
    static QMutex mutex;
    QMutexLocker locker(&mutex);

    QString baseName = QFileInfo(inputPath).completeBaseName() + suffix;
    QDir dir(outputDir);

    QString path = dir.filePath(baseName + ".bmp");
    for (int n = 1; QFileInfo::exists(path); ++n) {
        path = dir.filePath(QString("%1_%2.bmp").arg(baseName).arg(n));
    }

    QFile placeholder(path);
    if (!placeholder.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
        return QString();
    }
    return path;
}

int applyTransformation(Image* image, Transformation t, ImageFormat format,
                        int blurKernelSize) {
    switch (t) {
    case Transformation::FlipHorizontal: return image_flip_horizontal(image, format);
    case Transformation::FlipVertical:   return image_flip_vertical(image, format);
    case Transformation::Blur:           return image_blur(image, format, blurKernelSize);
    case Transformation::DerivativeX:    return image_derX(image, format);
    case Transformation::DerivativeY:    return image_derY(image, format);
    }
    return 0;
}

}  // namespace

// Mirrors the checks done by bmp_read() in src/image.c, plus a size check,
// since the library only discovers a truncated file while reading the pixels.
QString ImageProcessor::validateBmp(const QString& path) {
    if (!pathUsableByLibrary(path)) {
        return "The path has characters the C library cannot open on this system.";
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return "Cannot open the file: " + file.errorString();
    }

    unsigned char header[54];
    if (file.read(reinterpret_cast<char*>(header), sizeof(header)) != sizeof(header)) {
        return "Invalid BMP: the file is smaller than a BMP header (54 bytes).";
    }

    if (header[0] != 'B' || header[1] != 'M') {
        return "Not a BMP file (missing the 'BM' signature).";
    }

    uint32_t offset = readU32(&header[10]);
    uint32_t headerSize = readU32(&header[14]);
    uint32_t width = readU32(&header[18]);
    uint32_t rawHeight = readU32(&header[22]);
    unsigned bpp = header[28] | (header[29] << 8);
    uint32_t compression = readU32(&header[30]);

    if (headerSize < 40) {
        return QString("Unsupported BMP header (%1 bytes). The library needs a "
                       "header of at least 40 bytes.").arg(headerSize);
    }
    if (bpp != 24) {
        return QString("Unsupported bit depth: %1-bit. The library only reads "
                       "24-bit BMP files.").arg(bpp);
    }
    if (compression != 0) {
        return QString("Unsupported compression: %1. The library only reads "
                       "uncompressed BMP files.").arg(compressionName(compression));
    }
    if (width == 0 || width > INT_MAX || rawHeight == 0 || rawHeight == 0x80000000u) {
        return "Invalid image dimensions.";
    }
    if (offset < 54) {
        return "Invalid pixel data offset.";
    }

    // A negative height means a top-down BMP; the library accepts both
    uint64_t height = (rawHeight <= INT32_MAX)
        ? rawHeight
        : static_cast<uint32_t>(~rawHeight + 1u);
    uint64_t rowSize = static_cast<uint64_t>(width) * 3;
    uint64_t stride = (rowSize + 3) & ~static_cast<uint64_t>(3);
    uint64_t needed = offset + stride * (height - 1) + rowSize;

    if (static_cast<uint64_t>(file.size()) < needed) {
        return "The file is truncated: the pixel data is incomplete.";
    }

    return QString();
}

ProcessingResult ImageProcessor::process(const QString& inputPath,
                                         const QString& outputDir,
                                         const QString& suffix,
                                         const ProcessingOptions& options) {
    ProcessingResult result;

    if (options.transformations.isEmpty()) {
        result.error = "No transformation selected.";
        return result;
    }

    QFileInfo dirInfo(outputDir);
    if (!dirInfo.isDir() || !dirInfo.isWritable()) {
        result.error = "The output folder does not exist or is not writable.";
        return result;
    }

    QString problem = validateBmp(inputPath);
    if (!problem.isEmpty()) {
        result.error = problem;
        return result;
    }

    Image image = {};
    if (!bmp_read(&image, QFile::encodeName(inputPath).constData())) {
        result.error = "The library could not read the image.";
        return result;
    }

    // The name is reserved only now, so a failure above leaves nothing behind
    QString outputPath = reserveOutputPath(inputPath, outputDir, suffix);
    if (outputPath.isEmpty()) {
        image_free(&image);
        result.error = "Could not create the output file. Check the folder permissions.";
        return result;
    }
    if (!pathUsableByLibrary(outputPath)) {
        QFile::remove(outputPath);
        image_free(&image);
        result.error = "The output path has characters the C library cannot open "
                       "on this system.";
        return result;
    }

    // bmp_write() refuses files over INT32_MAX bytes; check it before spending
    // time on the transformations.
    bool grayscaleOut = (options.outputMode == ColorMode::Grayscale);
    uint64_t outStride = ((static_cast<uint64_t>(image.width) * (grayscaleOut ? 1 : 3)) + 3)
                         & ~static_cast<uint64_t>(3);
    uint64_t outSize = 54 + (grayscaleOut ? 1024 : 0)
                       + outStride * static_cast<uint64_t>(image.height);
    if (outSize > INT32_MAX) {
        image_free(&image);
        result.error = "The result would exceed the BMP size limit of the library (2 GB).";
        return result;
    }

    // Only the last step converts to the chosen output mode; the earlier ones
    // keep the current mode.
    int count = static_cast<int>(options.transformations.size());
    for (int i = 0; i < count; ++i) {
        Transformation t = options.transformations.at(i);
        ImageFormat format = (i == count - 1) ? toLibraryFormat(options.outputMode)
                                              : image.channels;

        if (!applyTransformation(&image, t, format, options.blurKernelSize)) {
            image_free(&image);
            result.error = QString("%1 failed (invalid image, invalid parameter or "
                                   "not enough memory).").arg(transformationName(t));
            return result;
        }
    }

    bool written = bmp_write(&image, QFile::encodeName(outputPath).constData()) != 0;
    ImageFormat finalFormat = image.channels;
    image_free(&image);

    if (!written) {
        // bmp_write() creates the file before it can fail, so remove what is left
        QFile::remove(outputPath);
        result.error = "The library could not write the output file. Check the "
                       "folder permissions and the free disk space.";
        return result;
    }

    result.ok = true;
    result.outputPath = outputPath;
    result.mode = (finalFormat == IMAGE_GRAYSCALE) ? ColorMode::Grayscale : ColorMode::Rgb;
    return result;
}

QString ImageProcessor::transformationName(Transformation t) {
    switch (t) {
    case Transformation::FlipHorizontal: return "Flip horizontal";
    case Transformation::FlipVertical:   return "Flip vertical";
    case Transformation::Blur:           return "Blur";
    case Transformation::DerivativeX:    return "Derivative X";
    case Transformation::DerivativeY:    return "Derivative Y";
    }
    return QString();
}

QString ImageProcessor::colorModeName(ColorMode mode) {
    return (mode == ColorMode::Grayscale) ? "Grayscale (B&W)" : "RGB (color)";
}
