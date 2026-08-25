#include "FloorPlanImageRenderer.h"

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <JPEGDEC.h>
#include <PNGdec.h>
#include <esp_heap_caps.h>

#include <cctype>
#include <cstring>
#include <new>

namespace
{
    constexpr size_t MaximumCompressedImageBytes =
        6UL * 1024UL * 1024UL;

    struct RenderContext
    {
        lv_color_t *canvas = nullptr;
        uint16_t canvasWidth = 0;
        uint16_t canvasHeight = 0;

        uint16_t sourceWidth = 0;
        uint16_t sourceHeight = 0;

        uint16_t renderedWidth = 0;
        uint16_t renderedHeight = 0;

        uint16_t offsetX = 0;
        uint16_t offsetY = 0;

        PNG *pngDecoder = nullptr;
        uint16_t *pngLineBuffer = nullptr;
        uint16_t pngLineCapacity = 0;
    };

    bool TextEqualsIgnoreCase(
        const char *left,
        const char *right)
    {
        if (left == nullptr || right == nullptr)
        {
            return false;
        }

        while (*left != '\0' && *right != '\0')
        {
            if (std::tolower(
                    static_cast<unsigned char>(*left)) !=
                std::tolower(
                    static_cast<unsigned char>(*right)))
            {
                return false;
            }

            ++left;
            ++right;
        }

        return *left == '\0' && *right == '\0';
    }

    const char *GetExtension(const char *path)
    {
        if (path == nullptr)
        {
            return nullptr;
        }

        const char *extension =
            std::strrchr(path, '.');

        return extension != nullptr
            ? extension
            : "";
    }

    void ClearCanvas(
        lv_color_t *canvas,
        uint16_t width,
        uint16_t height)
    {
        if (canvas == nullptr)
        {
            return;
        }

        const lv_color_t black =
            lv_color_make(0, 0, 0);

        const size_t pixelCount =
            static_cast<size_t>(width) *
            static_cast<size_t>(height);

        for (size_t index = 0;
             index < pixelCount;
             ++index)
        {
            canvas[index] = black;
        }
    }

    lv_color_t ConvertRgb565(uint16_t pixel)
    {
        const uint8_t red =
            static_cast<uint8_t>(
                ((pixel >> 11U) & 0x1FU) * 255U / 31U);

        const uint8_t green =
            static_cast<uint8_t>(
                ((pixel >> 5U) & 0x3FU) * 255U / 63U);

        const uint8_t blue =
            static_cast<uint8_t>(
                (pixel & 0x1FU) * 255U / 31U);

        return lv_color_make(
            red,
            green,
            blue);
    }

    bool PrepareFit(
        RenderContext &context,
        uint16_t sourceWidth,
        uint16_t sourceHeight)
    {
        if (context.canvas == nullptr ||
            context.canvasWidth == 0 ||
            context.canvasHeight == 0 ||
            sourceWidth == 0 ||
            sourceHeight == 0)
        {
            return false;
        }

        context.sourceWidth = sourceWidth;
        context.sourceHeight = sourceHeight;

        uint32_t renderedWidth = sourceWidth;
        uint32_t renderedHeight = sourceHeight;

        // Never upscale a Floor Plan. If it does not fit, preserve
        // aspect ratio and scale it down to the largest possible size.
        if (renderedWidth > context.canvasWidth ||
            renderedHeight > context.canvasHeight)
        {
            const uint64_t widthLimitedHeight =
                static_cast<uint64_t>(sourceHeight) *
                context.canvasWidth /
                sourceWidth;

            if (widthLimitedHeight <=
                context.canvasHeight)
            {
                renderedWidth = context.canvasWidth;
                renderedHeight =
                    static_cast<uint32_t>(
                        widthLimitedHeight);
            }
            else
            {
                renderedHeight = context.canvasHeight;
                renderedWidth =
                    static_cast<uint32_t>(
                        static_cast<uint64_t>(sourceWidth) *
                        context.canvasHeight /
                        sourceHeight);
            }
        }

        if (renderedWidth == 0)
        {
            renderedWidth = 1;
        }

        if (renderedHeight == 0)
        {
            renderedHeight = 1;
        }

        context.renderedWidth =
            static_cast<uint16_t>(renderedWidth);

        context.renderedHeight =
            static_cast<uint16_t>(renderedHeight);

        context.offsetX =
            static_cast<uint16_t>(
                (context.canvasWidth -
                 context.renderedWidth) /
                2U);

        context.offsetY =
            static_cast<uint16_t>(
                (context.canvasHeight -
                 context.renderedHeight) /
                2U);

        return true;
    }

    void DrawMappedPixel(
        RenderContext &context,
        uint16_t sourceX,
        uint16_t sourceY,
        lv_color_t color)
    {
        if (sourceX >= context.sourceWidth ||
            sourceY >= context.sourceHeight)
        {
            return;
        }

        const uint16_t destinationX =
            static_cast<uint16_t>(
                static_cast<uint32_t>(sourceX) *
                context.renderedWidth /
                context.sourceWidth);

        const uint16_t destinationY =
            static_cast<uint16_t>(
                static_cast<uint32_t>(sourceY) *
                context.renderedHeight /
                context.sourceHeight);

        if (destinationX >= context.renderedWidth ||
            destinationY >= context.renderedHeight)
        {
            return;
        }

        const uint16_t canvasX =
            static_cast<uint16_t>(
                context.offsetX + destinationX);

        const uint16_t canvasY =
            static_cast<uint16_t>(
                context.offsetY + destinationY);

        if (canvasX >= context.canvasWidth ||
            canvasY >= context.canvasHeight)
        {
            return;
        }

        context.canvas[
            static_cast<size_t>(canvasY) *
                context.canvasWidth +
            canvasX] = color;
    }

    uint8_t *LoadCompressedImage(
        const char *imagePath,
        size_t &imageSize)
    {
        imageSize = 0;

        File file =
            SD.open(imagePath, FILE_READ);

        if (!file || file.isDirectory())
        {
            if (file)
            {
                file.close();
            }

            return nullptr;
        }

        imageSize = file.size();

        if (imageSize == 0 ||
            imageSize > MaximumCompressedImageBytes)
        {
            file.close();
            imageSize = 0;
            return nullptr;
        }

        uint8_t *buffer =
            static_cast<uint8_t *>(
                heap_caps_malloc(
                    imageSize,
                    MALLOC_CAP_SPIRAM |
                        MALLOC_CAP_8BIT));

        if (buffer == nullptr)
        {
            buffer =
                static_cast<uint8_t *>(
                    heap_caps_malloc(
                        imageSize,
                        MALLOC_CAP_8BIT));
        }

        if (buffer == nullptr)
        {
            file.close();
            imageSize = 0;
            return nullptr;
        }

        const size_t bytesRead =
            file.read(buffer, imageSize);

        file.close();

        if (bytesRead != imageSize)
        {
            heap_caps_free(buffer);
            imageSize = 0;
            return nullptr;
        }

        return buffer;
    }

    int HandlePngDraw(PNGDRAW *draw)
    {
        if (draw == nullptr ||
            draw->pUser == nullptr)
        {
            return 0;
        }

        RenderContext &context =
            *static_cast<RenderContext *>(
                draw->pUser);

        if (context.pngDecoder == nullptr ||
            context.pngLineBuffer == nullptr ||
            draw->iWidth <= 0 ||
            draw->iWidth > context.pngLineCapacity ||
            draw->y < 0 ||
            draw->y >= context.sourceHeight)
        {
            return 0;
        }

        context.pngDecoder->getLineAsRGB565(
            draw,
            context.pngLineBuffer,
            PNG_RGB565_LITTLE_ENDIAN,
            0x00FFFFFFUL);

        const uint16_t sourceY =
            static_cast<uint16_t>(draw->y);

        const uint16_t width =
            static_cast<uint16_t>(draw->iWidth);

        for (uint16_t sourceX = 0;
             sourceX < width;
             ++sourceX)
        {
            DrawMappedPixel(
                context,
                sourceX,
                sourceY,
                ConvertRgb565(
                    context.pngLineBuffer[
                        sourceX]));
        }

        return 1;
    }

    bool RenderPng(
        const char *imagePath,
        RenderContext &context)
    {
        size_t imageSize = 0;
        uint8_t *imageData =
            LoadCompressedImage(
                imagePath,
                imageSize);

        if (imageData == nullptr)
        {
            return false;
        }

        void *decoderMemory =
            heap_caps_malloc(
                sizeof(PNG),
                MALLOC_CAP_SPIRAM |
                    MALLOC_CAP_8BIT);

        if (decoderMemory == nullptr)
        {
            decoderMemory =
                heap_caps_malloc(
                    sizeof(PNG),
                    MALLOC_CAP_8BIT);
        }

        if (decoderMemory == nullptr)
        {
            heap_caps_free(imageData);
            return false;
        }

        PNG *decoder =
            new (decoderMemory) PNG();

        const int openResult =
            decoder->openRAM(
                imageData,
                static_cast<int>(imageSize),
                HandlePngDraw);

        if (openResult != PNG_SUCCESS)
        {
            decoder->~PNG();
            heap_caps_free(decoderMemory);
            heap_caps_free(imageData);
            return false;
        }

        const int width = decoder->getWidth();
        const int height = decoder->getHeight();

        if (width <= 0 ||
            width > 65535 ||
            height <= 0 ||
            height > 65535 ||
            !PrepareFit(
                context,
                static_cast<uint16_t>(width),
                static_cast<uint16_t>(height)))
        {
            decoder->close();
            decoder->~PNG();
            heap_caps_free(decoderMemory);
            heap_caps_free(imageData);
            return false;
        }

        context.pngLineBuffer =
            static_cast<uint16_t *>(
                heap_caps_malloc(
                    static_cast<size_t>(width) *
                        sizeof(uint16_t),
                    MALLOC_CAP_SPIRAM |
                        MALLOC_CAP_8BIT));

        if (context.pngLineBuffer == nullptr)
        {
            context.pngLineBuffer =
                static_cast<uint16_t *>(
                    heap_caps_malloc(
                        static_cast<size_t>(width) *
                            sizeof(uint16_t),
                        MALLOC_CAP_8BIT));
        }

        if (context.pngLineBuffer == nullptr)
        {
            decoder->close();
            decoder->~PNG();
            heap_caps_free(decoderMemory);
            heap_caps_free(imageData);
            return false;
        }

        context.pngLineCapacity =
            static_cast<uint16_t>(width);
        context.pngDecoder = decoder;

        const int decodeResult =
            decoder->decode(
                &context,
                0);

        context.pngDecoder = nullptr;

        heap_caps_free(
            context.pngLineBuffer);

        context.pngLineBuffer = nullptr;
        context.pngLineCapacity = 0;

        decoder->close();
        decoder->~PNG();

        heap_caps_free(decoderMemory);
        heap_caps_free(imageData);

        return decodeResult == PNG_SUCCESS;
    }

    int HandleJpegDraw(JPEGDRAW *draw)
    {
        if (draw == nullptr ||
            draw->pUser == nullptr ||
            draw->pPixels == nullptr ||
            draw->iWidth <= 0 ||
            draw->iHeight <= 0)
        {
            return 0;
        }

        RenderContext &context =
            *static_cast<RenderContext *>(
                draw->pUser);

        for (int blockY = 0;
             blockY < draw->iHeight;
             ++blockY)
        {
            const int sourceY =
                draw->y + blockY;

            if (sourceY < 0 ||
                sourceY >= context.sourceHeight)
            {
                continue;
            }

            for (int blockX = 0;
                 blockX < draw->iWidth;
                 ++blockX)
            {
                const int sourceX =
                    draw->x + blockX;

                if (sourceX < 0 ||
                    sourceX >= context.sourceWidth)
                {
                    continue;
                }

                const uint16_t pixel =
                    draw->pPixels[
                        blockY * draw->iWidth +
                        blockX];

                DrawMappedPixel(
                    context,
                    static_cast<uint16_t>(sourceX),
                    static_cast<uint16_t>(sourceY),
                    ConvertRgb565(pixel));
            }
        }

        return 1;
    }

    bool RenderJpeg(
        const char *imagePath,
        RenderContext &context)
    {
        size_t imageSize = 0;
        uint8_t *imageData =
            LoadCompressedImage(
                imagePath,
                imageSize);

        if (imageData == nullptr)
        {
            return false;
        }

        void *decoderMemory =
            heap_caps_malloc(
                sizeof(JPEGDEC),
                MALLOC_CAP_SPIRAM |
                    MALLOC_CAP_8BIT);

        if (decoderMemory == nullptr)
        {
            decoderMemory =
                heap_caps_malloc(
                    sizeof(JPEGDEC),
                    MALLOC_CAP_8BIT);
        }

        if (decoderMemory == nullptr)
        {
            heap_caps_free(imageData);
            return false;
        }

        JPEGDEC *decoder =
            new (decoderMemory) JPEGDEC();

        if (!decoder->openRAM(
                imageData,
                static_cast<int>(imageSize),
                HandleJpegDraw))
        {
            decoder->~JPEGDEC();
            heap_caps_free(decoderMemory);
            heap_caps_free(imageData);
            return false;
        }

        int sourceWidth = decoder->getWidth();
        int sourceHeight = decoder->getHeight();

        if (sourceWidth <= 0 ||
            sourceWidth > 65535 ||
            sourceHeight <= 0 ||
            sourceHeight > 65535)
        {
            decoder->close();
            decoder->~JPEGDEC();
            heap_caps_free(decoderMemory);
            heap_caps_free(imageData);
            return false;
        }

        // Use JPEGDEC's built-in power-of-two downscaling when the
        // source is substantially larger than the AMOLED viewport.
        int decodeOptions = 0;
        uint8_t decodeShift = 0;

        while (decodeShift < 3)
        {
            const int nextWidth =
                sourceWidth >> (decodeShift + 1U);
            const int nextHeight =
                sourceHeight >> (decodeShift + 1U);

            if (nextWidth < context.canvasWidth ||
                nextHeight < context.canvasHeight)
            {
                break;
            }

            ++decodeShift;
        }

        if (decodeShift == 1)
        {
            decodeOptions = JPEG_SCALE_HALF;
        }
        else if (decodeShift == 2)
        {
            decodeOptions = JPEG_SCALE_QUARTER;
        }
        else if (decodeShift == 3)
        {
            decodeOptions = JPEG_SCALE_EIGHTH;
        }

        const uint16_t decodedWidth =
            static_cast<uint16_t>(
                (sourceWidth +
                 ((1U << decodeShift) - 1U)) >>
                decodeShift);

        const uint16_t decodedHeight =
            static_cast<uint16_t>(
                (sourceHeight +
                 ((1U << decodeShift) - 1U)) >>
                decodeShift);

        if (!PrepareFit(
                context,
                decodedWidth,
                decodedHeight))
        {
            decoder->close();
            decoder->~JPEGDEC();
            heap_caps_free(decoderMemory);
            heap_caps_free(imageData);
            return false;
        }

        decoder->setPixelType(
            RGB565_LITTLE_ENDIAN);

        decoder->setUserPointer(
            &context);

        const int decodeResult =
            decoder->decode(
                0,
                0,
                decodeOptions);

        decoder->close();
        decoder->~JPEGDEC();

        heap_caps_free(decoderMemory);
        heap_caps_free(imageData);

        return decodeResult == 1;
    }

    uint16_t ReadLe16(const uint8_t *value)
    {
        return static_cast<uint16_t>(
            value[0] |
            (static_cast<uint16_t>(value[1]) << 8U));
    }

    uint32_t ReadLe32(const uint8_t *value)
    {
        return
            static_cast<uint32_t>(value[0]) |
            (static_cast<uint32_t>(value[1]) << 8U) |
            (static_cast<uint32_t>(value[2]) << 16U) |
            (static_cast<uint32_t>(value[3]) << 24U);
    }

    bool RenderBmp(
        const char *imagePath,
        RenderContext &context)
    {
        File file =
            SD.open(imagePath, FILE_READ);

        if (!file || file.isDirectory())
        {
            if (file)
            {
                file.close();
            }

            return false;
        }

        uint8_t header[54] = {};

        if (file.read(
                header,
                sizeof(header)) !=
            sizeof(header))
        {
            file.close();
            return false;
        }

        if (header[0] != 'B' ||
            header[1] != 'M')
        {
            file.close();
            return false;
        }

        const uint32_t pixelOffset =
            ReadLe32(&header[10]);

        const uint32_t dibHeaderSize =
            ReadLe32(&header[14]);

        if (dibHeaderSize < 40)
        {
            file.close();
            return false;
        }

        const int32_t signedWidth =
            static_cast<int32_t>(
                ReadLe32(&header[18]));

        const int32_t signedHeight =
            static_cast<int32_t>(
                ReadLe32(&header[22]));

        const uint16_t planes =
            ReadLe16(&header[26]);

        const uint16_t bitsPerPixel =
            ReadLe16(&header[28]);

        const uint32_t compression =
            ReadLe32(&header[30]);

        if (signedWidth <= 0 ||
            signedWidth > 65535 ||
            signedHeight == 0 ||
            signedHeight == INT32_MIN ||
            planes != 1 ||
            (bitsPerPixel != 24 &&
             bitsPerPixel != 32) ||
            compression != 0)
        {
            file.close();
            return false;
        }

        const uint32_t absoluteHeight =
            signedHeight < 0
                ? static_cast<uint32_t>(
                      -signedHeight)
                : static_cast<uint32_t>(
                      signedHeight);

        if (absoluteHeight > 65535)
        {
            file.close();
            return false;
        }

        const uint16_t sourceWidth =
            static_cast<uint16_t>(signedWidth);

        const uint16_t sourceHeight =
            static_cast<uint16_t>(absoluteHeight);

        if (!PrepareFit(
                context,
                sourceWidth,
                sourceHeight))
        {
            file.close();
            return false;
        }

        const uint8_t bytesPerPixel =
            static_cast<uint8_t>(
                bitsPerPixel / 8U);

        const uint32_t rowBytes =
            (static_cast<uint32_t>(sourceWidth) *
                 bytesPerPixel +
             3U) &
            ~3U;

        uint8_t *rowBuffer =
            static_cast<uint8_t *>(
                heap_caps_malloc(
                    rowBytes,
                    MALLOC_CAP_SPIRAM |
                        MALLOC_CAP_8BIT));

        if (rowBuffer == nullptr)
        {
            rowBuffer =
                static_cast<uint8_t *>(
                    heap_caps_malloc(
                        rowBytes,
                        MALLOC_CAP_8BIT));
        }

        if (rowBuffer == nullptr)
        {
            file.close();
            return false;
        }

        const bool topDown =
            signedHeight < 0;

        bool success = true;

        for (uint16_t sourceY = 0;
             sourceY < sourceHeight;
             ++sourceY)
        {
            const uint32_t fileRow =
                topDown
                    ? sourceY
                    : static_cast<uint32_t>(
                          sourceHeight - 1U -
                          sourceY);

            const uint32_t rowOffset =
                pixelOffset +
                fileRow * rowBytes;

            if (!file.seek(rowOffset) ||
                file.read(
                    rowBuffer,
                    rowBytes) != rowBytes)
            {
                success = false;
                break;
            }

            for (uint16_t sourceX = 0;
                 sourceX < sourceWidth;
                 ++sourceX)
            {
                const uint8_t *pixel =
                    &rowBuffer[
                        static_cast<size_t>(sourceX) *
                        bytesPerPixel];

                DrawMappedPixel(
                    context,
                    sourceX,
                    sourceY,
                    lv_color_make(
                        pixel[2],
                        pixel[1],
                        pixel[0]));
            }
        }

        heap_caps_free(rowBuffer);
        file.close();

        return success;
    }
}

char FloorPlanImageRenderer::lastError[96] = {};

bool FloorPlanImageRenderer::RenderFit(
    const char *imagePath,
    lv_color_t *canvasBuffer,
    uint16_t canvasWidth,
    uint16_t canvasHeight,
    FloorPlanRenderInfo &renderInfo)
{
    renderInfo = FloorPlanRenderInfo{};
    lastError[0] = '\0';

    if (imagePath == nullptr ||
        imagePath[0] == '\0' ||
        canvasBuffer == nullptr ||
        canvasWidth == 0 ||
        canvasHeight == 0)
    {
        std::strncpy(
            lastError,
            "Invalid Floor Plan render request",
            sizeof(lastError) - 1);
        return false;
    }

    ClearCanvas(
        canvasBuffer,
        canvasWidth,
        canvasHeight);

    RenderContext context{};
    context.canvas = canvasBuffer;
    context.canvasWidth = canvasWidth;
    context.canvasHeight = canvasHeight;

    const char *extension =
        GetExtension(imagePath);

    bool rendered = false;

    if (TextEqualsIgnoreCase(
            extension,
            ".png"))
    {
        rendered =
            RenderPng(
                imagePath,
                context);
    }
    else if (TextEqualsIgnoreCase(
                 extension,
                 ".jpg") ||
             TextEqualsIgnoreCase(
                 extension,
                 ".jpeg"))
    {
        rendered =
            RenderJpeg(
                imagePath,
                context);
    }
    else if (TextEqualsIgnoreCase(
                 extension,
                 ".bmp"))
    {
        rendered =
            RenderBmp(
                imagePath,
                context);
    }
    else
    {
        std::strncpy(
            lastError,
            "Unsupported Floor Plan image format",
            sizeof(lastError) - 1);
        return false;
    }

    if (!rendered)
    {
        if (lastError[0] == '\0')
        {
            std::strncpy(
                lastError,
                "Floor Plan image could not be decoded",
                sizeof(lastError) - 1);
        }

        return false;
    }

    renderInfo.available = true;
    renderInfo.sourceWidth =
        context.sourceWidth;
    renderInfo.sourceHeight =
        context.sourceHeight;
    renderInfo.renderedWidth =
        context.renderedWidth;
    renderInfo.renderedHeight =
        context.renderedHeight;
    renderInfo.offsetX =
        context.offsetX;
    renderInfo.offsetY =
        context.offsetY;

    Serial.printf(
        "FloorPlanImageRenderer: %s rendered %ux%u -> %ux%u at %u,%u\n",
        imagePath,
        static_cast<unsigned int>(
            renderInfo.sourceWidth),
        static_cast<unsigned int>(
            renderInfo.sourceHeight),
        static_cast<unsigned int>(
            renderInfo.renderedWidth),
        static_cast<unsigned int>(
            renderInfo.renderedHeight),
        static_cast<unsigned int>(
            renderInfo.offsetX),
        static_cast<unsigned int>(
            renderInfo.offsetY));

    return true;
}

const char *FloorPlanImageRenderer::GetLastError()
{
    return lastError;
}
