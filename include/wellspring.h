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

#ifndef WELLSPRING_H
#define WELLSPRING_H

#ifdef _WIN32
#define WELLSPRINGAPI __declspec(dllexport)
#define WELLSPRINGCALL __cdecl
#else
#define WELLSPRINGAPI
#define WELLSPRINGCALL
#endif

#include <stdint.h>

#ifdef USE_SDL2
#include <SDL.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* Version API */

#define WELLSPRING_MAJOR_VERSION 0
#define WELLSPRING_MINOR_VERSION 1
#define WELLSPRING_PATCH_VERSION 0

#define WELLSPRING_COMPILED_VERSION ( \
	(WELLSPRING_MAJOR_VERSION * 100 * 100) + \
	(WELLSPRING_MINOR_VERSION * 100) + \
	(WELLSPRING_PATCH_VERSION) \
)

WELLSPRINGAPI uint32_t Wellspring_LinkedVersion(void);

/* Type definitions */

typedef struct Wellspring_Font Wellspring_Font;
typedef struct Wellspring_Packer Wellspring_Packer;

typedef struct Wellspring_FontRange
{
	uint32_t fontSize;
	uint32_t firstCodepoint;
	uint32_t numChars;
	uint8_t oversampleH;
	uint8_t oversampleV;
} Wellspring_FontRange;

typedef struct Wellspring_GlyphQuad
{
	float x0, y0, s0, t0; // top-left
	float x1, y1, s1, t1; // bottom-right;
} Wellspring_GlyphQuad;

/* API definition */

WELLSPRINGAPI Wellspring_Font* Wellspring_LoadFont(
	const uint8_t *fontData, /* can be freed after */
	uint32_t fontDataLength
);

WELLSPRINGAPI void Wellspring_GetFontMetrics(
	Wellspring_Font *font,
	int32_t *pAscent,
	int32_t *pDescent,
	int32_t *pLineGap
);

WELLSPRINGAPI int32_t Wellspring_FindGlyphIndex(
	Wellspring_Font *font,
	int32_t codepoint
);

WELLSPRINGAPI void Wellspring_GetGlyphMetrics(
	Wellspring_Font *font,
	int32_t glyphIndex,
	int32_t *pAdvance,
	int32_t *pLeftSideBearing
);

WELLSPRINGAPI int32_t Wellspring_GetGlyphKernAdvance(
	Wellspring_Font *font,
	int32_t glyphIndexA,
	int32_t glyphIndexB
);

WELLSPRINGAPI Wellspring_Packer* Wellspring_CreatePacker(
	uint8_t *pixels,
	uint32_t width,
	uint32_t height,
	uint32_t strideInBytes,
	uint32_t padding
);

WELLSPRINGAPI uint32_t Wellspring_PackFontRanges(
	Wellspring_Packer *packer,
	Wellspring_Font *font,
	Wellspring_FontRange *ranges,
	uint32_t numRanges
);

WELLSPRINGAPI uint8_t* Wellspring_GetPixels(
	Wellspring_Packer *packer
);

WELLSPRINGAPI uint32_t Wellspring_GetGlyphQuad(
	Wellspring_Packer *packer,
	int32_t glyphIndex,
	Wellspring_GlyphQuad *pGlyphQuad
);

WELLSPRINGAPI void Wellspring_DestroyPacker(Wellspring_Packer *packer);
WELLSPRINGAPI void Wellspring_DestroyFont(Wellspring_Font *font);

/* Function defines */

#ifdef USE_SDL2

#define Wellspring_malloc SDL_malloc
#define Wellspring_free SDL_free
#define Wellspring_memcpy SDL_memcpy
#define Wellspring_ifloor(x) ((int) SDL_floorf(x))
#define Wellspring_iceil(x) ((int) SDL_ceilf(x))
#define Wellspring_sqrt SDL_sqrt
#define Wellspring_pow SDL_pow

/* FIXME: finish these defines */

#else

#define Wellspring_malloc malloc
#define Wellspring_free free
#define Wellspring_memcpy memcpy
#define Wellspring_ifloor(x) ((int) floor(x))
#define Wellspring_iceil(x) ((int) ceil(x))
#define Wellspring_sqrt sqrt
#define Wellspring_pow pow

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* WELLSPRING_H */
