/* Wellspring - A very simple font rendering system
 *
 * Copyright (c) 2022 Evan Hemsley
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software in a
 * product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * Evan "cosmonaut" Hemsley <evan@moonside.games>
 *
 */

#include "wellspring.h"
#include "stb_rect_pack.h"

#define STBTT_malloc(x,u) ((void)(u),Wellspring_malloc(x))
#define STBTT_free(x,u) ((void)(u),Wellspring_free(x))
#define STBTT_ifloor Wellspring_ifloor
#define STBTT_iceil Wellspring_iceil
#define STBTT_sqrt Wellspring_sqrt
#define STBTT_pow Wellspring_pow

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#pragma GCC diagnostic ignored "-Wunused-function"
#include "stb_truetype.h"

typedef struct Font
{
	uint8_t *fontData;
	stbtt_fontinfo stbFontInfo;
} Font;

uint32_t Wellspring_LinkedVersion(void)
{
	return WELLSPRING_COMPILED_VERSION;
}

Wellspring_Font* Wellspring_CreateFont(
	const uint8_t *fontData,
	uint32_t fontDataLength
) {
	Font *font;

	font = Wellspring_malloc(sizeof(Font));
	Wellspring_memcpy(font->fontData, fontData, fontDataLength);
	stbtt_InitFont(&font->stbFontInfo, font->fontData, 0);

	return (Wellspring_Font*) font;
}

void Wellspring_GetFontMetrics(
	Wellspring_Font *font,
	int32_t *pAscent,
	int32_t *pDescent,
	int32_t *pLineGap
) {
	Font *myFont = (Font*) font;
	stbtt_GetFontVMetrics(&myFont->stbFontInfo, pAscent, pDescent, pLineGap);
}

int32_t Wellspring_FindGlyphIndex(
	Wellspring_Font *font,
	int32_t codepoint
) {
	Font *myFont = (Font*) font;
	return stbtt_FindGlyphIndex(&myFont->stbFontInfo, codepoint);
}

void Wellspring_GetGlyphMetrics(
	Wellspring_Font *font,
	int32_t glyphIndex,
	int32_t *pAdvance,
	int32_t *pLeftSideBearing
) {
	Font *myFont = (Font*) font;
	stbtt_GetGlyphHMetrics(&myFont->stbFontInfo, glyphIndex, pAdvance, pLeftSideBearing);
}

int32_t Wellspring_GetGlyphKernAdvance(
	Wellspring_Font *font,
	int32_t glyphIndexA,
	int32_t glyphIndexB
) {
	Font *myFont = (Font*) font;
	stbtt_GetGlyphKernAdvance(&myFont->stbFontInfo, glyphIndexA, glyphIndexB);
}

Wellspring_Packer* Wellspring_CreatePacker(
	uint8_t *pixels,
	uint32_t width,
	uint32_t height,
	uint32_t strideInBytes,
	uint32_t padding
) {

}

uint32_t Wellspring_PackFontRanges(
	Wellspring_Packer *packer,
	Wellspring_Font *font,
	Wellspring_FontRange *ranges,
	uint32_t numRanges
) {

}

uint8_t* Wellspring_GetPixels(
	Wellspring_Packer *packer
) {

}

uint32_t Wellspring_GetGlyphQuad(
	Wellspring_Packer *packer,
	int32_t glyphIndex,
	Wellspring_GlyphQuad *pGlyphQuad
) {

}

void Wellspring_DestroyPacker(Wellspring_Packer *packer)
{

}

void Wellspring_DestroyFont(
	Wellspring_Font *font
) {
	Font* myFont = (Font*) font;

	Wellspring_free(myFont->fontData);
	Wellspring_free(myFont);
}
