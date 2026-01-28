
#ifndef UI_H
#define UI_H

#include <stdint.h>
#include <stdbool.h>

#include <raylib.h>

#ifndef RAYGUI_H
#include "raygui.h"
#endif

typedef struct uiInfo {
    int32_t start_x;
    int32_t start_y;
    int32_t width;
    int32_t height;
    int32_t padding_x;
    int32_t padding_y;
    int32_t element_height;
} uiInfo;

// i know global state bad 
extern uiInfo _ui_global_info;
extern int32_t _ui_global_current_y;

void uiBegin(uiInfo* info);
void uiEnd(uiInfo* info);

/* Baiscally wrappers for raygui tho with bounds customisation */
int32_t uiButton(Rectangle* bounds, const uint8_t* text);
int32_t uiButtonEx(Rectangle* bounds, const uint8_t* text, bool padding_y);
int32_t uiSlider(Rectangle* bounds, const uint8_t* text_left, const uint8_t* text_right, float* val, float min_val, float max_val);
int32_t uiCheckBox(Rectangle* bounds, const uint8_t* text, bool* checked);

Rectangle uiGetCurrentRect();

static inline uiInfo* uiGetCurrentInfo() { return &_ui_global_info; }
static inline int32_t* uiGetCurrentY() { return &_ui_global_current_y; }
void uiInfoAddElement(int32_t height);

/*
    modifies bounds so the overall slider (with the text) 
    is the input bounds width
*/
static inline void uiGetSliderBounds(Rectangle* bounds, const uint8_t* text) {
    int32_t font_size = GuiGetStyle(DEFAULT, TEXT_SIZE);
    int32_t font_width = MeasureText(text, font_size);

    bounds->x += font_width;
    bounds->width -= font_width;
}

#endif // UI_H
