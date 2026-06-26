#ifndef SURGE_UI_FONT_ZH_18_H
#define SURGE_UI_FONT_ZH_18_H

#include "surge_ui_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SURGE_UI_ZH_GLYPH_T_DEFINED
#define SURGE_UI_ZH_GLYPH_T_DEFINED
typedef struct
{
    rt_uint16_t codepoint;
    const rt_uint8_t *bitmap;
} surge_ui_zh_glyph_t;
#endif

#define SURGE_UI_ZH_18_FONT_W 18
#define SURGE_UI_ZH_18_FONT_H 18
#define SURGE_UI_ZH_18_FONT_BYTES_PER_ROW 9
#define SURGE_UI_ZH_18_FONT_BITS_PER_PIXEL 4
#define SURGE_UI_ZH_18_GLYPH_COUNT 110

extern const surge_ui_zh_glyph_t surge_ui_zh_18_glyphs[];
extern const rt_uint16_t surge_ui_zh_18_glyph_count;

#ifdef __cplusplus
}
#endif

#endif
