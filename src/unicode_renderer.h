#ifndef UNICODE_RENDERER
#define UNICODE_RENDERER

#include <stdint.h>
#include <raylib.h>

#include <ft2build.h>
#include FT_FREETYPE_H

typedef struct {
    uint32_t codepoint;
    Texture2D texture;
} CodepointTexture;

typedef struct {
    size_t capacity;
    size_t count;
    size_t current;
    CodepointTexture *items;
} CodepointTextureRingBuffer;

typedef struct {
    FT_Library library;
    FT_Face font_face;
    CodepointTextureRingBuffer texture_buffer;
} UnicodeRenderer;

bool UnicodeRenderer_init(UnicodeRenderer *renderer, size_t texture_capacity);
bool UnicodeRenderer_deinit(UnicodeRenderer renderer);
bool UnicodeRenderer_load_font(UnicodeRenderer *renderer, const char *font_path, FT_UInt font_pixel_size);
void UnicodeRenderer_draw_text(UnicodeRenderer *renderer, const char *text, int x, int y, Color color);
Texture2D UnicodeRenderer_get_texture(UnicodeRenderer *renderer, uint32_t codepoint);
FT_Vector UnicodeRenderer_measure_text(UnicodeRenderer renderer, const char *text);

int get_utf8_type(uint8_t x);
uint32_t utf8_to_codepoint(const char *str, int *codepoint_size);

#endif //UNICODE_RENDERER
