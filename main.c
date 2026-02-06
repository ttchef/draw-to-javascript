
#include <dirent.h>
#include <iso646.h>
#include <stdio.h>

#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>

//#define STB_IMAGE_IMPLEMENTATION
//#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "libtinyfiledialogs/tinyfiledialogs.h"
#include "darray.h"

#include "common.h"
#include "arena_allocator.h"

#include "ui.c"

#define COLOR_PICKER_RESOLUTION 400

#define MAX_JAVASCRIPT_LINE 256

typedef struct jsLine {
    double offset_x;
    double offset_y;
    Color color;
} jsLine; 

static inline uint8_t hex2_to_u8(char* hex) {
    char buffer[3];
    buffer[0] = hex[0];
    buffer[1] = hex[1];
    buffer[2] = '\0';
    return strtol(buffer, NULL, 16);
}

/* Without errro checking and documentation xD */
static void parse_javascript_line(Context* ctx, char* line, int32_t line_index, jsLine** lines) {
    char* open_paren = strchr(line, '(');
    char* comma_pos_x = strchr(line, ',');
    char* plus_sign_x = strchr(line, '+');
    char* minus_sign_x = strchr(line, '-');

    bool plus_sign = false;
    if (plus_sign_x && plus_sign_x < comma_pos_x) plus_sign = true;

    char* sign_pos = plus_sign ? plus_sign_x : minus_sign_x;

    char* var_name_x = open_paren + 1;
    size_t var_name_x_len = sign_pos - var_name_x;

    size_t offset_x_digits = comma_pos_x - sign_pos;
    char offset_x_str[offset_x_digits + 1];
    memcpy(offset_x_str, sign_pos, offset_x_digits);
    offset_x_str[offset_x_digits] = '\0';

    double offset_x = atof(offset_x_str);

    char* plus_sign_y = strchr(comma_pos_x, '+');
    char* minus_sign_y = strchr(comma_pos_x, '-');

    plus_sign = false;
    if (plus_sign_y) plus_sign = true;

    sign_pos = plus_sign ? plus_sign_y : minus_sign_y;

    char* var_name_y = comma_pos_x + 2;
    size_t var_name_y_len = sign_pos - var_name_y;

    char* comma_pos_y = strchr(comma_pos_x + 1, ',');

    size_t offset_y_digits = comma_pos_y - sign_pos;
    char offset_y_str[offset_y_digits + 1];
    memcpy(offset_y_str, sign_pos, offset_y_digits);
    offset_y_str[offset_y_digits] = '\0';

    double offset_y = atof(offset_y_str);

    /* Width is same as height so only width needed */
    char* comma_width = strchr(comma_pos_y + 1, ',');
    char* width_pos = comma_pos_y + 2;

    size_t width_digits = comma_width - width_pos;
    char width_str[width_digits + 1];
    memcpy(width_str, width_pos, width_digits);
    width_str[width_digits] = '\0';

    double width = atof(width_str);

    char* hashtag_pos = strchr(comma_width, '#');
    char* curr_pos = hashtag_pos + 1;
    
    Color color = { .a = 255 };
    color.r = hex2_to_u8(curr_pos);
    curr_pos += 2;
    color.g = hex2_to_u8(curr_pos);
    curr_pos += 2;
    color.b = hex2_to_u8(curr_pos);

    /* Creation of Structure */
    jsLine data = {
        .offset_x = offset_x,
        .offset_y = offset_y,
        .color = color,
    };

    darrayPush(*lines, data);
}

static Vector2I get_image_dim_from_js(jsLine* array) {
    double max_x = 0.0;
    double max_y = 0.0;
    for (int32_t i = 0; i < darrayLength(array); i++) {
        jsLine* element = &array[i];
        if (fabs(element->offset_x) > max_x) max_x = fabs(element->offset_x);
        if (fabs(element->offset_y) > max_y) max_y = fabs(element->offset_y);
    }

    max_x = ceilf(max_x);
    max_y = ceilf(max_y);
    return (Vector2I){max_x * 2, max_y * 2 };
}

static void write_data_to_img(Context* ctx, jsLine* data)
{
    float center_x = ctx->new_image_width  * 0.5f;
    float center_y = ctx->new_image_height * 0.5f;

    int32_t offset = ctx->new_image_width % 2 == 0 ? 1 : 0;

    for (int32_t i = 0; i < darrayLength(data); i++)
    {
        float px = center_x - (float)data[i].offset_x;
        float py = center_y - (float)data[i].offset_y;

        int32_t img_pos_x = (int32_t)floorf(px) - offset;
        int32_t img_pos_y = (int32_t)floorf(py);


        if (img_pos_x < 0 || img_pos_x >= ctx->new_image_width ||
            img_pos_y < 0 || img_pos_y >= ctx->new_image_height)
            continue;

        int32_t index =
            (img_pos_y * ctx->new_image_width + img_pos_x) * 4;

        ctx->image_data[index + 0] = data[i].color.r;
        ctx->image_data[index + 1] = data[i].color.g;
        ctx->image_data[index + 2] = data[i].color.b;
        ctx->image_data[index + 3] = 255;
    }
}


void load_from_javascript(Context* ctx) {
    const char* filters[] = { "*.txt" };
    const char* path = tinyfd_openFileDialog(
            "Open Image",
            "",
            ARRAY_LEN(filters),
            filters,
            "Text Files",
            0);
    if (!path) {
        fprintf(stderr, "Failed to get path!\n");
        return;
    }

    FILE* file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "failed to open file: %s\n", path);
        return;
    }

    fseek(file, 0, SEEK_END);
    int64_t size = ftell(file);
    rewind(file);

    char buffer[size + 1];
    fread(buffer, size, 1, file);
    buffer[size] = '\0';

    char* delim = "\n";
    char* token = strtok(buffer, delim);

    jsLine* lines = darrayCreate(jsLine);

    int32_t line_index = 0;
    while (token != NULL) {
        char line[MAX_JAVASCRIPT_LINE];
        strncpy(line, token, MAX_JAVASCRIPT_LINE - 1);
        line[MAX_JAVASCRIPT_LINE - 1] = '\0';
        parse_javascript_line(ctx, line, line_index, &lines);

        token = strtok(NULL, delim);
        line_index++;
    }

    fclose(file);

    Vector2I dim = get_image_dim_from_js(lines);

    ctx->new_image_width = dim.x;
    ctx->new_image_height = dim.y;

    ctx->image_data = malloc(dim.x * dim.y * 4);
    if (!ctx->image_data) {
        fprintf(stderr, "failed to allocate image\n");
        exit(1);
    }

    int32_t total = ctx->new_image_width * ctx->new_image_height;
    for (int32_t i = 0; i < total; i++) {
        ctx->image_data[i * 4] = 0;
        ctx->image_data[i * 4 + 1] = 0;
        ctx->image_data[i * 4 + 2] = 0;
        ctx->image_data[i * 4 + 3] = 255;
    }

    write_data_to_img(ctx, lines);

    Image img = GenImageColor(ctx->new_image_width, ctx->new_image_height, BLACK);
    ctx->loaded_tex = LoadTextureFromImage(img);
    UnloadImage(img);

    SetTextureFilter(ctx->loaded_tex, TEXTURE_FILTER_POINT);
    UpdateTexture(ctx->loaded_tex, ctx->image_data);

    ctx->loaded_ratio = (float)ctx->loaded_tex.width / (float)ctx->loaded_tex.height;
    ctx->mode = UI_MODE_IMAGE_EDITING;

    darrayDestroy(lines);
}

static inline bool compare_colors(Color a, Color b) {
    return (a.r == b.r &&
            a.g == b.g &&
            a.b == b.b &&
            a.a == b.a);
}

/* Assumes Index is valid */
static inline Color get_color_from_index(Context* ctx, int32_t index) {
    return (Color){
        .r = ctx->image_data[index],
        .g = ctx->image_data[index + 1],
        .b = ctx->image_data[index + 2],
        .a = ctx->image_data[index + 3],
    };
}

static inline void write_pixel(Context* ctx, int32_t index, Color c) {
    ctx->image_data[index + 0] = c.r;
    ctx->image_data[index + 1] = c.g;
    ctx->image_data[index + 2] = c.b;
    ctx->image_data[index + 3] = c.a;
}

static void generate_rainbow_circle(Texture2D* result) {
    Image img = GenImageColor(COLOR_PICKER_RESOLUTION, COLOR_PICKER_RESOLUTION, BLACK); 
    Vector2I circle_center = {
        .x = COLOR_PICKER_RESOLUTION * 0.5f,
        .y = COLOR_PICKER_RESOLUTION * 0.5f,
    };
    int32_t radius = COLOR_PICKER_RESOLUTION * 0.5f;

    for (int32_t y = 0; y < COLOR_PICKER_RESOLUTION; y++) {
        for (int32_t x = 0; x < COLOR_PICKER_RESOLUTION; x++) {
            int32_t dx = x - circle_center.x;
            int32_t dy = y - circle_center.y;

            int32_t idx = (y * COLOR_PICKER_RESOLUTION + x) * 4;
            uint8_t* pix = &((uint8_t*)img.data)[idx];

            int32_t dist = dx * dx + dy * dy;
            if (dist <= radius * radius) {
                float val = 1.0f;
                float sat = sqrtf(dist) / radius;
                
                float angle = atan2f(dy, dx) * RAD2DEG;
                if (angle < 0) angle += 360.0f;
                angle += 180.0f;

                Color c = ColorFromHSV(angle, sat, val);
                
                pix[0] = c.r;
                pix[1] = c.g;
                pix[2] = c.b;
                pix[3] = 255;
            }
            else {
                pix[3] = 0;
            }
        }
    }

    *result = LoadTextureFromImage(img);
    UnloadImage(img);
}

void image_to_javascript(Context* ctx, FILE* fd, char* name_x, char* name_y) {
    for (int32_t y = 0; y < ctx->new_image_height; y++) {
        for (int32_t x = 0; x < ctx->new_image_width; x++) {
            int32_t index = (y * ctx->new_image_width + x) * 4;
            
            Color cmp_color = get_color_from_index(ctx, index);
            uint32_t color = 0;
            color |= cmp_color.b;
            color |= cmp_color.g << 8;
            color |= cmp_color.r << 16;

            if (compare_colors(cmp_color, ctx->ignore_color)) continue;

            int32_t pos_x = x - ctx->new_image_width / 2;
            int32_t pos_y = -(y - ctx->new_image_height / 2);
            if (ctx->export_x_mirrored) pos_x *= -1;
            fprintf(fd, "Canvas.rect(%s%+.2f, %s%+.2f, %.2f, %.2f, {fill:\"#%06X\"}),\n", name_x,
                    pos_x * ctx->export_scale, name_y, pos_y * ctx->export_scale,
                    1.5f * ctx->export_scale, 1.5f * ctx->export_scale, color);
        }
    }
}

static Rectangle get_image_dst(Context* ctx) {
    Rectangle dst = {0};

    int32_t scale_x = (int32_t)(ctx->window_width * 0.8f) / ctx->new_image_width;
    int32_t scale_y = ctx->window_height / ctx->new_image_height;
    int32_t scale = scale_x < scale_y ? scale_x : scale_y;
    if (scale < 1) scale = 1;

    dst.width  = ctx->new_image_width  * scale;
    dst.height = ctx->new_image_height * scale;
    dst.x = 0;
    dst.y = 0;

    return dst;
}

static inline Vector2I screen_to_image_space(Context* ctx, Vector2 vec, Rectangle dst) {
    float u = (vec.x - dst.x) / dst.width;
    float v = (vec.y - dst.y) / dst.height;

    u = fminf(fmaxf(u, 0.0f), 1.0f);
    v = fminf(fmaxf(v, 0.0f), 1.0f);

    int cx = (int32_t)floorf(u * ctx->new_image_width);
    int cy = (int32_t)floorf(v * ctx->new_image_height);
    return (Vector2I){cx, cy};
}

static inline int32_t vec_to_img(Context* ctx, Vector2I vec) {
    if (vec.x < 0 || vec.y < 0 ||
        vec.x >= ctx->new_image_width ||
        vec.y >= ctx->new_image_height)
        return -1;

    return (vec.y * ctx->new_image_width + vec.x) * 4;
}

static void draw_ignored_pixels(Context* ctx, Rectangle dst, float dst_pixel_width, float dst_pixel_height) {
    for (int32_t y = 0; y < ctx->new_image_width; y++) {
        for (int32_t x = 0; x < ctx->new_image_height; x++) {
            int32_t index = vec_to_img(ctx, (Vector2I){ x, y });

            if (index < 0) continue;
            Color pixel_color = get_color_from_index(ctx, index);

            if (ctx->draw_ignored_pixels && compare_colors(pixel_color, ctx->ignore_color)) {
                DrawRectangle(dst.x + x * dst_pixel_width,
                        dst.y + y * dst_pixel_height, dst_pixel_width, dst_pixel_height, Fade(PURPLE, 0.5f));
            }
        }
    }
}

static inline int32_t get_number_of_digits(int32_t num) {
    int32_t res = 0;
    while (num /= 10) res++;
    return res;
}

static void draw_debug_mode(Context* ctx, Rectangle dst, float dst_pixel_width, float dst_pixel_height) {
    Font font = GetFontDefault();
    float font_size = Clamp(fminf(dst_pixel_width, dst_pixel_height) * 0.5f, 0.6f, 10.0f);

    for (int32_t x = 0; x < ctx->new_image_width; x++) {
        float px = dst.x + x * dst_pixel_width;
        DrawLine(px, dst.y, px, dst.y + dst.height, RAYWHITE);

        if (x % 5 == 0) {
            DrawTextEx(font, TextFormat("%d", x), (Vector2){ px + font_size * 0.5f, dst.y - font_size },
                    font_size, font_size * 0.5f, RAYWHITE);
        }
    }

    int32_t font_width = MeasureText(TextFormat("%d", ctx->new_image_width), font_size);

    for (int32_t y = 0; y < ctx->new_image_height; y++) {
        float py = dst.y + y * dst_pixel_height;
        DrawLine(dst.x, py, dst.x + dst.width, py, RAYWHITE);

        if (y % 5 == 0) {
            int32_t number_of_digits = get_number_of_digits(y);
            float percent = number_of_digits / 10.0f;
            DrawTextEx(
                GetFontDefault(),
                TextFormat("%d", y),
                (Vector2){ dst.x - (font_width * percent) * 0.5f - font_size * 0.5f, py + font_size * 0.5f },
                font_size,
                font_size * 0.5f,
                RAYWHITE
            );
        }
    }
}

void draw_image(Context* ctx) {
    if (ctx->mode != UI_MODE_IMAGE_EDITING) return;
    Rectangle src = {
        .width = ctx->loaded_tex.width,
        .height = ctx->loaded_tex.height,
    };

    Rectangle dst = get_image_dst(ctx);
    float dst_pixel_width = dst.width / (float)ctx->new_image_width;
    float dst_pixel_height = dst.height / (float)ctx->new_image_height;

    DrawTexturePro(ctx->loaded_tex, src, dst, (Vector2){0, 0}, 0.0f, WHITE);

    if (ctx->draw_ignored_pixels) draw_ignored_pixels(ctx, dst, dst_pixel_width, dst_pixel_height);
    if (ctx->debug_mode) draw_debug_mode(ctx, dst, dst_pixel_width, dst_pixel_height);

    DrawRectangleLines(0, 0, dst.width, dst.height, RAYWHITE);
}

static bool pixel_saved(Context* ctx, int32_t index) {
    int32_t pixel_index = index / 4;

    if (ctx->pixel_stamp[pixel_index] == ctx->current_stamp) {
        return true;
    }
    ctx->pixel_stamp[pixel_index] = ctx->current_stamp;
    return false;
}

void draw_circle(Context* ctx, Vector2 pos_world, Rectangle dst, Color c) {
    float radius = ctx->brush_size;
    float radius_squared = radius * radius;

    Vector2I pos_image = screen_to_image_space(ctx, pos_world, dst);

    for (int32_t i = -radius; i <= radius; i++) {
        for (int32_t j = -radius; j <= radius; j++) {
            Vector2I pos = pos_image;
            pos.x += i;
            pos.y += j;
            if (i * i + j * j <= radius_squared) {
                int32_t index = vec_to_img(ctx, pos);
                if (index == -1) continue;

                if (!pixel_saved(ctx, index)) {
                    int32_t idx = ctx->save_states_index - 1;
                    if (idx < 0) idx = UNDO_COUNT - 1;

                    Color original_color = get_color_from_index(ctx, index);

                    PixelState s = {
                        .index = index,
                        .color = original_color,
                    };

                    darrayPush(ctx->save_states[idx].data.brush.pixels, s);
                }

                ctx->image_data[index] = c.r;
                ctx->image_data[index + 1] = c.g;
                ctx->image_data[index + 2] = c.b;
                ctx->image_data[index + 3] = 255;
            }
        }
    }
}

void bucket_fill(Context* ctx, Vector2I start) {
    int32_t w = ctx->new_image_width;
    int32_t h = ctx->new_image_height;

    if (compare_colors(ctx->draw_color, ctx->ignore_color))
        return;

    Vector2I *stack = malloc(sizeof(Vector2I) * w * h);
    int32_t top = 0;

    if (start.x < 0 || start.y < 0 || start.x >= w || start.y >= h) {
        free(stack);
        return;
    }

    int32_t idx = vec_to_img(ctx, start);
    Color c = get_color_from_index(ctx, idx);

    ctx->image_data[idx + 0] = ctx->draw_color.r;
    ctx->image_data[idx + 1] = ctx->draw_color.g;
    ctx->image_data[idx + 2] = ctx->draw_color.b;
    ctx->image_data[idx + 3] = ctx->draw_color.a;

    stack[top++] = start;

    while (top > 0) {
        Vector2I pos = stack[--top];

        Vector2I neighbors[4] = {
            { pos.x - 1, pos.y },
            { pos.x + 1, pos.y },
            { pos.x, pos.y - 1 },
            { pos.x, pos.y + 1 }
        };

        for (int i = 0; i < 4; i++) {
            Vector2I n = neighbors[i];

            if (n.x < 0 || n.y < 0 || n.x >= w || n.y >= h)
                continue;

            int32_t nidx = vec_to_img(ctx, n);
            Color nc = get_color_from_index(ctx, nidx);

            if (!compare_colors(nc, ctx->ignore_color) && compare_colors(nc, ctx->draw_color))
                continue;

            ctx->image_data[nidx + 0] = ctx->draw_color.r;
            ctx->image_data[nidx + 1] = ctx->draw_color.g;
            ctx->image_data[nidx + 2] = ctx->draw_color.b;
            ctx->image_data[nidx + 3] = ctx->draw_color.a;

            stack[top++] = n;
        }
    }

    free(stack);
}

static void new_save_state(Context* ctx, enum SaveStateType type) {
    if (ctx->save_states_index == UNDO_COUNT)
        ctx->save_states_index = 0;

    SaveState* s = &ctx->save_states[ctx->save_states_index];

    if (s->valid) {
        if (s->type == SAVE_STATE_TYPE_BRUSH) {
            darrayDestroy(s->data.brush.pixels);
        }
        else if (s->type == SAVE_STATE_TYPE_BUCKET_FILL) {

        }
    }

    s->type = type;
    s->valid = true;

    if (type == SAVE_STATE_TYPE_BRUSH) {
        s->data.brush.pixels = darrayCreate(PixelState);
    }

    ctx->save_states_index++;
    ctx->current_stamp++;
    printf("Saved: %d\n", ctx->save_states_index - 1);
}


void update_image_data(Context* ctx) {
    if (ctx->mode != UI_MODE_IMAGE_EDITING) return;
    if (ctx->above_ui) return; 

    if (!ctx->pixel_stamp) {
        ctx->pixel_stamp = calloc(ctx->new_image_width * ctx->new_image_height, sizeof(uint32_t));
        if (!ctx->pixel_stamp) {
            fprintf(stderr, "Failed to allocate stamp buffer\n");
            return;
        }
        ctx->current_stamp = 1;
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        bool first_time = !ctx->drawing;
        Vector2 mouse_screen = GetMousePosition();

        ctx->drawing = true;

        Rectangle dst = get_image_dst(ctx);
        Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), ctx->camera);

        if (!CheckCollisionPointRec(mouse, dst)) return;

        if (ctx->pick_color_draw) {
            Vector2I pos = screen_to_image_space(ctx, mouse, dst);
            int32_t index = vec_to_img(ctx, pos);
            ctx->draw_color = get_color_from_index(ctx, index);
            ctx->brush_colors[ctx->current_brush] = ctx->draw_color;
            ctx->pick_color_draw = false;
            return;
        }

        if (ctx->pick_color_ignore) {
            Vector2I pos = screen_to_image_space(ctx, mouse, dst);
            int32_t index = vec_to_img(ctx, pos);
            ctx->ignore_color = get_color_from_index(ctx, index);
            ctx->pick_color_ignore = false;
            return;
        }

        if (first_time) {
            new_save_state(ctx, SAVE_STATE_TYPE_BRUSH); // TODO: tmp with hardcoded type
        }

        if (ctx->ui_state.current_tool == UI_TOOL_BUCKET_FILL) {
            Vector2 curr_word = GetScreenToWorld2D(ctx->current_mouse_pos, ctx->camera);
            Vector2I pos_image = screen_to_image_space(ctx, curr_word, dst);
            bucket_fill(ctx, pos_image);
        }
        else {
            float radius = ctx->brush_size;
            float radius_squared = radius * radius;

            float dx = ctx->current_mouse_pos.x - ctx->previous_mouse_pos.x;
            float dy = ctx->current_mouse_pos.y - ctx->previous_mouse_pos.y;

            float screen_dist_squared = dx * dx + dy * dy;
            float radius_scale = Clamp(ctx->camera.zoom / 64.0f, 0.1f, 0.5f);

            if (screen_dist_squared >= radius_squared * radius_scale) {
                /* Need for filling in */
                int32_t lerp_count = screen_dist_squared / (radius_squared * radius_scale);
                if (lerp_count > 0) {
                    Vector2 prev_world = GetScreenToWorld2D(ctx->previous_mouse_pos, ctx->camera);
                    Vector2 curr_world = GetScreenToWorld2D(ctx->current_mouse_pos, ctx->camera);

                    float lerp_t = 0.0f;

                    for (int32_t i = 0; i <= lerp_count; i++) {
                        lerp_t = (float)i / lerp_count;
                        Vector2 pos_world = Vector2Lerp(prev_world, curr_world, lerp_t);

                        draw_circle(ctx, pos_world, dst, ctx->draw_color);
                    }
                }
            }

            draw_circle(ctx, mouse, dst, ctx->draw_color);
        }

        UpdateTexture(ctx->loaded_tex, ctx->image_data);
    }
}

static void undo(Context* ctx) {
    int32_t idx = ctx->save_states_index - 1;
    if (idx < 0) {
        fprintf(stderr, "cant undo no save state\n");
        return;
    }
    
    switch (ctx->save_states[idx].type) {
        case SAVE_STATE_TYPE_BRUSH: {
            PixelState* pixels = ctx->save_states[idx].data.brush.pixels;
            Rectangle dst = get_image_dst(ctx);

            for (int32_t i = 0; i < darrayLength(pixels); i++) {
                PixelState* p = &pixels[i];
                write_pixel(ctx, p->index, p->color);
            }
            darrayDestroy(pixels);
        };
        default:
            break;
    }

    ctx->save_states_index--;
    if (ctx->save_states_index < 0) ctx->save_states_index = UNDO_COUNT - 1;
    ctx->save_states[ctx->save_states_index].valid = false;

    UpdateTexture(ctx->loaded_tex, ctx->image_data);
}

static void redo(Context* ctx) {

}

static void handle_input(Context* ctx) {
    if (ctx->mode != UI_MODE_IMAGE_EDITING) return;
    
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        ctx->drawing = false;
    }

    // Panning
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Vector2 delta = GetMouseDelta();
        delta = Vector2Scale(delta, -1.0f / ctx->camera.zoom);
        ctx->camera.target = Vector2Add(ctx->camera.target, delta);
    }

    // Zoom
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        if (IsKeyDown(KEY_LEFT_SHIFT)) {
            ctx->brush_size += wheel;
            ctx->brush_size = Clamp(ctx->brush_size, 0.5f, 50.0f);
        }
        else {
            Vector2 mouse_world_pos = GetScreenToWorld2D(GetMousePosition(), ctx->camera);
            ctx->camera.offset = GetMousePosition();
            ctx->camera.target = mouse_world_pos;

            float scale = wheel * 0.2f;
            ctx->camera.zoom = Clamp(expf(logf(ctx->camera.zoom) + scale), 0.125f, 64.0f);
        }
    }

    if (IsKeyPressed(KEY_D)) {
        ctx->debug_mode = !ctx->debug_mode;
    }

    if (IsKeyDown(KEY_LEFT_SHIFT)) {
        ctx->draw_brush_size_debug = true;
    }
    if (IsKeyUp(KEY_LEFT_SHIFT)) {
        ctx->draw_brush_size_debug = false;
    }

    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

    /* Undo/Redo */
    if (ctrl && IsKeyPressed(KEY_Y)) { // German Keyboard layout
            undo(ctx);
    }
    else if (ctrl && IsKeyPressed(KEY_R)) {
            redo(ctx);
    }

    if (CheckCollisionPointRec(GetMousePosition(), ctx->ui_state.bounding_box)) {
        ctx->above_ui = true;
    }
}

int main() {
    Context ctx = { .window_width = 1200, .window_height = 800, .ui_state = {0} };

    uint64_t total_mem = Clay_MinMemorySize();
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(total_mem, malloc(total_mem));
    Clay_Initialize(arena, (Clay_Dimensions){ctx.window_width, ctx.window_height}, (Clay_ErrorHandler){handle_clay_errors});
    Clay_Raylib_Initialize(ctx.window_width, ctx.window_height, "Draw to Javascript", FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    SetWindowMinSize(800, 600);

    Image icon = LoadImage("res/images/paint.png");
    SetWindowIcon(icon);
    UnloadImage(icon);

    Font fonts[2] = {
        LoadFontEx("res/fonts/AdwaitaSans-Regular.ttf", 20, 0, 250),
        LoadFontEx("res/fonts/AdwaitaSans-Regular.ttf", 40, 0, 250),
    };
    Clay_SetMeasureTextFunction(Raylib_MeasureText, fonts);

    Texture2D ui_images[] = {
        LoadTexture("res/images/document.png"),
        LoadTexture("res/images/export.png"),
        LoadTexture("res/images/setting.png"),
        LoadTexture("res/images/brush.png"),
        LoadTexture("res/images/eraser.png"),
        LoadTexture("res/images/paint-bucket.png"),
    };

    ctx.mode = UI_MODE_FILE_SELECTION;
    ctx.clear_color = BLACK;
    ctx.ignore_color = BLACK;
    ctx.brush_colors[0] = RED;
    ctx.brush_colors[1] = BLUE;
    ctx.draw_color = ctx.brush_colors[0];
    ctx.brush_size = 2.0f;
    ctx.camera.zoom = 1.0f;
    ctx.export_scale = 1.0f;

    generate_rainbow_circle(&ctx.rainbow_circle);

    init_ui(&ctx);

    float ui_click_cooldown = UI_CLICK_COOLDOWN;

    while (!WindowShouldClose()) {
        ctx.window_width = GetScreenWidth();
        ctx.window_height = GetScreenHeight();

        ctx.previous_mouse_pos = ctx.current_mouse_pos;
        ctx.current_mouse_pos = GetMousePosition();

        ctx.above_ui = false;

        handle_input(&ctx);
        update_ui(&ctx); /* Important order of func calls here DONT CHANGE!! */
        compute_clay_layout(&ctx, ui_images, ARRAY_LEN(ui_images));

        if (ctx.enalbe_ui_click_cooldown) {
            ctx.above_ui = true;
            ui_click_cooldown -= GetFrameTime();
            if (ui_click_cooldown <= 0.0f) {
                ui_click_cooldown = UI_CLICK_COOLDOWN;
                ctx.above_ui = false;
                ctx.enalbe_ui_click_cooldown = false;
            }
        }

        update_image_data(&ctx);

        BeginDrawing();
        ClearBackground(ctx.clear_color);
        BeginMode2D(ctx.camera);

        draw_image(&ctx);
    
        if (ctx.draw_brush_size_debug) {
            Vector2 world_pos = GetScreenToWorld2D(GetMousePosition(), ctx.camera);
            Rectangle dst = get_image_dst(&ctx);

            float world_per_pixel_x = dst.width / ctx.new_image_width;
            float world_per_pixel_y = dst.height / ctx.new_image_height;
            float world_radius = ctx.brush_size * world_per_pixel_x;

            DrawCircleLinesV(world_pos, world_radius, WHITE);
        }

        DrawFPS(10, 10);

        EndMode2D();

        draw_ui(&ctx, fonts);

        EndDrawing();
    }

    if (ctx.image_data) free(ctx.image_data);
    UnloadTexture(ctx.loaded_tex);

    Clay_Raylib_Close();

    return 0;
}

