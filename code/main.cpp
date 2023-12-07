#include <stdio.h>
#include <stdlib.h>

#include <raylib/raylib.h>
#include <raylib/raymath.h>

#define UNUSED(x) ((void)(x))

#define internal static
#define global static

#define WIDTH 1280
#define HEIGHT 720
#define FPS 60

#define MIDI_LEN 22

internal void draw_text_centered(const char *text, int x, int y, int font_size)
{
    int text_width = MeasureText(text, font_size);
    DrawText(text, x - text_width/2, y - font_size/2, font_size, WHITE);
}

// @Note: By default we render 'regular/extended' ffxiv keyboard, at some point
// in the future we might generalize it to more/less keys.
internal void render_keyboard(Rectangle rect, int key_width, int key_padding)
{
    for (int i = 0; i < MIDI_LEN; ++i) {
        int key_offset = i*(key_width + key_padding);
        DrawRectangle(key_offset, (int) rect.y, key_width, (int) rect.height, WHITE);
    }

    for (int i = 0; i < MIDI_LEN - 1; ++i) {
        // @Robustness: Change this, it's just ugly.
        if (i % 7 == 6 || i % 7 == 2) continue;

        int half_white_width = (key_width + key_padding)/2;
        int key_offset = half_white_width + i*(key_width + key_padding);
        
        DrawRectangle(key_offset, (int) rect.y, key_width, (int) rect.height/2, BLACK);
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
        DrawRectangleRounded(button_rect, 0.4f, 0, { 50, 50, 50, 255 });
        draw_text_centered("preset", (int) text_center.x, (int) text_center.y, 25);
        
        button_rect.y += button_rect.height + button_padding;
        text_center.y = button_rect.y + button_rect.height/2.0f;
    }
}

int main(int argc, char **argv)
{
    UNUSED(argc);
    UNUSED(argv);

    InitWindow(WIDTH, HEIGHT, "A Window");
    SetTargetFPS(FPS);

    const int key_width = 35;
    const int key_padding = 5;

    Rectangle keyboard_rect = {0};
    keyboard_rect.width = MIDI_LEN * (key_width + key_padding) - key_padding;
    keyboard_rect.height = 240;
    keyboard_rect.x = 0;
    keyboard_rect.y = HEIGHT - keyboard_rect.height;

    Rectangle control_panel_rect = {0};
    control_panel_rect.width = WIDTH - keyboard_rect.width;
    control_panel_rect.height = HEIGHT;
    control_panel_rect.x = keyboard_rect.width;
    control_panel_rect.y = 0;
    
    Vector2 text_center = {0};
    text_center.x = keyboard_rect.width/2.0f;
    text_center.y = (HEIGHT - keyboard_rect.height)/2.0f;
    
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground({ 20, 20, 20, 255 });
        
        render_keyboard(keyboard_rect, key_width, key_padding);
        render_control_panel(control_panel_rect, 20, 4);
        draw_text_centered("There might be something here soon(tm)", (int) text_center.x, (int) text_center.y, 33);

        EndDrawing();
    }

    CloseWindow();
    
    return 0;
}
