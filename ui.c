
#include "common.h"

#include <string.h>
#include <assert.h>

#include <raylib.h>

#include "libtinyfiledialogs/tinyfiledialogs.h"

#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "clay_renderer_raylib.c"

#include "stb_image.h"
#include "stb_image_write.h"

#define WINDOW_SIZE_THRESHOLD_TOP_BAR_SNAPPING 1050
#define TOP_BAR_SNAP_ANIM_TIME 0.8f /* In seconds */

typedef struct Context Context;

// COLORS 
const Clay_Color UI_COLOR_LIGHT_GRAY = (Clay_Color){120, 120, 120, 255};
const Clay_Color UI_COLOR_DARK_GRAY = (Clay_Color){80, 80, 80, 255};
const Clay_Color UI_COLOR_DARK_DARK_GRAY = (Clay_Color){60, 60, 60, 255};
const Clay_Color UI_COLOR_DARK_DARK_DARK_GRAY = (Clay_Color){40, 40, 40, 255};
const Clay_Color UI_COLOR_BLACK = (Clay_Color){0, 0, 0, 255};
const Clay_Color UI_COLOR_WHITE = (Clay_Color){255, 255, 255, 255};
const Clay_Color UI_COLOR_RED = (Clay_Color){255, 0, 0, 255};
const Clay_Color UI_COLOR_LIGHT_BLUE = (Clay_Color){84, 145, 244, 125};

typedef void (*PFN_onHover)(Clay_ElementId, Clay_PointerData, void* userData);

/* Custom Types */
CustomLayoutElement input_box_color;
CustomLayoutElement custom_circle[BRUSH_COLORS_COUNT];
CustomLayoutElement color_picker;

void handle_clay_errors(Clay_ErrorData error_data) {
    fprintf(stderr, "[CLAY_ERROR]: %s\n", error_data.errorText.chars);
}   

void check_input_number(char* number_string) {
    if (strlen(number_string) != UI_MAX_INPUT_CHARACTERS) return;
    if (number_string[0] >= '4') {
        number_string[0] = '4';
        for (int32_t i = 1; i < UI_MAX_INPUT_CHARACTERS; i++) {
            number_string[i] = '0';
        }
    }
}

void initzialize_img_alpha(Context* ctx) {
    for (int32_t y = 0; y < ctx->new_image_height; y++) {
        for (int32_t x = 0; x < ctx->new_image_width; x++) {
            int32_t index = (y * ctx->new_image_width + x) * 4;
            ctx->image_data[index + 3] = 255; 
        }
    }
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
    Context* ctx = (Context*)user_data;
    uiState* state = &ctx->ui_state;
    if (pointer_info.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        state->new_image_menu = true;
        state->image_menu_pos.x = ctx->window_width / 2 - 400;
        state->image_menu_pos.y = ctx->window_height / 2 - 225;
    }
}

void utilities_open_image_dropdown_item_on_hover(Clay_ElementId element_id, Clay_PointerData pointer_info, void* user_data) {
    Context* ctx = (Context*)user_data;
    if (pointer_info.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        const char* filters[] = { "*.png", "*.jpg", "*.jpeg" };
        const char* path = tinyfd_openFileDialog(
                "Open Image",
                "",
                ARRAY_LEN(filters),
                filters,
                "Images",
                0);
        if (!path) {
            fprintf(stderr, "Failed to get path from file dialog!\n");
            return;
        }

        int32_t channels;
        ctx->image_data = stbi_load(path, &ctx->new_image_width, &ctx->new_image_height, &channels, 4);
        if (!ctx->image_data) {
            fprintf(stderr, "Failed to create image in memory\n");
            return;
        }
        initzialize_img_alpha(ctx);

        Image img = GenImageColor(ctx->new_image_width, ctx->new_image_height, BLACK);
        ctx->loaded_tex = LoadTextureFromImage(img);
        UnloadImage(img);

        SetTextureFilter(ctx->loaded_tex, TEXTURE_FILTER_POINT);
        UpdateTexture(ctx->loaded_tex, ctx->image_data);

        ctx->loaded_ratio = (float)ctx->loaded_tex.width / (float)ctx->loaded_tex.height;
        ctx->mode = UI_MODE_IMAGE_EDITING;
    }
}

void utilities_open_javascript_dropdown_item_on_hover(Clay_ElementId element_id, Clay_PointerData pointer_info, void* user_data) {
    Context* ctx = (Context*)user_data;
    if (pointer_info.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        load_from_javascript(ctx);
    }
}

void utilities_export_image_dropdown_item_on_hover(Clay_ElementId element_id, Clay_PointerData pointer_info, void* user_data) {
    Context* ctx = (Context*)user_data;
    if (pointer_info.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        const char* filters[] = { "*.png" };
        const char* path = tinyfd_saveFileDialog(
                "",
                "",
                ARRAY_LEN(filters),
                filters,
                "Image Files");
        if (!path) {
            fprintf(stderr, "Failed to path from filedialog\n");
            return;
        }
        stbi_write_png(path, ctx->new_image_width, ctx->new_image_height, 4, ctx->image_data, ctx->new_image_width * 4);
    }
}

void utilities_export_javascript_dropdown_item_on_hover(Clay_ElementId element_id, Clay_PointerData pointer_info, void* user_data) {
    Context* ctx = (Context*)user_data;
    if (pointer_info.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        const char* filters[] = { "*.txt", "*.js" };
        const char* path = tinyfd_saveFileDialog(
                "",
                "",
                ARRAY_LEN(filters),
                filters,
                "Text File");
        if (!path) {
            fprintf(stderr, "Failed to path from filedialog\n");
            return;
        }
        FILE* file = fopen(path, "wb");
        if (!file) {
            fprintf(stderr, "Failed to open file: %s\n", path);
            return;
        }
        image_to_javascript(ctx, file, "rx", "ry"); // TODO: make variable names customizable
        fclose(file);
    }
}

void clay_number_input_box(Clay_String text, Clay_String dynmaic_text, Clay_Color* select_color) {
    CustomLayoutElement_RectangleLines custom_rect = {
        .borderColor = UI_COLOR_WHITE,
    };

    input_box_color.type = CUSTOM_LAYOUT_ELEMENT_TYPE_RECTANGLE_LINES;
    input_box_color.customData.rect = custom_rect;

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
            .cornerRadius = CLAY_CORNER_RADIUS(12),
            .custom = {
                .customData = &input_box_color,
            },
        }) {
            Clay_Color background_color;
            if (select_color) background_color = *select_color;
            else if (Clay_Hovered()) background_color = UI_COLOR_LIGHT_GRAY;
            else background_color = UI_COLOR_DARK_GRAY;

            CLAY_AUTO_ID({
                .layout = {
                    .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                },
                .backgroundColor = background_color,
                .cornerRadius = CLAY_CORNER_RADIUS(12),
            }) {
                CLAY_TEXT(dynmaic_text, CLAY_TEXT_CONFIG({
                    .fontId = 0,
                    .fontSize = 20,
                    .textColor = UI_COLOR_WHITE,
                }));
            }
        }
        CLAY_TEXT(text, CLAY_TEXT_CONFIG({
            .fontId = 0,
            .fontSize = 20,
            .textColor = UI_COLOR_WHITE,
        }));
    }
}

void image_menu_close_on_hover(Clay_ElementId element_id, Clay_PointerData pointer_info, void* user_data) {
    uiState* state = &((Context*)user_data)->ui_state;
    if (pointer_info.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        state->new_image_menu = false;
    }
}

void image_menu_create_on_hover(Clay_ElementId element_id, Clay_PointerData pointer_info, void* user_data) {
    Context* ctx = (Context*)user_data;
    if (pointer_info.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        check_input_number(ctx->ui_state.image_width);
        check_input_number(ctx->ui_state.image_height);
        ctx->ui_state.image_menu_width_input = false;
        ctx->ui_state.image_menu_height_input = false;

        ctx->new_image_width = atoi(ctx->ui_state.image_width);
        ctx->new_image_height = atoi(ctx->ui_state.image_height);

        if (ctx->new_image_width == 0 || ctx->new_image_height == 0) return;

        ctx->image_data = malloc(ctx->new_image_width * ctx->new_image_height * 4);
        if (!ctx->image_data) {
            fprintf(stderr, "Memory allocation failed\n");
            return;
        }

        memset(ctx->image_data, 0, ctx->new_image_width * ctx->new_image_height * 4);
        initzialize_img_alpha(ctx);
        Image img = GenImageColor(ctx->new_image_width, ctx->new_image_height, BLACK);
        ctx->loaded_tex = LoadTextureFromImage(img);
        UnloadImage(img);

        SetTextureFilter(ctx->loaded_tex, TEXTURE_FILTER_POINT);
        UpdateTexture(ctx->loaded_tex, ctx->image_data);

        ctx->loaded_ratio = (float)ctx->loaded_tex.width / (float)ctx->loaded_tex.height;
        ctx->mode = UI_MODE_IMAGE_EDITING;
        ctx->ui_state.new_image_menu = false;
    }
}

void clay_image_menu_button(Clay_String button_text, PFN_onHover on_hover_func, void* user_data) {
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
        Clay_OnHover(on_hover_func, user_data);
        CLAY_TEXT(button_text, CLAY_TEXT_CONFIG({
            .fontId = 0,
            .fontSize = 20,
            .textColor = UI_COLOR_WHITE,
        }));
    }
}

void new_image_menu_on_hover(Clay_ElementId element_id, Clay_PointerData pointer_info, void* user_data) {
    Context* ctx = (Context*)user_data;
    if (pointer_info.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        ctx->ui_state.image_menu_floating = true;
    }
}

void compute_clay_new_image_menu(Context* ctx) {
    CLAY(CLAY_ID("new_image_menu"), {
        .floating = {
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .offset = {
                .x = ctx->ui_state.image_menu_pos.x,
                .y = ctx->ui_state.image_menu_pos.y,
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
            Clay_OnHover(new_image_menu_on_hover, ctx);
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
            Clay_String dym_string = {
                .chars = ctx->ui_state.image_width,
                .length = strlen(ctx->ui_state.image_width),
                .isStaticallyAllocated = true,
            };
            
            /* Only for ID */
            CLAY(CLAY_ID("width_number_input"), {
                .layout = {
                    .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                },
            }) {
                Clay_Color* select_color = NULL;
                if (ctx->ui_state.image_menu_width_input) select_color = (Clay_Color*)&UI_COLOR_LIGHT_BLUE;
                clay_number_input_box(CLAY_STRING("Width"), dym_string, select_color);
            }

            dym_string.chars = ctx->ui_state.image_height;
            dym_string.length = strlen(ctx->ui_state.image_height);

            /* Only for ID */
            CLAY(CLAY_ID("height_number_input"), {
                .layout = {
                    .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) },
                },
            }) {
                Clay_Color* select_color = NULL;
                if (ctx->ui_state.image_menu_height_input) select_color = (Clay_Color*)&UI_COLOR_LIGHT_BLUE;
                clay_number_input_box(CLAY_STRING("Height"), dym_string, select_color);
            }

            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                ctx->ui_state.image_menu_width_input = false;
                check_input_number(ctx->ui_state.image_width);

                if (Clay_PointerOver(Clay_GetElementId(CLAY_STRING("width_number_input")))) {
                    ctx->ui_state.image_menu_width_input = true;
                }
            }

            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                ctx->ui_state.image_menu_height_input = false;
                check_input_number(ctx->ui_state.image_height);

                if (Clay_PointerOver(Clay_GetElementId(CLAY_STRING("height_number_input")))) {
                    ctx->ui_state.image_menu_height_input = true;
                }
            }
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
            clay_image_menu_button(CLAY_STRING("Close"), image_menu_close_on_hover, ctx);
            clay_image_menu_button(CLAY_STRING("Create"), image_menu_create_on_hover, ctx);
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
                            utilities_open_image_dropdown_item_on_hover, ctx);
                    compute_clay_utilities_dropdown_menu_item(CLAY_STRING("Open Javascript"),
                            utilities_open_javascript_dropdown_item_on_hover, ctx);
                }
            }
        }

        /* Export menu */
        if ((IsMouseButtonDown(MOUSE_BUTTON_LEFT) && Clay_PointerOver(Clay_GetElementId(CLAY_STRING("utilities_export_button")))) ||
            Clay_PointerOver(Clay_GetElementId(CLAY_STRING("utilities_export_menu")))) {
            CLAY(CLAY_ID("utilities_export_menu"), {
                .floating = {
                    .attachTo = CLAY_ATTACH_TO_PARENT,
                    .attachPoints = {
                        .parent = CLAY_ATTACH_POINT_LEFT_TOP,
                    },
                    .offset = { .x = 100 },
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
                    compute_clay_utilities_dropdown_menu_item(CLAY_STRING("Export Image"), 
                            utilities_export_image_dropdown_item_on_hover, ctx);
                    compute_clay_utilities_dropdown_menu_item(CLAY_STRING("Export Javascript"), 
                            utilities_export_javascript_dropdown_item_on_hover, ctx);
                }
            }
        }

        /* New Image Menu? */
        if (ctx->ui_state.new_image_menu) {
            compute_clay_new_image_menu(ctx);
        }
    }
}

void tools_brush_on_hover(Clay_ElementId element_id, Clay_PointerData pointer_info, void* user_data) {
    Context* ctx = (Context*)user_data;
    if (pointer_info.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        ctx->ui_state.current_tool = UI_TOOL_BRUSH;
        ctx->draw_color = ctx->brush_colors[ctx->current_brush];
    }
}

void tools_eraser_on_hover(Clay_ElementId element_id, Clay_PointerData pointer_info, void* user_data) {
    Context* ctx = (Context*)user_data;
    if (pointer_info.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        ctx->ui_state.current_tool = UI_TOOL_ERASER;
        ctx->draw_color = ctx->ignore_color;
    }
}

void tools_bucket_fill_on_hover(Clay_ElementId element_id, Clay_PointerData pointer_info, void* user_data) {
    Context* ctx = (Context*)user_data;
    if (pointer_info.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        ctx->ui_state.current_tool = UI_TOOL_BUCKET_FILL;
    }
}

void tools_button(Context* ctx, Texture2D* texture, PFN_onHover on_hover_func, int32_t tool_id) {
    CLAY_AUTO_ID({
        .layout = {
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .padding = CLAY_PADDING_ALL(10),
        },
        .backgroundColor = ctx->ui_state.current_tool == tool_id ? UI_COLOR_WHITE : Clay_Hovered() ? UI_COLOR_LIGHT_GRAY : UI_COLOR_DARK_GRAY,
        .cornerRadius = CLAY_CORNER_RADIUS(12),
    }) {
        Clay_OnHover(on_hover_func, ctx);
        CLAY_AUTO_ID({
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            },
            .aspectRatio = (float)texture->width / (float)texture->height,
            .image = {
                .imageData = texture,
            },
        });
    }
}

void compute_clay_tools(Context* ctx, Texture2D* textures, size_t image_count) {
    CLAY(CLAY_ID("tools"), {
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = { CLAY_SIZING_PERCENT(0.33f), CLAY_SIZING_PERCENT(1.0f) }, 
            .padding = CLAY_PADDING_ALL(12),
            .childGap = 10,
        },
        .backgroundColor = UI_COLOR_DARK_GRAY,
        .cornerRadius = { 12, 12, 12, 12 },
    }) {
        tools_button(ctx, &textures[3], tools_brush_on_hover, UI_TOOL_BRUSH);
        tools_button(ctx, &textures[4], tools_eraser_on_hover, UI_TOOL_ERASER);
        tools_button(ctx, &textures[5], tools_bucket_fill_on_hover, UI_TOOL_BUCKET_FILL);
    }
}

void color_picker_close_button_on_hover(Clay_ElementId element_id, Clay_PointerData pointer_info, void* user_data) {
    Context* ctx = (Context*)user_data;
    if (pointer_info.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        ctx->ui_state.color_picker_menu = false;
    }
}

void color_picker_top_bar_on_hover(Clay_ElementId element_id, Clay_PointerData pointer_info, void* user_data) {
    Context* ctx = (Context*)user_data;
    if (pointer_info.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        ctx->ui_state.color_picker_menu_floating = true;
    }
}

void compute_clay_color_picker_menu(Context* ctx) {
    CLAY(CLAY_ID("color_picker_menu"), {
        .floating = {
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .offset = { ctx->ui_state.color_picker_menu_pos.x, ctx->ui_state.color_picker_menu_pos.y },
        },
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = { CLAY_SIZING_FIXED(550), CLAY_SIZING_FIXED(650) },
            .padding = CLAY_PADDING_ALL(12),
        },
        .backgroundColor = UI_COLOR_DARK_DARK_DARK_GRAY,
        .cornerRadius = CLAY_CORNER_RADIUS(12),
    }) {
        CLAY(CLAY_ID("color_picker_menu_top_bar"), {
            .layout = {
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                .sizing = { CLAY_SIZING_PERCENT(1.0f), CLAY_SIZING_PERCENT(0.1f) },
                .padding = { 24, 24, 12, 12 },
                .childAlignment = CLAY_ALIGN_X_CENTER,
                .childGap = 12,
            },
            .backgroundColor = UI_COLOR_DARK_DARK_GRAY,
            .cornerRadius = CLAY_CORNER_RADIUS(12),
        }) {
            Clay_OnHover(color_picker_top_bar_on_hover, ctx);
            CLAY_TEXT(CLAY_STRING("Edit Color"), CLAY_TEXT_CONFIG({
                .fontId = 1,
                .fontSize = 40,
                .textColor = UI_COLOR_WHITE,
            }));
            CLAY_AUTO_ID({.layout = {.sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}}});
            CLAY(CLAY_ID("color_picker_menu_close_button"), {
                .layout = {
                    .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_GROW(0) },
                    .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER },
                },
                .cornerRadius = CLAY_CORNER_RADIUS(12),
                .aspectRatio = 1,
                .backgroundColor = UI_COLOR_RED,
            }) {
                Clay_OnHover(color_picker_close_button_on_hover, ctx);
                CLAY_TEXT(CLAY_STRING("X"), CLAY_TEXT_CONFIG({
                    .fontId = 1,
                    .fontSize = 40,
                    .textColor = UI_COLOR_WHITE,
                }));
            }
        }
    }
}

void tool_settings_brush_on_hover(Clay_ElementId element_id, Clay_PointerData pointer_info, void* user_data) {
    Context* ctx = (Context*)user_data;
    if (pointer_info.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        ctx->ui_state.color_picker_menu = true;
        ctx->ui_state.color_picker_menu_pos.x = ctx->window_width * 0.5f - 550 * 0.5f;
        ctx->ui_state.color_picker_menu_pos.y = ctx->window_height * 0.5f - 650 * 0.5f;
    }
}

void clay_tool_settings_brush(Context* ctx) {
    CLAY_AUTO_ID({
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .padding = CLAY_PADDING_ALL(8),
            .childAlignment = CLAY_ALIGN_X_CENTER,
            .childGap = 12,
        },
    }) {
        CustomLayoutElement_Circle circle = {
            .lines = true,
            .fill = true,
            .line_color = UI_COLOR_BLACK,
        };

        for (int32_t i = 0; i < BRUSH_COLORS_COUNT; i++) {
            circle.fill_color = RAYLIB_COLOR_TO_CLAY_COLOR(ctx->brush_colors[i]);
            custom_circle[i].type = CUSTOM_LAYOUT_ELEMENT_TYPE_CIRCLE;
            custom_circle[i].customData.circle = circle;
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                },
                .custom = {
                    .customData = &custom_circle[i],
                },
            }) {
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && Clay_Hovered()) {
                    ctx->current_brush = i;
                    ctx->draw_color = ctx->brush_colors[i];
                }
            }
        }  

        CLAY(CLAY_ID("color_picker"), {
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            },
            .image = {
                .imageData = &ctx->rainbow_circle,
            },
            .aspectRatio = 1.0f,
        }) {
            Clay_OnHover(tool_settings_brush_on_hover, ctx);
            if (ctx->ui_state.color_picker_menu) {
                compute_clay_color_picker_menu(ctx);
            }
        }
    }
}

void clay_tool_settings_eraser(Context* ctx) {

}

void clay_tool_settings_bucket_fill(Context* ctx) {

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
        switch (ctx->ui_state.current_tool) {
            case UI_TOOL_BRUSH:
                clay_tool_settings_brush(ctx);
                break;
            case UI_TOOL_ERASER:
                clay_tool_settings_eraser(ctx);
                break;
            case UI_TOOL_BUCKET_FILL:
                clay_tool_settings_bucket_fill(ctx);
                break;
            default:
                clay_tool_settings_brush(ctx);
        }
    }
}

void compute_clay_topbar(Context* ctx, Texture2D* textures, size_t image_count) {
    float t = ctx->ui_state.top_bar_lerp;

    float width_px = ease_in_out_quart_lerp(1000.0f, ctx->window_width, t);
    float height_px = ease_in_out_quart_lerp(100.0f, 112.0f, t);
    float radius_px = ease_in_out_quart_lerp(12.0f, 0.0f, t);
    float padding_top = ease_in_out_quart_lerp(10.0f, 22.0f, t);

    Clay_Sizing top_bar_size = {
        .width = CLAY_SIZING_FIXED(width_px),
        .height = CLAY_SIZING_FIXED(height_px),
    };

    Clay_Padding padding_setting = CLAY_PADDING_ALL(10);
    padding_setting.top = padding_top;

    CLAY(CLAY_ID("top_bar"), {
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = top_bar_size,
            .padding = padding_setting,
            .childGap = 16,
            .childAlignment = CLAY_ALIGN_X_CENTER,
        },
        .backgroundColor = UI_COLOR_DARK_DARK_GRAY,
        .cornerRadius = CLAY_CORNER_RADIUS(radius_px),
    }) {
        compute_clay_utilities(ctx, textures, image_count);
        compute_clay_tools(ctx, textures, image_count);
        compute_clay_tools_settings(ctx, textures, image_count);
    }
}

// because of headers
void compute_clay_layout(struct Context* ctx, Texture2D* textures, size_t images_count) {
    Clay_BeginLayout();

    float t = ctx->ui_state.top_bar_lerp;
    Clay_Padding padding_setting = CLAY_PADDING_ALL(12);
    padding_setting.left = ease_in_out_quart_lerp(12.0f, 0.0f, t);
    padding_setting.right = ease_in_out_quart_lerp(12.0f, 0.0f, t);
    padding_setting.top = ease_in_out_quart_lerp(12.0f, 0.0f, t);

    /* Window Container */
    CLAY(CLAY_ID("outer_container"), {
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .padding = padding_setting,
            .childGap = 12,
        },
    }) {
        CLAY_AUTO_ID({
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) }, 
                .childAlignment = CLAY_ALIGN_X_CENTER,
            },
        }) {
            compute_clay_topbar(ctx, textures, images_count);
        }
    }
}

void update_ui(struct Context *ctx) {
    bool is_mouse_down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    Clay_SetLayoutDimensions((Clay_Dimensions){ctx->window_width, ctx->window_height});
    Clay_SetPointerState((Clay_Vector2){ctx->current_mouse_pos.x, ctx->current_mouse_pos.y}, is_mouse_down);

    float delta = (1.0f / TOP_BAR_SNAP_ANIM_TIME) * GetFrameTime();

    bool merged =
        ctx->window_width < WINDOW_SIZE_THRESHOLD_TOP_BAR_SNAPPING;

    if (merged) {
        ctx->ui_state.top_bar_lerp += delta;
    }
    else {
        ctx->ui_state.top_bar_lerp -= delta;
    }

    /* Clamp */;
    if (ctx->ui_state.top_bar_lerp > 1.0f) ctx->ui_state.top_bar_lerp = 1.0f;
    if (ctx->ui_state.top_bar_lerp < 0.0f) ctx->ui_state.top_bar_lerp = 0.0f;

    /* Input */
    uiState* state = &ctx->ui_state;
    if (state->image_menu_width_input) {
        if (state->image_menu_height_input) state->image_menu_height_input = false;

        char c = GetKeyPressed();

        if (c >= 48 && c <= 57 && state->image_menu_width_index < UI_MAX_INPUT_CHARACTERS) {
            if (!(c == 48 && state->image_menu_width_index == 0)) {
                state->image_width[state->image_menu_width_index++] = c;
            }
        }
        else if (c == 3 && state->image_menu_width_index > 0) {
            state->image_width[--state->image_menu_width_index] = 0;
        }
        else if (c == 1) {
            state->image_menu_width_input = false;
            check_input_number(state->image_width);
        }
    }

    if (state->image_menu_height_input) {
        if (state->image_menu_width_input) state->image_menu_width_input = false;

        char c = GetKeyPressed();

        if (c >= 48 && c <= 57 && state->image_menu_height_index < UI_MAX_INPUT_CHARACTERS) {
            if (!(c == 48 && state->image_menu_height_index == 0)) {
                state->image_height[state->image_menu_height_index++] = c;
            } 
        }
        else if (c == 3 && state->image_menu_height_index > 0) {
            state->image_height[--state->image_menu_height_index] = 0;
        }
        else if (c == 1) {
            state->image_menu_height_input = false;
            check_input_number(state->image_height);
        }
    }

    if (is_mouse_down) {
        float dx = ctx->current_mouse_pos.x - ctx->previous_mouse_pos.x;
        float dy = ctx->current_mouse_pos.y - ctx->previous_mouse_pos.y;

        if (ctx->ui_state.image_menu_floating) {
            ctx->ui_state.image_menu_pos.x += dx;
            ctx->ui_state.image_menu_pos.y += dy;
        }
        if (ctx->ui_state.color_picker_menu_floating) {
            ctx->ui_state.color_picker_menu_pos.x += dx;
            ctx->ui_state.color_picker_menu_pos.y += dy;
        }
    }
   
    if (IsMouseButtonUp(MOUSE_BUTTON_LEFT)) {
        ctx->ui_state.image_menu_floating = false;
        ctx->ui_state.color_picker_menu_floating = false;
    }
}

void draw_ui(struct Context *ctx, Font* fonts) {
    Clay_RenderCommandArray cmd_array = Clay_EndLayout();
    Clay_Raylib_Render(cmd_array, fonts);
}

