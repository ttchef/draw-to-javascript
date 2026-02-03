
#include <dirent.h>
#include <stdio.h>

#include <raylib.h>
#include <raymath.h>

//#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "libtinyfiledialogs/tinyfiledialogs.h"
#include "darray.h"

#include "common.h"
#include "ui.c"

#define COLOR_PICKER_RESOLUTION 400

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

    fseek(file, 0, SEEK_END);
    int64_t size = ftell(file);
    rewind(file);

    char buffer[size + 1];
    fread(buffer, size, 1, file);
    buffer[size] = '\0';

    char* delim = ",({";
    char* token = strtok(buffer, delim);
    while (token != NULL) {
        if (token[0] == 'C') {
            token = strtok(NULL, delim);
            continue;
        }
        printf("%s\n", token);
        token = strtok(NULL, delim);
    }

    fclose(file);
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
            
            uint8_t r = ctx->image_data[index];
            uint8_t g = ctx->image_data[index + 1];
            uint8_t b = ctx->image_data[index + 2];
            uint8_t a = ctx->image_data[index + 3];
            Color cmp_color = (Color){
                .r = r,
                .b = b,
                .g = g,
                .a = a,
            };
            uint32_t color = 0;
            color |= b;
            color |= g << 8;
            color |= r << 16;

            if (compare_colors(cmp_color, ctx->ignore_color)) continue;

            int32_t pos_x = x - ctx->new_image_width / 2;
            int32_t pos_y = -(y - ctx->new_image_height / 2);
            if (ctx->export_x_mirrored) pos_x *= -1;
            fprintf(fd, "Canvas.rect(%s%+.2f, %s%+.2f, %.2f, %.2f, {fill:\"#%X\"}),\n", name_x,
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

void draw_circle(Context* ctx, Vector2 pos_world, Rectangle dst) {
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
                ctx->image_data[index] = ctx->draw_color.r;
                ctx->image_data[index + 1] = ctx->draw_color.g;
                ctx->image_data[index + 2] = ctx->draw_color.b;
                ctx->image_data[index + 3] = 255;
            }
        }
    }
}

void update_image_data(Context* ctx) {
    if (ctx->mode != UI_MODE_IMAGE_EDITING) return;
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
            /* New Save state */
            if (ctx->save_states[ctx->save_states_index]) {
                darrayDestroy(ctx->save_states[ctx->save_states_index]);
            }
            ctx->save_states[ctx->save_states_index++] = darrayCreate(PixelState);
            if (ctx->save_states_index >= UNDO_COUNT) ctx->save_states_index = 0; 
        }

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
                    draw_circle(ctx, pos_world, dst);
                 
                    if (first_time) {
                        /* Save to save state */
                        
                    }
                }
            }
        }

        draw_circle(ctx, mouse, dst);

        UpdateTexture(ctx->loaded_tex, ctx->image_data);
    }
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
        Vector2 mouse_world_pos = GetScreenToWorld2D(GetMousePosition(), ctx->camera);
        ctx->camera.offset = GetMousePosition();
        ctx->camera.target = mouse_world_pos;

        float scale = wheel * 0.2f;
        ctx->camera.zoom = Clamp(expf(logf(ctx->camera.zoom) + scale), 0.125f, 64.0f);
    }

    if (IsKeyPressed(KEY_D)) {
        ctx->debug_mode = !ctx->debug_mode;
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
        LoadTexture("res/images/save.png"),
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

    while (!WindowShouldClose()) {
        ctx.window_width = GetScreenWidth();
        ctx.window_height = GetScreenHeight();

        ctx.previous_mouse_pos = ctx.current_mouse_pos;
        ctx.current_mouse_pos = GetMousePosition();

        handle_input(&ctx);
        update_image_data(&ctx);
        update_ui(&ctx);

        BeginDrawing();
        ClearBackground(ctx.clear_color);
        BeginMode2D(ctx.camera);

        DrawRectangle(0, 0, ctx.window_width, ctx.window_height, BLACK); // TMP

        draw_image(&ctx);
        DrawFPS(10, 10);

        EndMode2D();

        compute_clay_layout(&ctx, ui_images, ARRAY_LEN(ui_images));
        draw_ui(&ctx, fonts);

        EndDrawing();
    }

    if (ctx.image_data) free(ctx.image_data);
    UnloadTexture(ctx.loaded_tex);

    Clay_Raylib_Close();
    return 0;
}

