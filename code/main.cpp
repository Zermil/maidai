#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <raylib/raylib.h>
#include <raylib/raymath.h>

#include "./font.h"
#include "./vk.h"

#define UNUSED(x) ((void)(x))
#define ARR_SZ(arr) (sizeof(arr)/sizeof(arr[0]))

// @Note: Please, if anyone has a better solution for this _without namespaces_
// I'll gladly take it
#if defined(_WIN32)
#define NOMINMAX
#define NODRAWTEXT
#define NOGDI
#define WIN32_LEAN_AND_MEAN

#define CloseWindow win32_close_window
#define ShowCursor win32_show_cursor

#include <windows.h>
#include <mmsystem.h>

#undef CloseWindow
#undef ShowCursor
#endif // _WIN32

#define NOTE_ON 0x90
#define NOTE_OFF 0x80
#define NOTE_OFFSET 48

#define internal static
#define global static

#define WIDTH 1280
#define HEIGHT 720
#define MIN_WIDTH 1100
#define MIN_HEIGHT 700
#define FPS 60

#define WHITE_KEYS_LEN 22
#define BLACK_KEYS_LEN 15
#define MIDI_FULL_LEN (WHITE_KEYS_LEN + BLACK_KEYS_LEN)

struct Note {
    Rectangle rect;
    Color color;
    
    int note_number;
    bool hovered; // @Robustness: We should find a way to get rid of this boolean
};

struct Config {
    const char *name;
    int keys_map[MIDI_FULL_LEN];    
};

struct Internal_State {
    const char *log_message;

    int active_key = -1; // @Note: Means no active key at startup
    
    bool highlighted_notes[MIDI_FULL_LEN];
    int midi_keys_map[MIDI_FULL_LEN];

    bool device_connected;
    HMIDIIN midi_handle;

    Font font;
};

// @Note: For all new programmers, I'm sorry but real life isn't how your CS professor wants it to be.
// In real life you deal with globals and that's fine, as long as you know how to handle them and who
// and when is going to touch them.
global Internal_State state = {0};

internal void draw_text_centered(const char *text, int x, int y, float font_size, Color color)
{
    Vector2 text_width = MeasureTextEx(state.font, text, (float) font_size, 1.0f);
    DrawTextEx(state.font, text, { x - text_width.x/2.0f, y - font_size/2.0f }, font_size, 1.0f, color);
}

// @Robustness: Find a better way of figuring out the colour, possible
// refactor of 'render_keyboard()'
internal Color get_colour_from_state(int note_number, Color color)
{
    if (state.highlighted_notes[note_number]) return(GREEN);
    if (state.active_key == note_number) return(ORANGE);

    return(color);
}

internal void load_keyboard_config(size_t config_id)
{
    for (size_t i = 0; i < MIDI_FULL_LEN; ++i) {
        state.midi_keys_map[i] = 0;
    }
    
    switch (config_id) {
        case 0: {
            const char *keys = "Q2W3ER5T6Y7UI";
            
            for (size_t i = 0; i < strlen(keys); ++i) {
                state.midi_keys_map[i + 12] = keys[i];
            }
        } break;

        case 1: {
            const char *keys = "QWERTYUASDFGHJZXCVBNM";
            size_t indices[] = { 0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19, 21, 23, 24, 26, 28, 29, 31, 33, 35, 36 };
 
            for (size_t i = 0; i < ARR_SZ(indices); ++i) {
                state.midi_keys_map[indices[i]] = keys[i];
            }
        } break;
    }
}

internal void render_set_of_keys(Rectangle rect, Note *keys, size_t size, int key_width, int key_padding)
{
    Rectangle tooltip = {0};
    tooltip.width = key_width - key_padding*2.0f;
    tooltip.height = rect.height * 0.25f;
    
    for (int i = 0; i < size; ++i) {        
        Color c = keys[i].color;
        
        if (keys[i].hovered) {
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                state.active_key = keys[i].note_number;
                state.log_message = "Press ASCII key to finish mapping";
            }
            
            if (!state.highlighted_notes[keys[i].note_number]) {
                c = RED;
            }
        }
        
        DrawRectangleRec(keys[i].rect, c);

        if (state.midi_keys_map[keys[i].note_number] != 0) {
            tooltip.x = keys[i].rect.x + key_padding;
            tooltip.y = rect.y + keys[i].rect.height - tooltip.height - key_padding;
            DrawRectangleRounded(tooltip, 0.4f, 0, { 50, 50, 50, 255 });

            Vector2 text_center = {0};
            text_center.x = tooltip.x + tooltip.width / 2.0f;
            text_center.y = tooltip.y + tooltip.height / 2.0f;

            int index = state.midi_keys_map[keys[i].note_number];
            
            if (strlen(vk_translation[index]) > 2) {
                // @Note: snprintf() causes weird behaviour that I don't want to investigate right now,
                // plus this approach is fine here.
                char text[3] = { vk_translation[index][0], vk_translation[index][1], 0 };
                draw_text_centered(text, (int) text_center.x, (int) text_center.y, 28, WHITE);
            } else {
                draw_text_centered(vk_translation[index], (int) text_center.x, (int) text_center.y, 28, WHITE);
            }
        }
    }
}

// @Note: By default we render 'regular/extended' ffxiv keyboard.
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

    // @Note: I needed to decouple rendering from logic
    // handling, because rectangles overlap each-other.
    // I wanted to solve this problem with z-index or something similar
    // but raylib (afaik) doesn't provide anything like that (nor layers etc.)
    Note white_keys[WHITE_KEYS_LEN] = {0};
    Note black_keys[BLACK_KEYS_LEN] = {0};
    bool hovered = false;
    
    for (int i = 0, j = 0, note_number = 0; i < WHITE_KEYS_LEN; ++i) {
        white_keys[i].rect = white_key;
        white_keys[i].note_number = note_number;
        white_keys[i].color = get_colour_from_state(note_number, WHITE);
        note_number += 1;
        
        // @Robustness: Change this, it's just ugly.
        if (i != WHITE_KEYS_LEN - 1 && (i % 7 != 6 && i % 7 != 2)) {
            black_keys[j].rect = black_key;
            black_keys[j].note_number = note_number;
            black_keys[j].color = get_colour_from_state(note_number, BLACK);
            note_number += 1;

            // @Note: This is some of the worst code I've ever written...
            if (!hovered && CheckCollisionPointRec(GetMousePosition(), black_keys[j].rect)) {
                hovered = true;
                black_keys[j].hovered = true;
            }
            
            j += 1;
        }

        if (!hovered && CheckCollisionPointRec(GetMousePosition(), white_keys[i].rect)) {
            hovered = true;
            white_keys[i].hovered = true;
        }
        
        white_key.x += key_width + key_padding;
        black_key.x += key_width + key_padding;
    }

    render_set_of_keys(rect, white_keys, WHITE_KEYS_LEN, key_width, key_padding);
    render_set_of_keys(rect, black_keys, BLACK_KEYS_LEN, key_width, key_padding);
}

internal void render_control_panel(Rectangle rect, int button_padding)
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

    const char *presets[] = { "default", "genshin", "custom_1", "custom_2" };
    
    for (size_t i = 0; i < ARR_SZ(presets); ++i) {
        if (CheckCollisionPointRec(GetMousePosition(), button_rect)) {
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                load_keyboard_config(i);
                state.active_key = -1;
                state.log_message = "Loaded config";
            }
            
            DrawRectangleRounded(button_rect, 0.4f, 0, { 70, 70, 70, 255 });
        } else {
            DrawRectangleRounded(button_rect, 0.4f, 0, { 50, 50, 50, 255 });
        }

        draw_text_centered(presets[i], (int) text_center.x, (int) text_center.y, 32, WHITE);
        
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
    
    if (msg != MIM_DATA) return;
    
    unsigned int midi_message = arg0 & 0xFF;
    unsigned int note = (arg0 >> 8) & 0xFF;
    
    int index = note - NOTE_OFFSET;
    if (midi_message == NOTE_ON) {
        if (index >= 0 && index < MIDI_FULL_LEN) {
            state.highlighted_notes[index] = true;

            if (state.midi_keys_map[index] != 0) {
                INPUT input = {0};
                input.type = INPUT_KEYBOARD;
                input.ki.wVk = (WORD) state.midi_keys_map[index];
                
                SendInput(1, &input, sizeof(INPUT));
                input.ki.dwFlags |= KEYEVENTF_KEYUP;
                SendInput(1, &input, sizeof(INPUT));
            }
        } else {
            state.log_message = "Note is outside the visible range";
        }
    } else if (midi_message == NOTE_OFF) {
        if (index >= 0 && index < MIDI_FULL_LEN) {
            state.highlighted_notes[index] = false;
        }
    }
}

internal void check_midi_controller()
{
    // @ToDo: We're selecting the first connected device, we should
    // give the user an option to select which device they want to
    // select/map.
        
    // @ToDo: Proper error messages based on MMSYSERR.
    MIDIINCAPS midi_info = {0};
    MMRESULT device_result = midiInGetDevCaps(0, &midi_info, sizeof(MIDIINCAPS));
    
    if (device_result == MMSYSERR_NOERROR && !state.device_connected) {
        device_result = midiInOpen(&state.midi_handle, 0, (DWORD_PTR) midi_callback, 0, CALLBACK_FUNCTION);
        
        if (device_result == MMSYSERR_NOERROR) {
            state.device_connected = true;
            midiInStart(state.midi_handle);
        }
    } else if (device_result != MMSYSERR_NOERROR) {
        state.device_connected = false;

        for (size_t i = 0; i < MIDI_FULL_LEN; ++i) {
            state.highlighted_notes[i] = 0;
        }
        
        midiInStop(state.midi_handle);
        midiInClose(state.midi_handle);
    }
}

internal void check_key_assignment()
{
    if (IsKeyPressed(KEY_ESCAPE) && state.active_key != -1) {
        if (state.midi_keys_map[state.active_key] != 0) {
            state.log_message = "Key unmapped";
            state.midi_keys_map[state.active_key] = 0;
        } else {
            state.log_message = "Mapping stopped";
        }
        
        state.active_key = -1;
    } else if (state.active_key != -1) {
        int key_code = -1;

        // @Note: We're starting from 0x08 because previous values
        // map to mouse input.
        for (int i = 8; i < 256; ++i) {
            if (GetAsyncKeyState(i) & 0x8000) {
                key_code = i;
                break;
            }
        }

        if (key_code != -1) {
            state.midi_keys_map[state.active_key] = key_code;
            state.active_key = -1;
            state.log_message = "Key mapped";
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
    SetExitKey(0);
    
    SetTargetFPS(FPS);

    load_keyboard_config(0);
    
    state.font = LoadFontFromMemory(".otf", g_font, g_font_size, 128, 0, 0);
    SetTextureFilter(state.font.texture, TEXTURE_FILTER_BILINEAR);
    state.log_message = "Select a piano key to begin mapping";
    
    while (!WindowShouldClose()) {
        const int key_width = (int) (GetScreenWidth() * 0.032f);
        const int key_padding = 5;
        
        Rectangle keyboard_rect = {0};
        keyboard_rect.width = (float) (WHITE_KEYS_LEN * (key_width + key_padding) - key_padding);
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

        check_key_assignment();
        check_midi_controller();

        BeginDrawing();
        ClearBackground({ 20, 20, 20, 255 });
        
        render_keyboard(keyboard_rect, key_width, key_padding);
        render_control_panel(control_panel_rect, 20);

        draw_text_centered(state.log_message, (int) text_center.x, (int) text_center.y, 42, WHITE);

        if (state.device_connected) {
            DrawTextEx(state.font, "MIDI device connected", { 10, 10 }, 32, 1.0f, GREEN);
        } else {
            DrawTextEx(state.font, "MIDI device not connected", { 10, 10 }, 32, 1.0f, RED);
        }

        EndDrawing();
    }

    midiInStop(state.midi_handle);
    midiInClose(state.midi_handle);
    UnloadFont(state.font);
    CloseWindow();
    
    return 0;
}
