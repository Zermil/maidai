#include <stdio.h>
#include <stdlib.h>

#include <raylib/raylib.h>
#include <raylib/raymath.h>

#define UNUSED(x) ((void)(x))

#include "./windows_platform.h"
#define NOTE_ON 0x90
#define NOTE_OFF 0x80

#define internal static
#define global static

#define WIDTH 1280
#define HEIGHT 720
#define MIN_WIDTH 1000
#define MIN_HEIGHT 700
#define FPS 60

#define MIDI_LEN 22 // @Note: Length in white keys
#define MIDI_FULL_LEN 37

struct Note {
    Rectangle rect;
    int note_number;
};

global bool g_highlighted_notes[MIDI_FULL_LEN] = {0};
global const char *g_log_message = 0;
global bool g_midi_device_started = false;
global MMRESULT g_open_device_result = MMSYSERR_BADDEVICEID;

internal void draw_text_centered(const char *text, int x, int y, int font_size)
{
    int text_width = MeasureText(text, font_size);
    DrawText(text, x - text_width/2, y - font_size/2, font_size, WHITE);
}

// @Note: Yes this is linear, we have 15 black keys/22 white keys, this is not a performance bottle-neck as CPUs are fast.
// Feel free to change it if you have other, better ideas though.
internal bool collides_with_keys(Note *keys, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        if (CheckCollisionPointRec(GetMousePosition(), keys[i].rect)) {
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
    Note black_keys[MIDI_LEN - 1] = {0};
    Note white_keys[MIDI_LEN] = {0};
    
    for (int i = 0, note_number = 0; i < MIDI_LEN; ++i) {
        white_keys[i].rect = white_key;
        white_keys[i].note_number = note_number;
        note_number += 1;
        
        // @Robustness: Change this, it's just ugly.
        if (i != MIDI_LEN - 1 && (i % 7 != 6 && i % 7 != 2)) {
            black_keys[i].rect = black_key;
            black_keys[i].note_number = note_number;
            note_number += 1;
        }
        
        white_key.x += key_width + key_padding;
        black_key.x += key_width + key_padding;
    }

    for (int i = 0; i < MIDI_LEN; ++i) {
        bool black_key_collision = collides_with_keys(black_keys, MIDI_LEN - 1);
        
        if (CheckCollisionPointRec(GetMousePosition(), white_keys[i].rect) && !black_key_collision) {
            DrawRectangleRec(white_keys[i].rect, RED);

            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                printf("[WHITE_KEY_%d]\n", white_keys[i].note_number);
            }
        } else {
            bool highlight = g_highlighted_notes[white_keys[i].note_number];
            DrawRectangleRec(white_keys[i].rect, highlight ? GREEN : WHITE);
        }
    }

    for (int i = 0; i < MIDI_LEN - 1; ++i) {
        bool white_key_collision = collides_with_keys(white_keys, MIDI_LEN);
        bool collision = CheckCollisionPointRec(GetMousePosition(), black_keys[i].rect);

        if (collision || (collision && white_key_collision)) {
            DrawRectangleRec(black_keys[i].rect, RED);

            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                printf("[BLACK_KEY_%d]\n", black_keys[i].note_number);
            }
        } else {
            bool highlight = g_highlighted_notes[black_keys[i].note_number];
            DrawRectangleRec(black_keys[i].rect, highlight ? GREEN : BLACK);
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

// @Note: This thing is so poorly document it's like John Microsoft doesn't want us
// to develop things for their system.
internal void CALLBACK midi_callback(HMIDIIN handle, UINT msg, DWORD_PTR instance, DWORD_PTR arg0, DWORD_PTR arg1)
{    
    UNUSED(arg1);
    UNUSED(handle);
    UNUSED(instance);

    // @ToDo: Device won't send MIM_CLOSE message when disconnected
    
    if (msg == MIM_DATA) {
        unsigned int midi_message = arg0 & 0xFF;
        unsigned int note = (arg0 >> 8) & 0xFF;
    
        int index = note - 48;
        if (midi_message == NOTE_ON) {
            if (index >= 0 && index < MIDI_FULL_LEN) {
                g_log_message = "Highlighting note";
                g_highlighted_notes[index] = true;
            } else {
                g_log_message = "Current note is outside the visible range";
            }
        } else {
            if (index >= 0 && index < MIDI_FULL_LEN) {
                g_highlighted_notes[index] = false;
            }
        }
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

    HMIDIIN handle = 0;
    g_open_device_result = midiInOpen(&handle, 0, (DWORD_PTR) midi_callback, 0, CALLBACK_FUNCTION);
    
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

        if (g_open_device_result != MMSYSERR_NOERROR) {
            g_log_message = "Could not find MIDI device";
            
            g_open_device_result = midiInOpen(&handle, 0, (DWORD_PTR) midi_callback, 0, CALLBACK_FUNCTION);
        } else if (g_midi_device_started == false) {
            g_log_message = "MIDI device connected!";
            
            midiInStart(handle);
            g_midi_device_started = true;
        }
        
        draw_text_centered(g_log_message, (int) text_center.x, (int) text_center.y, 30);

        EndDrawing();
    }

    midiInStop(handle);
    midiInClose(handle);
    CloseWindow();
    
    return 0;
}
