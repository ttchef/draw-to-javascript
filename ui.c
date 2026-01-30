
#include "ui.h"
#include "raylib.h"

#include <string.h>
#include <stdio.h>

uiInfo _ui_global_info = {0};
int32_t _ui_global_current_y = 0;

static Rectangle _uiGetRectangleInput(Rectangle* bounds) {
    Rectangle rect = {0};
    if (bounds == NULL) {
        rect = uiGetCurrentRect();
    }
    else {
        rect = *bounds;
    }
    return rect;
}

void uiInfoAddElement(int32_t height) {
    _ui_global_current_y += height + _ui_global_info.padding_y;
}


Rectangle uiGetCurrentRect() {
    return (Rectangle){
        .x = _ui_global_info.start_x + _ui_global_info.padding_x,
        .y = _ui_global_current_y,
        .width = _ui_global_info.width - 2 * _ui_global_info.padding_x,
        .height = _ui_global_info.element_height,
    };
}

void uiBegin(uiInfo *info) {
    memcpy(&_ui_global_info, info, sizeof(uiInfo));   
    _ui_global_current_y = _ui_global_info.start_y + _ui_global_info.padding_y;
}

void uiEnd(uiInfo *info) {
    memset(&_ui_global_info, 0, sizeof(uiInfo));
    _ui_global_current_y = 0;
}

int32_t uiButton(Rectangle* bounds, const uint8_t *text)  {
    Rectangle rect = _uiGetRectangleInput(bounds);
    uiInfoAddElement(rect.height);
    return GuiButton(rect, text);
}

int32_t uiButtonEx(Rectangle* bounds, const uint8_t *text, bool padding_y)  {
    Rectangle rect = _uiGetRectangleInput(bounds);
    if (padding_y) {
        uiInfoAddElement(rect.height);
    }
    return GuiButton(rect, text);
}

int32_t uiSlider(Rectangle *bounds, const uint8_t *text_left, const uint8_t *text_right, float *val, float min_val, float max_val) {
    Rectangle rect = _uiGetRectangleInput(bounds);
    uiInfoAddElement(rect.height);
    return GuiSlider(rect, text_left, text_right, val, min_val, max_val);
}

int32_t uiCheckBox(Rectangle *bounds, const uint8_t *text, bool *checked) {
    Rectangle rect = _uiGetRectangleInput(bounds);
    uiInfoAddElement(rect.height);
    return GuiCheckBox(rect, text, checked);
}

int32_t uiColorPicker(Rectangle *bounds, const uint8_t *text, Color *color) {
    Rectangle rect = _uiGetRectangleInput(bounds);
    uiInfoAddElement(rect.height);
    return GuiColorPicker(rect, text, color);
}

int32_t uiTextBox(Rectangle *bounds, uint8_t *text, int32_t text_size, bool edit_mode) {
    Rectangle rect = _uiGetRectangleInput(bounds);
    uiInfoAddElement(rect.height);
    return GuiTextBox(rect, text, text_size, edit_mode);
}

void uiText(FontInfo *info, const uint8_t *text, Color color) {
    FontInfo font_info = {0};
    if (!info) {
        Rectangle rect = _uiGetRectangleInput(NULL);
        font_info.x = rect.x;
        font_info.y = rect.y;
        font_info.size = rect.height;

        int32_t font_width = MeasureText(text, font_info.size);
        while (font_width > (_ui_global_info.width - 2 * _ui_global_info.padding_x)) {
            font_info.size--;
            font_width = MeasureText(text, font_info.size);
        }
    }
    else {
        font_info = *info;
    }
    uiInfoAddElement(font_info.size);
    DrawText(text, font_info.x, font_info.y, font_info.size, color);
}

void uiTextEx(FontInfo *info, const uint8_t *text, Color color, bool padding_y) {
    FontInfo font_info = {0};
    if (!info) {
        Rectangle rect = _uiGetRectangleInput(NULL);
        font_info.x = rect.x;
        font_info.y = rect.y;
        font_info.size = rect.height;

        int32_t font_width = MeasureText(text, font_info.size);
        while (font_width > (_ui_global_info.width - 2 * _ui_global_info.padding_x)) {
            font_info.size--;
            font_width = MeasureText(text, font_info.size);
        }
    }
    else {
        font_info = *info;
    }

    if (!padding_y) {
        _ui_global_current_y += font_info.size;
    }
    else {
        uiInfoAddElement(font_info.size);
    }

    DrawText(text, font_info.x, font_info.y, font_info.size, color);
}


