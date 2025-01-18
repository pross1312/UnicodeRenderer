#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "nob.h"
#include "unicode_renderer.h"
#include "raylib.h"

void usage(const char *program) {
    fprintf(stderr, "Usage: %s <font_file> \"<text>\"\n", program);
}

int main(int argc, const char **argv) {
    const char *program = nob_shift(argv, argc);
    if (argc != 2) {
        usage(program);
        return 1;
    }
    const char *font_file_path = nob_shift(argv, argc);
    const char *text = nob_shift(argv, argc);
    InitWindow(600, 600, "Unicode rendering");
    printf("*************************************************************\n");

    UnicodeRenderer ur = {0};
    if (!UnicodeRenderer_init(&ur, 32)) return 1;
    if (!UnicodeRenderer_load_font(&ur, font_file_path, 32)) return 1;
    int width = GetScreenWidth();
    int height = GetScreenHeight();
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        FT_Vector vector = UnicodeRenderer_measure_text(ur, text);

        UnicodeRenderer_draw_text(&ur, text, (width - vector.x)/2, (height - vector.y)/2, WHITE);

        EndDrawing();
    }
    UnicodeRenderer_deinit(ur);
    CloseWindow();
    return 0;
}
