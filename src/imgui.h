#ifndef IMGUI_H_
#define IMGUI_H_

#include <raylib.h>
#include <stddef.h>

#define COLOR_RGB(c)                  \
    ((Color){.r = ((c) >> 16) & 0xff, \
             .g = ((c) >> 8) & 0xff,  \
             .b = (c)&0xff,           \
             .a = 0xff})

static const Color color_palette[] = {
    COLOR_RGB(0x000000), COLOR_RGB(0x3A3C4E), COLOR_RGB(0x4D4F68),
    COLOR_RGB(0x7E9CD8), COLOR_RGB(0x7AA89F),
};

static const int gui_font_size = 30;

extern size_t button_normal_color;
extern size_t button_pressed_color;

Rectangle get_screen_rect();

Rectangle split_rect_vert(Rectangle container, float t);

Rectangle split_rect_grid(Rectangle container, int gw, int gh, int row,
                          int col);

Rectangle margin_rect(Rectangle container, int margin);

bool im_button(Rectangle rec, const char* text);

#endif  // IMGUI_H_
