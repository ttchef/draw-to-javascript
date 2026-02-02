
#include "common.h"

#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "clay_renderer_raylib.c"

typedef struct Context Context;

// COLORS 
const Clay_Color UI_COLOR_LIGHT_GRAY = (Clay_Color){120, 120, 120, 255};
const Clay_Color UI_COLOR_DARK_GRAY = (Clay_Color){80, 80, 80, 255};
const Clay_Color UI_COLOR_DARK_DARK_GRAY = (Clay_Color){60, 60, 60, 255};
const Clay_Color UI_COLOR_DARK_DARK_DARK_GRAY = (Clay_Color){40, 40, 40, 255};
const Clay_Color UI_COLOR_BLACK = (Clay_Color){0, 0, 0, 255};
const Clay_Color UI_COLOR_WHITE = (Clay_Color){255, 255, 255, 255};
const Clay_Color UI_COLOR_RED = (Clay_Color){255, 0, 0, 255};

typedef void (*PFN_onHover)(Clay_ElementId, Clay_PointerData, void* userData);

// Custom UI elements
const CustomLayoutElement_RectangleLines rect = {
    .borderColor = UI_COLOR_WHITE,
};
CustomLayoutElement custom_element = {
    .type = CUSTOM_LAYOUT_ELEMENT_TYPE_RECTANGLE_LINES,
    .customData.rect = rect,
};

void handle_clay_errors(Clay_ErrorData error_data) {
    fprintf(stderr, "[CLAY_ERROR]: %s\n", error_data.errorText.chars);
}     

void clay_utilities_button(Clay_String button_text, Texture2D* image) {
    /* Outer Container for push to middle vertically */
    CLAY_AUTO_ID({
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_PERCENT(1.0f) },
            .childAlignment = CLAY_ALIGN_X_CENTER,
         },
        .cornerRadius = CLAY_CORNER_RADIUS(12),
        .backgroundColor = Clay_Hovered() ? UI_COLOR_LIGHT_GRAY : UI_COLOR_DARK_GRAY,
    }) {
        /* Vertical push padding */
        CLAY_AUTO_ID({ .layout = { .sizing = CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(20) }});
        /* Actual button */
        CLAY_AUTO_ID({
            .layout = {
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                .childGap = 10,
            },
        }) {
            CLAY_TEXT(button_text, CLAY_TEXT_CONFIG({
                .fontId = 0,
                .fontSize = 20,
                .textColor = UI_COLOR_WHITE,
            }));
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = { CLAY_SIZING_FIXED(20), CLAY_SIZING_FIXED(20) },
                 },
                 .image = {
                    .imageData = image,
                 },
            });
        }
    }
}

void compute_clay_utilities_dropdown_menu_item(Clay_String text, PFN_onHover on_hover_func, void* user_data) {
   CLAY_AUTO_ID({
        .layout = {
            .padding = CLAY_PADDING_ALL(16),
            .sizing = { .width = CLAY_SIZING_GROW(0) },
        },
        .backgroundColor = Clay_Hovered() ? UI_COLOR_DARK_GRAY : UI_COLOR_DARK_DARK_GRAY,
        .cornerRadius = CLAY_CORNER_RADIUS(12),
    }) {
        Clay_OnHover(on_hover_func, user_data);
        CLAY_TEXT(text, CLAY_TEXT_CONFIG({
            .fontId = 0,
            .fontSize = 20,
            .textColor = { 255, 255, 255, 255 }
        }));
    }
}

void utilities_new_dropdown_item_on_hover(Clay_ElementId element_id, Clay_PointerData pointer_info, void* user_data) {
    uiState* state = &((Context*)user_data)->ui_state;
    if (pointer_info.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        state->new_image_menu = true;
    }
}

void utilities_open_image_dropdown_item_on_hover(Clay_ElementId element_id, Clay_PointerData pointer_info, void* user_data) {
    if (pointer_info.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        printf("Open Image clicked!\n");
    }
}

void utilities_open_javascript_dropdown_item_on_hover(Clay_ElementId element_id, Clay_PointerData pointer_info, void* user_data) {
    if (pointer_info.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        printf("Open Javascript clicked!\n");
    }
}

void clay_number_input_box(Clay_String text) {
    CLAY_AUTO_ID({
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
            .padding = CLAY_PADDING_ALL(12),
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
            .childGap = 6,
        },
        .backgroundColor = UI_COLOR_DARK_GRAY,
        .cornerRadius = CLAY_CORNER_RADIUS(12),
    }) {
        CLAY_AUTO_ID({
            .layout = {
                .sizing = { CLAY_SIZING_FIXED(75), CLAY_SIZING_FIXED(75) },
            },
            .backgroundColor = UI_COLOR_RED,
            .cornerRadius = CLAY_CORNER_RADIUS(12),
        });
        CLAY_TEXT(text, CLAY_TEXT_CONFIG({
            .fontId = 0,
            .fontSize = 20,
            .textColor = UI_COLOR_WHITE,
        }));
    }
}

void clay_image_menu_button(Clay_String button_text) {
    CLAY_AUTO_ID({
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = { CLAY_SIZING_FIXED(110), CLAY_SIZING_FIXED(50) },
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
            .padding = CLAY_PADDING_ALL(12),
        },
        .backgroundColor = Clay_Hovered() ? UI_COLOR_LIGHT_GRAY : UI_COLOR_DARK_GRAY,
        .cornerRadius = CLAY_CORNER_RADIUS(12),
    }) {
        CLAY_TEXT(button_text, CLAY_TEXT_CONFIG({
            .fontId = 0,
            .fontSize = 20,
            .textColor = UI_COLOR_WHITE,
        }));
    }
}

void compute_clay_new_image_menu(Context* ctx) {
    CLAY(CLAY_ID("new_image_menu"), {
        .floating = {
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .offset = {
                .x = ctx->window_width / 2 - 400,
                .y = ctx->window_height / 2 - 225,
            },
        },
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = { CLAY_SIZING_FIXED(800), CLAY_SIZING_FIXED(450) },
            .padding = CLAY_PADDING_ALL(12),
        },
        .backgroundColor = UI_COLOR_DARK_DARK_DARK_GRAY,
        .cornerRadius = CLAY_CORNER_RADIUS(12),
    }) {
        /* "Header" */
        CLAY_AUTO_ID({
            .layout = { 
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                .sizing = { CLAY_SIZING_PERCENT(1.0f), CLAY_SIZING_PERCENT(0.15f) },
                .padding = CLAY_PADDING_ALL(12),
            },
            .backgroundColor = UI_COLOR_DARK_DARK_GRAY,
            .cornerRadius = CLAY_CORNER_RADIUS(12),
        }) {
            CLAY_TEXT(CLAY_STRING("New Image"), CLAY_TEXT_CONFIG({
                .fontId = 1,
                .fontSize = 40,
                .textColor = UI_COLOR_WHITE,
            }));
        }

        CLAY_AUTO_ID({
            .layout = {
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .padding = { 0, 0, 50, 0 },
                .childAlignment = CLAY_ALIGN_X_CENTER,
                .childGap = 200,
            },
        }) {
            clay_number_input_box(CLAY_STRING("Width"));
            clay_number_input_box(CLAY_STRING("Height"));
        }

        CLAY_AUTO_ID({
            .layout = {
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_PERCENT(0.2f) },
                .padding = { 0, 0, 50, 0 },
                .childAlignment = CLAY_ALIGN_X_CENTER,
                .childGap = 200,
            },
        }) {
            clay_image_menu_button(CLAY_STRING("Close"));
            clay_image_menu_button(CLAY_STRING("Create"));
        }
    }
}

void compute_clay_utilities(Context* ctx, Texture2D* textures, size_t image_count) {
    CLAY(CLAY_ID("utilities"), {
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = { CLAY_SIZING_PERCENT(0.33f), CLAY_SIZING_PERCENT(1.0f) }, 
            .padding = CLAY_PADDING_ALL(12),
            .childGap = 12,
        },
        .backgroundColor = UI_COLOR_DARK_GRAY,
        .cornerRadius = { 12, 12, 12, 12 },
    }) {
        CLAY(CLAY_ID("utilities_file_button"), {
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            },
        }) {
            clay_utilities_button(CLAY_STRING("File"), &textures[0]);
        }
        CLAY(CLAY_ID("utilities_export_button"), {
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            },
        }) {
            clay_utilities_button(CLAY_STRING("Export"), &textures[1]);
        }
        CLAY(CLAY_ID("utilities_save_button"), {
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            },
        }) {
            clay_utilities_button(CLAY_STRING("Save"), &textures[2]);
        }

        /* File Menu */ 
        if ((IsMouseButtonDown(MOUSE_BUTTON_LEFT) && Clay_PointerOver(Clay_GetElementId(CLAY_STRING("utilities_file_button")))) ||
            Clay_PointerOver(Clay_GetElementId(CLAY_STRING("utilities_file_menu")))) {
            CLAY(CLAY_ID("utilities_file_menu"), {
                .floating = {
                    .attachTo = CLAY_ATTACH_TO_PARENT,
                    .attachPoints = {
                        .parent = CLAY_ATTACH_POINT_LEFT_TOP,
                    },
                },
                .layout = {
                    .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                    .padding = { 0, 0, 60, 60 },
                },
            }) {
                CLAY_AUTO_ID({
                    .layout = {
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        .sizing = { CLAY_SIZING_FIXED(200) },
                        .padding = CLAY_PADDING_ALL(8),
                        .childGap = 8,
                    },
                    .backgroundColor = UI_COLOR_DARK_DARK_DARK_GRAY,
                    .cornerRadius = CLAY_CORNER_RADIUS(12),
                }) {
                    compute_clay_utilities_dropdown_menu_item(CLAY_STRING("New"),
                            utilities_new_dropdown_item_on_hover, ctx);
                    compute_clay_utilities_dropdown_menu_item(CLAY_STRING("Open Image"),
                            utilities_open_image_dropdown_item_on_hover, NULL);
                    compute_clay_utilities_dropdown_menu_item(CLAY_STRING("Open Javascript"),
                            utilities_open_javascript_dropdown_item_on_hover, NULL);
                }
            }
        }

        /* New Image Menu? */
        if (ctx->ui_state.new_image_menu) {
            compute_clay_new_image_menu(ctx);
        }
    }
}

void compute_clay_tools(Context* ctx, Texture2D* textures, size_t image_count) {
    CLAY(CLAY_ID("tools"), {
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = { CLAY_SIZING_PERCENT(0.33f), CLAY_SIZING_PERCENT(1.0f) }, 
        },
        .backgroundColor = UI_COLOR_DARK_GRAY,
        .cornerRadius = { 12, 12, 12, 12 },
    }) {

    }
}

void compute_clay_tools_settings(Context* ctx, Texture2D* textures, size_t image_count) {
    CLAY(CLAY_ID("tool_settings"), {
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = { CLAY_SIZING_PERCENT(0.33f), CLAY_SIZING_PERCENT(1.0f) }, 
        },
        .backgroundColor = UI_COLOR_DARK_GRAY,
        .cornerRadius = { 12, 12, 12, 12 },
    }) {
         
    }
}

void compute_clay_topbar(Context* ctx, Texture2D* textures, size_t image_count) {
    Clay_Sizing top_bar_size = { CLAY_SIZING_PERCENT(1.0f), CLAY_SIZING_FIXED(100) };

    CLAY(CLAY_ID("top_bar"), {
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = top_bar_size,
            .padding = CLAY_PADDING_ALL(10),
            .childGap = 16,
            .childAlignment = CLAY_ALIGN_X_CENTER,
        },
        .backgroundColor = UI_COLOR_DARK_DARK_GRAY,
        .cornerRadius = { 12, 12, 12, 12 },
    }) {
        compute_clay_utilities(ctx, textures, image_count);
        compute_clay_tools(ctx, textures, image_count);
        compute_clay_tools_settings(ctx, textures, image_count);
    }
}

void compute_clay_bottom(Context* ctx) {
    CLAY(CLAY_ID("bottom_part"), {
        .layout = { 
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = { CLAY_SIZING_PERCENT(1.0f), CLAY_SIZING_GROW(0) },
            .padding = CLAY_PADDING_ALL(0),
            .childGap = 32,
         },
         .backgroundColor = UI_COLOR_DARK_DARK_GRAY,
         .cornerRadius = { 12, 12, 12, 12 },
    }) {
    
    }
}

// because of headers
void compute_clay_layout(struct Context* ctx, Texture2D* textures, size_t images_count) {
    Clay_BeginLayout();

    /* Window Container */
    CLAY(CLAY_ID("outer_container"), {
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .padding = CLAY_PADDING_ALL(12),
            .childGap = 12,
        },
        .backgroundColor = UI_COLOR_BLACK,
    }) {
        compute_clay_topbar(ctx, textures, images_count);
        compute_clay_bottom(ctx);
    }
}

