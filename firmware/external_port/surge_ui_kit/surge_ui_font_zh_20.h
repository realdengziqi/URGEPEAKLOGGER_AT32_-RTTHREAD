#ifndef SURGE_UI_FONT_ZH_20_H
#define SURGE_UI_FONT_ZH_20_H

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

#define SURGE_UI_ZH_20_FONT_W 20
#define SURGE_UI_ZH_20_FONT_H 20
#define SURGE_UI_ZH_20_FONT_BYTES_PER_ROW 10
#define SURGE_UI_ZH_20_FONT_BITS_PER_PIXEL 4
#define SURGE_UI_ZH_20_GLYPH_COUNT 110

extern const surge_ui_zh_glyph_t surge_ui_zh_20_glyphs[];
extern const rt_uint16_t surge_ui_zh_20_glyph_count;

#ifdef __cplusplus
}
#endif

#endif
