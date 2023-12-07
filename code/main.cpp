#include <stdio.h>
#include <stdlib.h>

#include <raylib/raylib.h>
#include <raylib/raymath.h>

#define UNUSED(x) ((void)(x))

#define internal static
#define global static

#define WIDTH 1280
#define HEIGHT 720
#define MIN_WIDTH 1000
#define MIN_HEIGHT 700
#define FPS 60

#define MIDI_LEN 22

internal void draw_text_centered(const char *text, int x, int y, int font_size)
{
    int text_width = MeasureText(text, font_size);
    DrawText(text, x - text_width/2, y - font_size/2, font_size, WHITE);
}

internal bool collides_with_keys(Rectangle *keys, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        if (CheckCollisionPointRec(GetMousePosition(), keys[i])) {
            return(true);
        }
    }

    return(false);
}

// @Note: By default we render 'regular/extended' ffxiv keyboard, at some point
// in the future we might generalize it to more/less keys.
internal void render_keyboard(Rectangle rect, int key_width, int key_padding)
{
    Rectangle white_key = {0};
    white_key.width = (float) key_width;
    white_key.height = rect.height;
    white_key.x = 0;
    white_key.y = rect.y;

    Rectangle black_key = {0};
    black_key.width = (float) key_width;
    black_key.height = rect.height/2.0f;
    black_key.x = (key_width + key_padding)/2.0f;
    black_key.y = rect.y;

    // @Note: We need to decouple rendering from logic
    // handling, because rectangles overlap each-other
    // I wanted to solve this problem with z-index or something similar
    // but raylib (afaik) doesn't provide anything like that (nor layers etc.)
    Rectangle black_keys[MIDI_LEN - 1] = {0};
    Rectangle white_keys[MIDI_LEN] = {0};
    
    for (int i = 0, j = 0; i < MIDI_LEN; ++i) {
        white_keys[i] = white_key;

        // @Robustness: Change this, it's just ugly.
        if (i != MIDI_LEN - 1 && (i % 7 != 6 && i % 7 != 2)) {
            black_keys[j] = black_key;
            j += 1;
        }
        
        white_key.x += key_width + key_padding;
        black_key.x += key_width + key_padding;
    }

    for (int i = 0; i < MIDI_LEN; ++i) {
        // @Note: Yes this is linear, we have 15 black keys/22 white keys, this is not a performance bottle-neck as CPUs are fast.
        // Feel free to change it if you have other, better ideas though.
        bool black_key_collision = collides_with_keys(black_keys, MIDI_LEN - 1);
        
        if (CheckCollisionPointRec(GetMousePosition(), white_keys[i]) && !black_key_collision) {
            DrawRectangleRec(white_keys[i], RED);

            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                printf("[WHITE_KEY_%d]\n", i);
            }
        } else {
            DrawRectangleRec(white_keys[i], WHITE);
        }
    }

    for (int i = 0; i < MIDI_LEN - 1; ++i) {
        bool white_key_collision = collides_with_keys(white_keys, MIDI_LEN);
        bool collision = CheckCollisionPointRec(GetMousePosition(), black_keys[i]);

        if (collision || (collision && white_key_collision)) {
            DrawRectangleRec(black_keys[i], RED);

            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                printf("[BLACK_KEY_%d]\n", i);
            }
        } else {
            DrawRectangleRec(black_keys[i], BLACK);
        }
    }
}

internal void render_control_panel(Rectangle rect, int button_padding, int num_buttons)
{
    DrawRectangleRec(rect, { 25, 25, 25, 255 });

    Rectangle button_rect = {0};
    button_rect.width = rect.width - 2.0f*button_padding;
    button_rect.height = 50.0f;
    button_rect.x = rect.x + button_padding;
    button_rect.y = rect.y + button_padding;
    
    Vector2 text_center = {0};
    text_center.x = button_rect.x + button_rect.width/2.0f;
    text_center.y = button_rect.y + button_rect.height/2.0f;

    for (int i = 0; i < num_buttons; ++i) {
        if (CheckCollisionPointRec(GetMousePosition(), button_rect)) {
            DrawRectangleRounded(button_rect, 0.4f, 0, { 70, 70, 70, 255 });

            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                printf("[PRESET_BUTTON]\n");
            }
        } else {
            DrawRectangleRounded(button_rect, 0.4f, 0, { 50, 50, 50, 255 });
        }
        
        draw_text_centered("preset", (int) text_center.x, (int) text_center.y, 23);
        
        button_rect.y += button_rect.height + button_padding;
        text_center.y = button_rect.y + button_rect.height/2.0f;
    }
}

int main(int argc, char **argv)
{
    UNUSED(argc);
    UNUSED(argv);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(WIDTH, HEIGHT, "A Window");
    SetWindowMinSize(MIN_WIDTH, MIN_HEIGHT);    
    SetTargetFPS(FPS);
    
    while (!WindowShouldClose()) {
        const int key_width = (int) (GetScreenWidth() * 0.028f);
        const int key_padding = 5;
        
        Rectangle keyboard_rect = {0};
        keyboard_rect.width = (float) (MIDI_LEN * (key_width + key_padding) - key_padding);
        keyboard_rect.height = 250;
        keyboard_rect.x = 0;
        keyboard_rect.y = GetScreenHeight() - keyboard_rect.height;

        Rectangle control_panel_rect = {0};
        control_panel_rect.width = GetScreenWidth() - keyboard_rect.width;
        control_panel_rect.height = (float) GetScreenHeight();
        control_panel_rect.x = keyboard_rect.width;
        control_panel_rect.y = 0;
    
        Vector2 text_center = {0};
        text_center.x = keyboard_rect.width/2.0f;
        text_center.y = (GetScreenHeight() - keyboard_rect.height)/2.0f;
        
        BeginDrawing();
        ClearBackground({ 20, 20, 20, 255 });
        
        render_keyboard(keyboard_rect, key_width, key_padding);
        render_control_panel(control_panel_rect, 20, 4);
        draw_text_centered("There might be something here soon(tm)", (int) text_center.x, (int) text_center.y, 30);

        EndDrawing();
    }

    CloseWindow();
    
    return 0;
}
