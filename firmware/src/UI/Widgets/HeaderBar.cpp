#include "HeaderBar.h"
#include "../Theme.h"
#include "../../Services/Power/PowerService.h"

#include <Arduino.h>
#include <lvgl.h>

namespace
{
constexpr lv_coord_t HeaderTop = 8;
constexpr lv_coord_t HorizontalMargin = 8;
constexpr lv_coord_t HeaderLabelHeight = 22;

// Each label is positioned independently. The page title is anchored to
// the physical centre of the screen instead of being placed between two
// unequal side columns.
constexpr lv_coord_t BrandWidth = 92;
constexpr lv_coord_t PageWidth = 88;
constexpr lv_coord_t PowerWidth = 78;

const char *GetPageTitle(ScreenID screen)
{
    switch (screen)
    {
        case ScreenID::Dashboard:
            return "Dashboard";

        case ScreenID::Scan:
            return "Wi-Fi Scan";

        case ScreenID::Tools:
            return "Tools";

        case ScreenID::Settings:
            return "Settings";

        default:
            return "Dashboard";
    }
}

void ConfigureSingleLineLabel(
    lv_obj_t *label,
    lv_coord_t width,
    const lv_font_t *font,
    lv_color_t colour,
    lv_text_align_t alignment)
{
    // A fixed one-line height is essential. LV_SIZE_CONTENT allowed LVGL
    // to increase the label height and wrap text onto a second line.
    lv_obj_set_size(label, width, HeaderLabelHeight);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, colour, 0);
    lv_obj_set_style_text_align(label, alignment, 0);
}
}

void HeaderBar::Show(ScreenID current)
{
    lv_obj_t *screen = lv_scr_act();

    brandLabel = lv_label_create(screen);
    lv_label_set_text(brandLabel, "SentinelOS");
    ConfigureSingleLineLabel(
        brandLabel,
        BrandWidth,
        &lv_font_montserrat_16,
        Theme::Accent(),
        LV_TEXT_ALIGN_LEFT);
    lv_obj_align(
        brandLabel,
        LV_ALIGN_TOP_LEFT,
        HorizontalMargin,
        HeaderTop);

    pageLabel = lv_label_create(screen);
    ConfigureSingleLineLabel(
        pageLabel,
        PageWidth,
        &lv_font_montserrat_14,
        Theme::Text(),
        LV_TEXT_ALIGN_CENTER);
    lv_obj_align(
        pageLabel,
        LV_ALIGN_TOP_MID,
        0,
        HeaderTop + 1);

    powerLabel = lv_label_create(screen);
    ConfigureSingleLineLabel(
        powerLabel,
        PowerWidth,
        &lv_font_montserrat_12,
        Theme::Text(),
        LV_TEXT_ALIGN_RIGHT);
    lv_obj_align(
        powerLabel,
        LV_ALIGN_TOP_RIGHT,
        -HorizontalMargin,
        HeaderTop + 2);

    SetCurrent(current);
    Update();
}

void HeaderBar::SetCurrent(ScreenID current)
{
    if (pageLabel != nullptr)
    {
        lv_label_set_text(pageLabel, GetPageTitle(current));
    }
}

void HeaderBar::Update()
{
    if (powerLabel == nullptr)
    {
        return;
    }

    char buffer[24];

    PowerService::FormatStatus(
        buffer,
        sizeof(buffer));

    lv_label_set_text(powerLabel, buffer);
}
