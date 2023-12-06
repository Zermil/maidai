#include <stdio.h>
#include <stdlib.h>

#include <raylib/raylib.h>

#define UNUSED(x) ((void)(x))

#define WIDTH 1280
#define HEIGHT 720
#define FPS 60

int main(int argc, char **argv)
{
    UNUSED(argc);
    UNUSED(argv);
    
    InitWindow(WIDTH, HEIGHT, "A Window");
    SetTargetFPS(FPS);
    
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground({ 18, 18, 18, 255 });
        EndDrawing();
    }

    CloseWindow();
    
    return 0;
}
