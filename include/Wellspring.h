/* Wellspring - An immediate mode font rendering system in C
 *
 * Copyright (c) 2022-2024 Evan Hemsley
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

#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_log.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* Version API */

#define WELLSPRING_MAJOR_VERSION 1
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
typedef struct Wellspring_TextBatch Wellspring_TextBatch;

typedef struct Wellspring_FontRange
{
	uint32_t firstCodepoint;
	uint32_t numChars;
	uint8_t oversampleH;
	uint8_t oversampleV;
} Wellspring_FontRange;

typedef struct Wellspring_Vertex
{
	float x, y;
	float u, v;
	uint32_t chunkIndex;
} Wellspring_Vertex;

typedef struct Wellspring_Rectangle
{
	float x;
	float y;
	float w;
	float h;
} Wellspring_Rectangle;

typedef enum Wellspring_HorizontalAlignment
{
	WELLSPRING_HORIZONTALALIGNMENT_LEFT,
	WELLSPRING_HORIZONTALALIGNMENT_CENTER,
	WELLSPRING_HORIZONTALALIGNMENT_RIGHT
} Wellspring_HorizontalAlignment;

typedef enum Wellspring_VerticalAlignment
{
	WELLSPRING_VERTICALALIGNMENT_BASELINE,
	WELLSPRING_VERTICALALIGNMENT_TOP,
	WELLSPRING_VERTICALALIGNMENT_MIDDLE,
	WELLSPRING_VERTICALALIGNMENT_BOTTOM
} Wellspring_VerticalAlignment;

/* API definition */

WELLSPRINGAPI Wellspring_Font* Wellspring_CreateFont(
	const uint8_t *fontBytes,
	uint32_t fontBytesLength,
	const uint8_t *atlasJsonBytes,
	uint32_t atlasJsonBytesLength,
	float *pPixelsPerEm,
	float *pDistanceRange
);

/* Batches are not thread-safe, recommend one batch per thread. */
WELLSPRINGAPI Wellspring_TextBatch* Wellspring_CreateTextBatch(void);

/* Also restarts the batch */
WELLSPRINGAPI void Wellspring_StartTextBatch(
	Wellspring_TextBatch *textBatch
);

WELLSPRINGAPI uint8_t Wellspring_TextBounds(
	Wellspring_Font *font,
	int pixelSize,
	Wellspring_HorizontalAlignment horizontalAlignment,
	Wellspring_VerticalAlignment verticalAlignment,
	const uint8_t *strBytes,
	uint32_t strLengthInBytes,
	Wellspring_Rectangle *pRectangle
);

WELLSPRINGAPI uint8_t Wellspring_AddChunkToTextBatch(
	Wellspring_TextBatch *textBatch,
	Wellspring_Font *font,
	int pixelSize,
	Wellspring_HorizontalAlignment horizontalAlignment,
	Wellspring_VerticalAlignment verticalAlignment,
	const uint8_t *strBytes,
	uint32_t strLengthInBytes
);

WELLSPRINGAPI void Wellspring_GetBufferData(
	Wellspring_TextBatch *textBatch,
	uint32_t* pVertexCount,
	Wellspring_Vertex **pVertexBuffer
);

WELLSPRINGAPI void Wellspring_DestroyTextBatch(Wellspring_TextBatch *textBatch);
WELLSPRINGAPI void Wellspring_DestroyFont(Wellspring_Font *font);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* WELLSPRING_H */
