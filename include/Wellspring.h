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

typedef struct Wellspring_Packer Wellspring_Packer;
typedef struct Wellspring_TextBatch Wellspring_TextBatch;

typedef struct Wellspring_FontRange
{
	uint32_t fontSize;
	uint32_t firstCodepoint;
	uint32_t numChars;
	uint8_t oversampleH;
	uint8_t oversampleV;
} Wellspring_FontRange;

typedef struct Wellspring_Color
{
	uint8_t r, g, b, a;
} Wellspring_Color;

typedef struct Wellspring_Vertex
{
	float x, y, z;
	float u, v;
	uint8_t r, g, b, a;
} Wellspring_Vertex;

/* API definition */

WELLSPRINGAPI Wellspring_Packer* Wellspring_CreatePacker(
	const uint8_t *fontBytes,
	uint32_t fontBytesLength,
	uint32_t width,
	uint32_t height,
	uint32_t strideInBytes, /* 0 means the buffer is tightly packed. */
	uint32_t padding /* A sensible value here is 1 to allow bilinear filtering. */
);

WELLSPRINGAPI uint32_t Wellspring_PackFontRanges(
	Wellspring_Packer *packer,
	Wellspring_FontRange *ranges,
	uint32_t numRanges
);

/* Returns a pointer to an array of rasterized pixels of the packed font.
 * This data must be uploaded to a texture before you render!
 * The pixel data becomes outdated if you call PackFontRanges.
 * Length is width * height.
 */
WELLSPRINGAPI uint8_t* Wellspring_GetPixelDataPointer(
	Wellspring_Packer *packer
);

/* Batches are not thread-safe, recommend one batch per thread. */
WELLSPRINGAPI Wellspring_TextBatch* Wellspring_CreateTextBatch();

/* Also restarts the batch */
WELLSPRINGAPI void Wellspring_StartTextBatch(
	Wellspring_TextBatch *textBatch,
	Wellspring_Packer *packer
);

WELLSPRINGAPI uint8_t Wellspring_Draw(
	Wellspring_TextBatch *textBatch,
	float x,
	float y,
	float depth,
	Wellspring_Color *color,
	const uint8_t *strBytes,
	uint32_t strLengthInBytes
);

WELLSPRINGAPI void Wellspring_GetBufferData(
	Wellspring_TextBatch *textBatch,
	Wellspring_Vertex **pVertexBuffer,
	uint32_t *pVertexBufferLengthInBytes,
	uint32_t **pIndexBuffer,
	uint32_t *pIndexBufferLengthInBytes
);

WELLSPRINGAPI void Wellspring_DestroyTextBatch(Wellspring_TextBatch *textBatch);
WELLSPRINGAPI void Wellspring_DestroyPacker(Wellspring_Packer *packer);

/* Function defines */

#ifdef USE_SDL2

#define Wellspring_malloc SDL_malloc
#define Wellspring_realloc SDL_realloc
#define Wellspring_free SDL_free
#define Wellspring_memcpy SDL_memcpy
#define Wellspring_memset SDL_memset
#define Wellspring_ifloor(x) ((int) SDL_floorf(x))
#define Wellspring_iceil(x) ((int) SDL_ceilf(x))
#define Wellspring_sqrt SDL_sqrt
#define Wellspring_pow SDL_pow
#define Wellspring_fmod SDL_fmod
#define Wellspring_cos SDL_cos
#define Wellspring_acos SDL_acos
#define Wellspring_fabs SDL_fabs
#define Wellspring_assert SDL_assert
#define Wellspring_strlen SDL_strlen

#else

#define Wellspring_malloc malloc
#define Wellspring_realloc realloc
#define Wellspring_free free
#define Wellspring_memcpy memcpy
#define Wellspring_ifloor(x) ((int) floor(x))
#define Wellspring_iceil(x) ((int) ceil(x))
#define Wellspring_sqrt sqrt
#define Wellspring_pow pow
#define Wellspring_fmod fmod
#define Wellspring_cos cos
#define Wellspring_acos acos
#define Wellspring_fabs fabs
#define Wellspring_assert assert
#define Wellspring_strlen strlen

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* WELLSPRING_H */
