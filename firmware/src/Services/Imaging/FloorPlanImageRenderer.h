#pragma once

#include <stdint.h>
#include <lvgl.h>

struct FloorPlanRenderInfo
{
    bool available = false;

    uint16_t sourceWidth = 0;
    uint16_t sourceHeight = 0;

    uint16_t renderedWidth = 0;
    uint16_t renderedHeight = 0;

    uint16_t offsetX = 0;
    uint16_t offsetY = 0;
};

class FloorPlanImageRenderer
{
public:
    // Decodes a registered Floor Plan image from SD storage into an
    // RGB565 LVGL canvas buffer. The image is fitted to the canvas while
    // preserving aspect ratio and is never enlarged beyond native size.
    // Supported viewer formats: PNG, JPG/JPEG, and uncompressed 24/32-bit BMP.
    static bool RenderFit(
        const char *imagePath,
        lv_color_t *canvasBuffer,
        uint16_t canvasWidth,
        uint16_t canvasHeight,
        FloorPlanRenderInfo &renderInfo);

    static const char *GetLastError();

private:
    static char lastError[96];
};
