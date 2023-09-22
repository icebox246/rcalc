#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <raylib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "imgui.h"

// number

typedef int64_t Number;

const int number_decimal_digits = 6;
const int64_t number_scaling_factor = 1000000;

Number number_handle_overflow(__int128_t t) {
    if (t > LLONG_MAX) return LLONG_MAX;
    if (t < LLONG_MIN) return LLONG_MIN;
    return t;
}

Number number_add(Number a, Number b) {
    __int128_t ta = a;
    __int128_t tb = b;
    return ta + tb;
}

Number number_sub(Number a, Number b) {
    __int128_t ta = a;
    __int128_t tb = b;
    return ta - tb;
}

Number number_mul(Number a, Number b) {
    __int128_t ta = a;
    __int128_t tb = b;
    return number_handle_overflow((ta * tb) / number_scaling_factor);
}

Number number_div(Number a, Number b) {
    if (b == 0) return a > 0 ? LLONG_MAX : LLONG_MIN;
    __int128_t ta = a;
    __int128_t tb = b;
    return number_handle_overflow(ta * number_scaling_factor / tb);
}

Number number_root(Number x, int base) {
    if (x == LLONG_MAX) return LLONG_MAX;
    if (x == LLONG_MIN) return LLONG_MIN;

    __int128_t l = 0, r = x;
    while (l != r) {
        Number c = ((__int128_t)l + (__int128_t)r + 1) / 2;
        Number a = number_scaling_factor;
        for (size_t i = 0; i < base; i++) {
            a = number_mul(a, c);
        }
        if (a > x) {
            r = c - 1;
        } else {
            l = c;
        }
    }

    return l;
}

Number number_pow_i(Number x, int e) {
    Number acc = number_scaling_factor;
    Number p2 = x;

    while (e) {
        if (e & 1) acc = number_mul(acc, p2);
        e >>= 1;
        p2 = number_mul(p2, p2);
    }

    return acc;
}

Number number_pow(Number x, Number y) {
    if (y < 0) {
        return number_pow(number_div(number_scaling_factor, x), -y);
    }

    Number acc = number_pow_i(x, y / number_scaling_factor);

    if (x > 0) {
        uint64_t mantisa = y % number_scaling_factor;

        Number digs[number_decimal_digits];
        digs[0] = number_root(x, 10);
        for (size_t i = 1; i < number_decimal_digits; i++) {
            digs[i] = number_root(digs[i - 1], 10);
        }

        for (int i = number_decimal_digits - 1; i >= 0; i--) {
            size_t d = mantisa % 10;
            mantisa /= 10;
            Number a = number_scaling_factor;
            for (size_t j = 0; j < d; j++) {
                a = number_mul(a, digs[i]);
            }
            acc = number_mul(acc, a);
        }
    }

    return acc;
}

// text buffer

#define max_text_buffer_size 18

typedef struct {
    char buffer[max_text_buffer_size + 1];
    size_t count;
    size_t cursor;
    bool period_present;
    int period_position;
    bool negative;
    float last_edit_time;
} TextBuffer;

void text_buffer_append_digit(TextBuffer* tb, int digit) {
    assert(digit >= 0 && digit <= 9);
    if (tb->count >= max_text_buffer_size) return;

    if (tb->cursor < tb->count) {
        memmove(&tb->buffer[tb->cursor + 1], &tb->buffer[tb->cursor],
                max_text_buffer_size - tb->cursor);
    }
    tb->buffer[tb->cursor++] = '0' + digit;
    tb->count++;
    tb->last_edit_time = GetTime();
}

void text_buffer_append_period(TextBuffer* tb) {
    if (tb->period_present) return;
    if (tb->count >= max_text_buffer_size) return;

    if (tb->cursor < tb->count) {
        memmove(&tb->buffer[tb->cursor + 1], &tb->buffer[tb->cursor],
                max_text_buffer_size - tb->cursor);
    }
    tb->period_present = true;
    tb->period_position = tb->cursor;
    tb->buffer[tb->cursor++] = '.';
    tb->count++;
    tb->last_edit_time = GetTime();
}

void text_buffer_backspace(TextBuffer* tb) {
    if (tb->cursor == 0) return;
    if (tb->buffer[tb->cursor - 1] == '.') {
        tb->period_present = false;
    }
    if (tb->cursor != tb->count) {
        memmove(&tb->buffer[tb->cursor - 1], &tb->buffer[tb->cursor],
                max_text_buffer_size - tb->cursor);
    }
    tb->cursor--;
    tb->count--;
    tb->buffer[tb->count] = '\0';
    tb->last_edit_time = GetTime();
}

void text_toggle_negative(TextBuffer* tb) { tb->negative = !tb->negative; }

void text_buffer_clear(TextBuffer* tb) { memset(tb, 0, sizeof(TextBuffer)); }

Number text_buffer_get(TextBuffer* tb) {
    char* end;
    Number out = strtoll(tb->buffer, &end, 10);
    if (errno == ERANGE) {
        if (!tb->negative)
            return LLONG_MAX;
        else
            return LLONG_MIN;
    }
    if ((__int128_t)out * (__int128_t)number_scaling_factor > LLONG_MAX)
        return LLONG_MAX;

    out *= number_scaling_factor;

    if (end != tb->buffer + tb->count) {
        if (*end == '.') {
            char* begin = ++end;

            Number mantisa = 0;

            while (*end && (end - begin + 1) <= number_decimal_digits) {
                mantisa *= 10;
                mantisa += *end - '0';
                end++;
            }

            for (int i = number_decimal_digits - (end - begin); i > 0; i--) {
                mantisa *= 10;
            }

            out += mantisa;
        } else {
            assert(false && "Unreachable");
        }
    }

    if (tb->negative) out *= -1;

    return out;
}

void draw_text_buffer(Rectangle container, TextBuffer* tb) {
    DrawRectangleRec(container, color_palette[1]);

    container = margin_rect(container, 8);

    int font_size = gui_font_size * 2;

    int w = MeasureText(tb->buffer, font_size);

    if (w + font_size > container.width) {
        float t = container.width / (w + font_size);
        font_size *= t;
        w *= t;
    }

    Vector2 mouse = GetMousePosition();

    if (CheckCollisionPointRec(mouse, container) &&
        IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        float mx = mouse.x - (container.x + container.width - w);

        size_t b;
        float bd = INFINITY;

        for (size_t i = 0; i <= tb->count; i++) {
            char temp = tb->buffer[i];
            tb->buffer[i] = '\0';
            float tw = MeasureText(tb->buffer, font_size);
            float d = fabsf(tw - mx);
            if (d < bd) {
                b = i;
                bd = d;
            }
            tb->buffer[i] = temp;
        }

        tb->cursor = b;
    }

    DrawText(tb->buffer, container.x + container.width - w,
             container.y + (container.height - font_size) / 2, font_size,
             color_palette[3]);

    if (tb->negative) {
        DrawText("-", container.x + container.width - w - font_size * 0.5f,
                 container.y + (container.height - font_size) / 2, font_size,
                 color_palette[4]);
    }

    if (!((int)((GetTime() - tb->last_edit_time) / 0.5) & 1)) {
        char temp = tb->buffer[tb->cursor];

        tb->buffer[tb->cursor] = '\0';

        int ox = MeasureText(tb->buffer, font_size);

        const int cursor_width = 2;

        DrawRectangle(container.x + container.width - w + ox + cursor_width,
                      container.y + (container.height - font_size) / 2,
                      cursor_width, font_size, color_palette[4]);

        tb->buffer[tb->cursor] = temp;
    }
}

// keyboard

typedef enum {
    NONE,
    DIGIT0,
    DIGIT1,
    DIGIT2,
    DIGIT3,
    DIGIT4,
    DIGIT5,
    DIGIT6,
    DIGIT7,
    DIGIT8,
    DIGIT9,
    PERIOD,
    BACKSPACE,
    PUSH,
    ADD,
    SUB,
    MUL,
    DIV,
    POW,
    SQRT,
    TOGGLE_SIGN,
    POP_TO_BUFFER,
    SWAP,
} KeyboardButton;

KeyboardButton draw_keyboard(Rectangle container) {
    int gw = 4;
    int gh = 6;

    DrawRectangleRec(container, color_palette[0]);

    const int button_margin = 2;

    container = margin_rect(container, button_margin);

    KeyboardButton pressed_button = NONE;

    button_normal_color = 4;
    button_pressed_color = 1;

    {
        int digit = 0;
        {
            if (im_button(margin_rect(split_rect_grid(container, gw, gh, 5, 0),
                                      button_margin),
                          "0")) {
                pressed_button = DIGIT0;
            }
        }
        digit++;

        for (int row = 4; row >= 2; row--) {
            for (int col = 0; col <= 2; col++) {
                const char* label = TextFormat("%d", digit);
                if (im_button(margin_rect(
                                  split_rect_grid(container, gw, gh, row, col),
                                  button_margin),
                              label)) {
                    pressed_button = DIGIT0 + digit;
                }
                digit++;
            }
        }
    }

    button_normal_color = 2;
    button_pressed_color = 1;

    if (im_button(margin_rect(split_rect_grid(container, gw, gh, 5, 1),
                              button_margin),
                  ".")) {
        pressed_button = PERIOD;
    }

    if (im_button(margin_rect(split_rect_grid(container, gw, gh, 5, 2),
                              button_margin),
                  "del")) {
        pressed_button = BACKSPACE;
    }

    if (im_button(margin_rect(split_rect_grid(container, gw, gh, 5, 3),
                              button_margin),
                  "push")) {
        pressed_button = PUSH;
    }

    if (im_button(margin_rect(split_rect_grid(container, gw, gh, 4, 3),
                              button_margin),
                  "+")) {
        pressed_button = ADD;
    }

    if (im_button(margin_rect(split_rect_grid(container, gw, gh, 3, 3),
                              button_margin),
                  "-")) {
        pressed_button = SUB;
    }

    if (im_button(margin_rect(split_rect_grid(container, gw, gh, 2, 3),
                              button_margin),
                  "*")) {
        pressed_button = MUL;
    }

    if (im_button(margin_rect(split_rect_grid(container, gw, gh, 1, 3),
                              button_margin),
                  "/")) {
        pressed_button = DIV;
    }

    if (im_button(margin_rect(split_rect_grid(container, gw, gh, 0, 3),
                              button_margin),
                  "pow")) {
        pressed_button = POW;
    }

    if (im_button(margin_rect(split_rect_grid(container, gw, gh, 0, 2),
                              button_margin),
                  "sqrt")) {
        pressed_button = SQRT;
    }

    if (im_button(margin_rect(split_rect_grid(container, gw, gh, 1, 2),
                              button_margin),
                  "+/-")) {
        pressed_button = TOGGLE_SIGN;
    }

    if (im_button(margin_rect(split_rect_grid(container, gw, gh, 1, 1),
                              button_margin),
                  "pop")) {
        pressed_button = POP_TO_BUFFER;
    }

    if (im_button(margin_rect(split_rect_grid(container, gw, gh, 1, 0),
                              button_margin),
                  "swap")) {
        pressed_button = SWAP;
    }

    return pressed_button;
}

// stack

typedef struct {
    Number* items;
    size_t count;
    size_t capacity;
} Stack;

void stack_push(Stack* stack, Number n) {
    if (stack->capacity == 0) {
        stack->items = malloc(4 * sizeof(Number));
        stack->capacity = 4;
    } else if (stack->capacity == stack->count) {
        stack->items =
            realloc(stack->items, stack->capacity * 2 * sizeof(Number));
        stack->capacity = stack->capacity * 2;
    }
    stack->items[stack->count++] = n;
}

Number stack_pop(Stack* stack) { return stack->items[--stack->count]; }

void draw_stack(Rectangle container, Stack* stack) {
    container = margin_rect(container, 8);

    const int spacing = 2;

    for (size_t i = 0; i < stack->count; i++) {
        char* num;
        switch (stack->items[i]) {
            case LLONG_MAX:
                num = "infty";
                break;
            case LLONG_MIN:
                num = "-infty";
                break;
            default: {
                num = (char*)TextFormat(
                    "%ld.%0*ld", stack->items[i] / number_scaling_factor,
                    number_decimal_digits,
                    llabs(stack->items[i]) % number_scaling_factor);
                size_t it = TextLength(num);
                size_t dropped = 0;
                while (--it) {
                    if (num[it] != '0' && num[it] != '.') break;
                    num[it] = '\0';
                    if (++dropped == number_decimal_digits + 1) break;
                }
            }
        }

        int w = MeasureText(num, gui_font_size);
        DrawText(num, container.x + container.width - w,
                 container.y + container.height -
                     (gui_font_size + spacing) * (stack->count - i),
                 gui_font_size, color_palette[4]);
    }
}

void stack_pop_onto_text_buffer(Stack* st, TextBuffer* tb) {
    Number n = stack_pop(st);
    switch (n) {
        case LLONG_MAX:
        case LLONG_MIN:
            return;
        default: {
            memset(tb, 0, sizeof(TextBuffer));
            char* num = (char*)TextFormat(
                "%ld.%0*ld", n / number_scaling_factor, number_decimal_digits,
                llabs(n) % number_scaling_factor);
            size_t it = TextLength(num);
            size_t dropped = 0;
            while (--it) {
                if (num[it] != '0' && num[it] != '.') break;
                num[it] = '\0';
                if (++dropped == number_decimal_digits + 1) break;
            }
            if (*num == '-') {
                tb->negative = true;
                num++;
            }
            size_t len = TextLength(num);
            TextCopy(tb->buffer, num);
            tb->count = tb->cursor = len;
            for (size_t i = 0; i < len; i++) {
                if (num[i] == '.') {
                    tb->period_present = true;
                    tb->period_position = i;
                    break;
                }
            }
        }
    }
}

typedef Number(BinaryOp)(Number, Number);

void perform_binary_op(TextBuffer* tb, Stack* st, BinaryOp* op) {
    if (tb->count) {
        Number n = text_buffer_get(tb);
        text_buffer_clear(tb);
        stack_push(st, n);
    }
    if (st->count >= 2) {
        Number b = stack_pop(st);
        Number a = stack_pop(st);
        stack_push(st, op(a, b));
    }
}

int main() {
    TraceLog(LOG_INFO, "Hallo");

    InitWindow(500, 1000, "rcalc");

    TextBuffer tb = {0};
    Stack st = {0};

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(color_palette[0]);

        int key;
        bool shoud_exit = false;
        while ((key = GetKeyPressed())) {
            if (key == KEY_BACK) {
                shoud_exit = true;
                break;
            }
        }
        if (shoud_exit) break;

        Rectangle screen_rect = get_screen_rect();

        {
            Rectangle upper_pane = split_rect_vert(screen_rect, 0.45);
            draw_stack(split_rect_vert(upper_pane, 0.8), &st);
            draw_text_buffer(split_rect_vert(upper_pane, -0.8), &tb);
        }

        KeyboardButton pressed_button =
            draw_keyboard(split_rect_vert(screen_rect, -0.45));

        switch (pressed_button) {
            case NONE:
                break;
            case DIGIT0:
            case DIGIT1:
            case DIGIT2:
            case DIGIT3:
            case DIGIT4:
            case DIGIT5:
            case DIGIT6:
            case DIGIT7:
            case DIGIT8:
            case DIGIT9:
                text_buffer_append_digit(&tb, pressed_button - DIGIT0);
                break;
            case PERIOD:
                text_buffer_append_period(&tb);
                break;
            case BACKSPACE:
                text_buffer_backspace(&tb);
                break;
            case PUSH: {
                if (tb.count) {
                    Number n = text_buffer_get(&tb);
                    text_buffer_clear(&tb);
                    stack_push(&st, n);
                }
            } break;
            case ADD:
                perform_binary_op(&tb, &st, number_add);
                break;
            case SUB:
                perform_binary_op(&tb, &st, number_sub);
                break;
            case MUL:
                perform_binary_op(&tb, &st, number_mul);
                break;
            case DIV:
                perform_binary_op(&tb, &st, number_div);
                break;
            case POW:
                perform_binary_op(&tb, &st, number_pow);
                break;
            case SQRT: {
                if (tb.count) {
                    Number n = text_buffer_get(&tb);
                    text_buffer_clear(&tb);
                    stack_push(&st, n);
                }
                if (st.count) {
                    Number a = stack_pop(&st);
                    stack_push(&st, number_root(a, 2));
                }
            } break;
            case TOGGLE_SIGN:
                text_toggle_negative(&tb);
                break;
            case POP_TO_BUFFER:
                if (st.count)
                    stack_pop_onto_text_buffer(&st, &tb);
                else
                    text_buffer_clear(&tb);
                break;
            case SWAP:
                if ((tb.count && st.count) || st.count > 2) {
                    bool with_buffer = false;
                    if (tb.count) {
                        Number n = text_buffer_get(&tb);
                        text_buffer_clear(&tb);
                        stack_push(&st, n);
                        with_buffer = true;
                    }
                    Number a = stack_pop(&st);
                    Number b = stack_pop(&st);
                    stack_push(&st, a);
                    stack_push(&st, b);

                    if (with_buffer) stack_pop_onto_text_buffer(&st, &tb);
                }
                break;
        }

        EndDrawing();
    }

    free(st.items);

    CloseWindow();
}
