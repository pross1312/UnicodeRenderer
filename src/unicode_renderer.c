#include "unicode_renderer.h"
#include <assert.h>
#include <raylib.h>

#define abort_if_error(ft_error, ...) do {      \
    if ((ft_error) != 0) {              \
        fprintf(stderr, "Error: ");   \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "");        \
    }                                 \
} while(0)

int get_utf8_type(uint8_t x) {
    if (((1 << 7) & x) == 0) return 1;
    else if (((1 << 6) & x) == 0) return -1;
    else if (((1 << 5) & x) == 0) return 2;
    else if (((1 << 4) & x) == 0) return 3;
    else if (((1 << 3) & x) == 0) return 4;
    return -1;
}

uint32_t utf8_to_codepoint(const char *str, int *codepoint_size) {
    const uint8_t *ptr = (uint8_t*)str;
    switch (*codepoint_size = get_utf8_type(*ptr)) {
        case 1:
            return (uint32_t)*ptr;
        case 2:
            return ((*ptr & 0b11111ul)<<6) + (*(ptr+1) & 0b111111ul);
        case 3:
            return ((*ptr & 0b1111ul)<<12) + ((*(ptr+1) & 0b111111ul) << 6) + (*(ptr+2) & 0b111111ul);
        case 4:
            return ((*ptr & 0b111ul)<<18) + ((*(ptr+1) & 0b111111ul) << 12) + ((*(ptr+2) & 0b111111ul) << 6) + (*(ptr+3) & 0b111111ul);
        default:
            return 0;
    }
    return 0;
}

bool UnicodeRenderer_init(UnicodeRenderer *renderer, size_t texture_capacity) {
    int err = FT_Init_FreeType(&renderer->library);
    if (err != 0) {
        TraceLog(LOG_ERROR, "Could not initialize freetype library");
        return false;
    }
    renderer->texture_buffer.capacity = texture_capacity;
    renderer->texture_buffer.count = 0;
    renderer->texture_buffer.current = 0;
    renderer->texture_buffer.items = malloc(sizeof(*renderer->texture_buffer.items) * texture_capacity);
    return true;
}

bool UnicodeRenderer_deinit(UnicodeRenderer renderer) {
    bool success = true;
    int err;
    if (renderer.library != NULL) {
        if (renderer.font_face != NULL) {
            err = FT_Done_Face(renderer.font_face);
            success = err == 0;
            if (err != 0) TraceLog(LOG_ERROR, "[UnicodeRenderer] Could not deinit freetype font face");
        }
        err = FT_Done_FreeType(renderer.library);
        success = err == 0;
        if (err != 0) TraceLog(LOG_ERROR, "[UnicodeRenderer] Could not deinit freetype library");
    }
    return success;
}

bool UnicodeRenderer_load_font(UnicodeRenderer *renderer, const char *font_path, FT_UInt font_pixel_size) {
    int err = FT_New_Face(renderer->library, font_path, 0, &renderer->font_face);
    if (err != 0) {
        TraceLog(LOG_ERROR, "[UnicodeRenderer] Could not load font %s", font_path);
        return false;
    }
    err = FT_Set_Pixel_Sizes(renderer->font_face, 0, font_pixel_size);
    if (err != 0) {
        TraceLog(LOG_ERROR, "[UnicodeRenderer] Could not set font size %s", font_path);
        FT_Done_Face(renderer->font_face);
        return false;
    }
    TraceLog(LOG_INFO, "[UnicodeRenderer] Num faces: %li", renderer->font_face->num_faces);
    TraceLog(LOG_INFO, "[UnicodeRenderer] Face index: %li", renderer->font_face->face_index);
    TraceLog(LOG_INFO, "[UnicodeRenderer] Num glyphs: %li", renderer->font_face->num_glyphs);
    TraceLog(LOG_INFO, "[UnicodeRenderer] Num charmaps: %d", renderer->font_face->num_charmaps);
    TraceLog(LOG_INFO, "[UnicodeRenderer] Num fixed sizes: %d", renderer->font_face->num_fixed_sizes);
    TraceLog(LOG_INFO, "[UnicodeRenderer] Face flag: %li", renderer->font_face->face_flags);
    TraceLog(LOG_INFO, "[UnicodeRenderer] Scalable: %li", renderer->font_face->face_flags & FT_FACE_FLAG_SCALABLE);
    return true;
}

Texture2D UnicodeRenderer_get_texture(UnicodeRenderer *renderer, uint32_t codepoint) {
    // try to find
    for (size_t i = 0; i < renderer->texture_buffer.count; i++) {
        if (renderer->texture_buffer.items[i].codepoint == codepoint) return renderer->texture_buffer.items[i].texture;
    }
    // insert new
    FT_UInt glyph_index = FT_Get_Char_Index(renderer->font_face, codepoint);
    if (glyph_index == 0) {
        TraceLog(LOG_WARNING, "Font does not support glyph %u", codepoint);
        abort();
    }
    int err = FT_Load_Glyph(renderer->font_face, glyph_index, FT_LOAD_RENDER);
    if (err != 0) {
        TraceLog(LOG_ERROR, "Could not load glyph %u at %u", codepoint, glyph_index);
        abort();
    }
    if (renderer->texture_buffer.count == renderer->texture_buffer.capacity) {
        UnloadTexture(renderer->texture_buffer.items[renderer->texture_buffer.current].texture);
    }
    assert(renderer->font_face->glyph->bitmap.width == (uint32_t)renderer->font_face->glyph->bitmap.pitch);
    assert(renderer->font_face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY);
    Image image = {
        .width = renderer->font_face->glyph->bitmap.width,
        .height = renderer->font_face->glyph->bitmap.rows,
        .data = renderer->font_face->glyph->bitmap.buffer,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE,
    };
    renderer->texture_buffer.items[renderer->texture_buffer.current].codepoint = codepoint;
    renderer->texture_buffer.items[renderer->texture_buffer.current].texture = LoadTextureFromImage(image);
    renderer->texture_buffer.current++;
    if (renderer->texture_buffer.current == renderer->texture_buffer.capacity) renderer->texture_buffer.current = 0;
    if (renderer->texture_buffer.count != renderer->texture_buffer.capacity) renderer->texture_buffer.count++;
    return renderer->texture_buffer.items[renderer->texture_buffer.current].texture;
}

void UnicodeRenderer_draw_text(UnicodeRenderer *renderer, const char *text, int x, int y, Color color) {
    int codepoint_size = 0;
    const char *ptr = text;
    int pen_x = x, pen_y = y;
    while (*ptr != '\0') {
        uint32_t codepoint = GetCodepoint(ptr, &codepoint_size);
        if (codepoint == 0x3f) { // could not find
            abort();
        } else {
            Texture2D texture = UnicodeRenderer_get_texture(renderer, codepoint);
            DrawTexture(texture,
                    pen_x + renderer->font_face->glyph->bitmap_left,
                    pen_y + renderer->font_face->glyph->bitmap_top,
                    color);
            pen_x += renderer->font_face->glyph->advance.x >> 6;
            pen_y += renderer->font_face->glyph->advance.y >> 6;
        }
        ptr += codepoint_size;
    }
}

FT_Vector UnicodeRenderer_measure_text(UnicodeRenderer renderer, const char *text) {
    int codepoint_size = 0;
    const char *ptr = text;
    int pen_x = 0, pen_y = 0;
    while (*ptr != '\0') {
        uint32_t codepoint = utf8_to_codepoint(ptr, &codepoint_size);
        if (codepoint == 0) { // could not find
            abort();
        } else {
            FT_UInt glyph_index = FT_Get_Char_Index(renderer.font_face, codepoint);
            if (glyph_index == 0) {
                TraceLog(LOG_WARNING, "Font does not support glyph %u", codepoint);
                abort();
            }
            int err = FT_Load_Glyph(renderer.font_face, glyph_index, FT_LOAD_RENDER);
            if (err != 0) {
                TraceLog(LOG_ERROR, "Could not load glyph %u at %u", codepoint, glyph_index);
                abort();
            }
            pen_x += renderer.font_face->glyph->advance.x >> 6;
            pen_y += renderer.font_face->glyph->advance.y >> 6;
        }
        ptr += codepoint_size;
    }
    pen_y += renderer.font_face->glyph->bitmap_top + renderer.font_face->glyph->bitmap.rows;
    return (FT_Vector) {.x = pen_x, .y = pen_y };
}
