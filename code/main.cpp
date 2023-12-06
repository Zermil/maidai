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

#define KEY_ZERO 48

internal void draw_text_centered(const char *text, int x, int y, int font_size)
{
    int text_width = MeasureText(text, font_size);
    DrawText(text, x - text_width/2, y - font_size/2, font_size, WHITE);
}

// @Note: By default we render 'regular' ffxiv keyboard, at some point
// in the future we might generalize it to more/less keys.
internal Vector2 render_keyboard()
{
    const int h = GetScreenHeight();
    
    constexpr int key_height = 240;
    constexpr int key_width = 35;
    
    constexpr int key_padding = 6;

    for (int i = 0; i < MIDI_LEN; ++i) {
        int key_offset = i * (key_width + key_padding);
        DrawRectangle(key_offset, h - key_height, key_width, key_height, WHITE);
    }

    for (int i = 0; i < MIDI_LEN - 1; ++i) {
        // @Robustness: Replace this, it's just ugly.
        if (i % 7 == 6 || i % 7 == 2) continue;
        
        int key_offset = i * (key_width + key_padding) + (key_width + key_padding)/2;
        DrawRectangle(key_offset, h - key_height, key_width, key_height/2, BLACK);
    }

    // @Note: How much space are we taking up. Not the best solution
    // but pretty straight-forward and approachable.
    Vector2 occupied = {0};
    occupied.x = MIDI_LEN * (key_width + key_padding) - key_padding;
    occupied.y = key_height;

    return(occupied);
}

internal void render_control_panel(int start_x)
{
    const int full_width = GetScreenWidth() - start_x;
    const float rect_padding = 15.0f;
    
    Rectangle rect = {0};
    rect.x = start_x + 20.0f;
    rect.y = rect_padding;
    rect.width = full_width - 40.0f;
    rect.height = 50.0f;

    DrawRectangle(start_x, 0, full_width, GetScreenHeight(), { 25, 25, 25, 255 });

    for (int i = 0; i < 4; ++i) {
        DrawRectangleRounded(rect, 0.4f, 0, { 50, 50, 50, 255 });
        draw_text_centered("preset", (int) (rect.x + rect.width/2), (int) (rect.y + rect.height/2), 25);
        
        rect.y += rect.height + rect_padding;
    }
}

int main(int argc, char **argv)
{
    UNUSED(argc);
    UNUSED(argv);

    InitWindow(WIDTH, HEIGHT, "A Window");
    SetTargetFPS(FPS);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground({ 20, 20, 20, 255 });
        
        Vector2 kb = render_keyboard();
        render_control_panel((int) kb.x);
        
        draw_text_centered("There might be something here soon(tm)", (int) kb.x/2, (int) (GetScreenHeight() - kb.y)/2, 33);

        EndDrawing();
    }

    CloseWindow();
    
    return 0;
}
