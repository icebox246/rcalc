#include "imgui.h"

#include <raymath.h>

Rectangle get_screen_rect() {
    return (Rectangle){
        .x = 0,
        .y = 0,
        .width = GetRenderWidth(),
        .height = GetRenderHeight(),
    };
}

Rectangle split_rect_vert(Rectangle container, float t) {
    if (t > 0) {
        return (Rectangle){
            .x = container.x,
            .y = container.y,
            .width = container.width,
            .height = container.height * t,
        };
    } else {
        return (Rectangle){
            .x = container.x,
            .y = container.y + container.height * -t,
            .width = container.width,
            .height = container.height * (1 + t),
        };
    }
}

Rectangle split_rect_grid(Rectangle container, int gw, int gh, int row,
                          int col) {
    float cw = container.width / gw;
    float ch = container.height / gh;
    return (Rectangle){
        .x = container.x + cw * col,
        .y = container.y + ch * row,
        .width = cw,
        .height = ch,
    };
}

Rectangle margin_rect(Rectangle container, int margin) {
    return (Rectangle){
        .x = container.x + margin,
        .y = container.y + margin,
        .width = container.width - 2 * margin,
        .height = container.height - 2 * margin,
    };
}

static Vector2 get_rect_center(Rectangle rec) {
    return (Vector2){
        .x = rec.x + rec.width / 2.f,
        .y = rec.y + rec.height / 2.f,
    };
}

static void draw_centered_text(const char* text, Vector2 point, int size,
                               Color color) {
    int tw = MeasureText(text, size);
    DrawText(text, point.x - tw / 2.f, point.y - size / 2.f, size, color);
}

bool im_button(Rectangle rec, const char* text) {
    Color bg_color = color_palette[button_normal_color];
    Color fg_color = color_palette[button_pressed_color];
    bool colliding = false;
    if (CheckCollisionPointRec(GetMousePosition(), rec)) {
        colliding = true;
    }
    if (colliding && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        bg_color = color_palette[button_pressed_color];
        fg_color = color_palette[button_normal_color];
    }
    DrawRectangleRec(rec, bg_color);

    Vector2 center = get_rect_center(rec);

    draw_centered_text(text, center, gui_font_size, fg_color);

    return colliding && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}
