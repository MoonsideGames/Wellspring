/* Wellspring - An immediate mode font rendering system in C
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

#include "Wellspring.h"

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
#define Wellspring_sort SDL_qsort

#else

#ifdef _MSC_VER

#include <assert.h>
#include <stdlib.h>
#include <search.h>
#include <math.h>
#include <string.h>
#endif

#define Wellspring_malloc malloc
#define Wellspring_realloc realloc
#define Wellspring_free free
#define Wellspring_memcpy memcpy
#define Wellspring_memset memset
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
#define Wellspring_sort qsort

#endif

#define STBTT_malloc(x,u) ((void)(u),Wellspring_malloc(x))
#define STBTT_free(x,u) ((void)(u),Wellspring_free(x))
#define STBTT_memcpy Wellspring_memcpy
#define STBTT_memset Wellspring_memset
#define STBTT_ifloor Wellspring_ifloor
#define STBTT_iceil Wellspring_iceil
#define STBTT_sqrt Wellspring_sqrt
#define STBTT_pow Wellspring_pow
#define STBTT_fmod Wellspring_fmod
#define STBTT_cos Wellspring_cos
#define STBTT_acos Wellspring_acos
#define STBTT_fabs Wellspring_fabs
#define STBTT_assert Wellspring_assert
#define STBTT_strlen Wellspring_strlen

#define STBRP_SORT Wellspring_sort
#define STBRP_ASSERT Wellspring_assert

typedef uint8_t stbtt_uint8;
typedef int8_t stbtt_int8;
typedef uint16_t stbtt_uint16;
typedef int16_t stbtt_int16;
typedef uint32_t stbtt_uint32;
typedef int32_t stbtt_int32;

#pragma GCC diagnostic ignored "-Wunused-function"

#define STBRP_STATIC
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#pragma GCC diagnostic warning "-Wunused-function"

#define INITIAL_QUAD_CAPACITY 128

/* Structs */

typedef struct Font
{
	uint8_t *fontBytes;
	stbtt_fontinfo fontInfo;

	int32_t ascent;
	int32_t descent;
	int32_t lineGap;
} Font;

typedef struct CharRange
{
	stbtt_packedchar *data;
	uint32_t firstCodepoint;
	uint32_t charCount;
	float fontSize;
} CharRange;

typedef struct Packer
{
	Font *font;
	float fontSize;

	stbtt_pack_context *context;
	uint8_t *pixels;
	uint32_t width;
	uint32_t height;
	uint32_t strideInBytes;
	uint32_t padding;

	float scale; /* precomputed at init */

	CharRange *ranges;
	uint32_t rangeCount;
} Packer;

typedef struct Batch
{
	Wellspring_Vertex *vertices;
	uint32_t vertexCount;
	uint32_t vertexCapacity;

	uint32_t *indices;
	uint32_t indexCount;
	uint32_t indexCapacity;

	Packer *currentPacker;
} Batch;

/* UTF-8 Decoder */

/* Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
 * See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
 */

#define UTF8_ACCEPT 0
#define UTF8_REJECT 1

static const uint8_t utf8d[] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
	8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
	0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
	0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff
	0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
	1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, // s3..s4
	1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, // s5..s6
	1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s7..s8
};

static uint32_t inline
decode(uint32_t* state, uint32_t* codep, uint32_t byte) {
	uint32_t type = utf8d[byte];

	*codep = (*state != UTF8_ACCEPT) ?
	(byte & 0x3fu) | (*codep << 6) :
	(0xff >> type) & (byte);

	*state = utf8d[256 + *state*16 + type];
	return *state;
}

/* API */

uint32_t Wellspring_LinkedVersion(void)
{
	return WELLSPRING_COMPILED_VERSION;
}

Wellspring_Font* Wellspring_CreateFont(
	const uint8_t* fontBytes,
	uint32_t fontBytesLength
) {
	Font *font = Wellspring_malloc(sizeof(Font));

	font->fontBytes = Wellspring_malloc(fontBytesLength);
	Wellspring_memcpy(font->fontBytes, fontBytes, fontBytesLength);
	stbtt_InitFont(&font->fontInfo, font->fontBytes, 0);
	stbtt_GetFontVMetrics(&font->fontInfo, &font->ascent, &font->descent, &font->lineGap);

	return (Wellspring_Font*) font;
}

Wellspring_Packer* Wellspring_CreatePacker(
	Wellspring_Font *font,
	float fontSize,
	uint32_t width,
	uint32_t height,
	uint32_t strideInBytes,
	uint32_t padding
) {
	Packer *packer = Wellspring_malloc(sizeof(Packer));

	packer->font = (Font*) font;
	packer->fontSize = fontSize;

	packer->context = Wellspring_malloc(sizeof(stbtt_pack_context));
	packer->pixels = Wellspring_malloc(sizeof(uint8_t) * width * height);

	packer->width = width;
	packer->height = height;
	packer->strideInBytes = strideInBytes;
	packer->padding = padding;

	packer->ranges = NULL;
	packer->rangeCount = 0;

	packer->scale = stbtt_ScaleForPixelHeight(&packer->font->fontInfo, fontSize);

	stbtt_PackBegin(packer->context, packer->pixels, width, height, strideInBytes, padding, NULL);

	return (Wellspring_Packer*) packer;
}

uint32_t Wellspring_PackFontRanges(
	Wellspring_Packer *packer,
	Wellspring_FontRange *ranges,
	uint32_t numRanges
) {
	Packer *myPacker = (Packer*) packer;
	Wellspring_FontRange *currentFontRange;
	stbtt_pack_range* stbPackRanges = Wellspring_malloc(sizeof(stbtt_pack_range) * numRanges);
	CharRange *currentCharRange;
	uint32_t i;

	for (i = 0; i < numRanges; i += 1)
	{
		currentFontRange = &ranges[i];
		stbPackRanges[i].font_size = myPacker->fontSize;
		stbPackRanges[i].first_unicode_codepoint_in_range = currentFontRange->firstCodepoint;
		stbPackRanges[i].array_of_unicode_codepoints = NULL;
		stbPackRanges[i].num_chars = currentFontRange->numChars;
		stbPackRanges[i].h_oversample = currentFontRange->oversampleH;
		stbPackRanges[i].v_oversample = currentFontRange->oversampleV;
		stbPackRanges[i].chardata_for_range = Wellspring_malloc(sizeof(stbtt_packedchar) * currentFontRange->numChars);
	}

	if (!stbtt_PackFontRanges(myPacker->context, myPacker->font->fontBytes, 0, stbPackRanges, numRanges))
	{
		/* Font packing failed, time to bail */
		for (i = 0; i < numRanges; i += 1)
		{
			Wellspring_free(stbPackRanges[i].chardata_for_range);
		}
		return 0;
	}

	myPacker->ranges = Wellspring_realloc(myPacker->ranges, sizeof(CharRange) * (myPacker->rangeCount + numRanges));

	for (i = 0; i < numRanges; i += 1)
	{
		currentCharRange = &myPacker->ranges[myPacker->rangeCount + i];
		currentCharRange->data = stbPackRanges[i].chardata_for_range;
		currentCharRange->firstCodepoint = stbPackRanges[i].first_unicode_codepoint_in_range;
		currentCharRange->charCount = stbPackRanges[i].num_chars;
		currentCharRange->fontSize = stbPackRanges[i].font_size;
	}

	myPacker->rangeCount += numRanges;

	Wellspring_free(stbPackRanges);
	return 1;
}

uint8_t* Wellspring_GetPixelDataPointer(
	Wellspring_Packer *packer
) {
	Packer* myPacker = (Packer*) packer;
	return myPacker->pixels;
}

Wellspring_TextBatch* Wellspring_CreateTextBatch()
{
	Batch *batch = Wellspring_malloc(sizeof(Batch));

	batch->vertexCapacity = INITIAL_QUAD_CAPACITY * 4;
	batch->vertices = Wellspring_malloc(sizeof(Wellspring_Vertex) * batch->vertexCapacity);
	batch->vertexCount = 0;

	batch->indexCapacity = INITIAL_QUAD_CAPACITY * 6;
	batch->indices = Wellspring_malloc(sizeof(uint32_t) * batch->indexCapacity);
	batch->indexCount = 0;

	return (Wellspring_TextBatch*) batch;
}

void Wellspring_StartTextBatch(
	Wellspring_TextBatch *textBatch,
	Wellspring_Packer *packer
) {
	Batch *batch = (Batch*) textBatch;
	batch->currentPacker = (Packer*) packer;
	batch->vertexCount = 0;
	batch->indexCount = 0;
}

static float Wellspring_INTERNAL_GetVerticalAlignOffset(
	Font *font,
	Wellspring_VerticalAlignment verticalAlignment,
	float scale
) {
	if (verticalAlignment == WELLSPRING_VERTICALALIGNMENT_TOP)
	{
		return scale * font->ascent;
	}
	else if (verticalAlignment == WELLSPRING_VERTICALALIGNMENT_MIDDLE)
	{
		return scale * (font->ascent + font->descent) / 2.0f;
	}
	else if (verticalAlignment == WELLSPRING_VERTICALALIGNMENT_BASELINE)
	{
		return 0;
	}
	else /* BOTTOM */
	{
		return scale * font->descent;
	}
}

uint8_t Wellspring_TextBounds(
	Wellspring_TextBatch* textBatch,
	float x,
	float y,
	Wellspring_HorizontalAlignment horizontalAlignment,
	Wellspring_VerticalAlignment verticalAlignment,
	const uint8_t* strBytes,
	uint32_t strLengthInBytes,
	Wellspring_Rectangle *pRectangle
) {
	Batch* batch = (Batch*)textBatch;
	Packer* myPacker = batch->currentPacker;
	uint32_t decodeState = 0;
	uint32_t codepoint;
	int32_t glyphIndex;
	int32_t previousGlyphIndex;
	int32_t rangeIndex;
	stbtt_packedchar* rangeData;
	float rangeFontSize;
	stbtt_aligned_quad charQuad;
	uint32_t i, j;
	float minX = x;
	float minY = y;
	float maxX = x;
	float maxY = y;
	float startX = x;
	float advance = 0;

	y += Wellspring_INTERNAL_GetVerticalAlignOffset(myPacker->font, verticalAlignment, myPacker->scale);

	for (i = 0; i < strLengthInBytes; i += 1)
	{
		if (decode(&decodeState, &codepoint, strBytes[i]))
		{
			if (decodeState == UTF8_REJECT)
			{
				/* Something went very wrong */
				return 0;
			}

			continue;
		}

		rangeData = NULL;

		/* Find the packed char data */
		for (j = 0; j < myPacker->rangeCount; j += 1)
		{
			if (
				codepoint >= myPacker->ranges[j].firstCodepoint &&
				codepoint < myPacker->ranges[j].firstCodepoint + myPacker->ranges[j].charCount
			) {
				rangeData = myPacker->ranges[j].data;
				rangeIndex = codepoint - myPacker->ranges[j].firstCodepoint;
				rangeFontSize = myPacker->ranges[j].fontSize;
				break;
			}
		}

		if (rangeData == NULL)
		{
			/* Requested char wasn't packed! */
			return 0;
		}

		glyphIndex = stbtt_FindGlyphIndex(&myPacker->font->fontInfo, codepoint);

		if (i > 0)
		{
			x += myPacker->scale * stbtt_GetGlyphKernAdvance(&myPacker->font->fontInfo, previousGlyphIndex, glyphIndex);
		}

		stbtt_GetPackedQuad(
			rangeData,
			myPacker->width,
			myPacker->height,
			rangeIndex,
			&x,
			&y,
			&charQuad,
			0
		);

		if (charQuad.x0 < minX) { minX = charQuad.x0; }
		if (charQuad.x1 > maxX) { maxX = charQuad.x1; }
		if (charQuad.y0 < minY) { minY = charQuad.y0; }
		if (charQuad.y1 > maxY) { maxY = charQuad.y1; }

		previousGlyphIndex = glyphIndex;
	}

	advance = x - startX;

	if (horizontalAlignment == WELLSPRING_HORIZONTALALIGNMENT_RIGHT)
	{
		minX -= advance;
		maxX -= advance;
	}
	else if (horizontalAlignment == WELLSPRING_HORIZONTALALIGNMENT_CENTER)
	{
		minX -= advance * 0.5f;
		maxX -= advance * 0.5f;
	}

	pRectangle->x = minX;
	pRectangle->y = minY;
	pRectangle->w = maxX - minX;
	pRectangle->h = maxY - minY;

	return 1;
}

uint8_t Wellspring_Draw(
	Wellspring_TextBatch *textBatch,
	float x,
	float y,
	float depth,
	Wellspring_Color *color,
	Wellspring_HorizontalAlignment horizontalAlignment,
	Wellspring_VerticalAlignment verticalAlignment,
	const uint8_t *strBytes,
	uint32_t strLengthInBytes
) {
	Batch *batch = (Batch*) textBatch;
	Packer *myPacker = batch->currentPacker;
	uint32_t decodeState = 0;
	uint32_t codepoint;
	int32_t glyphIndex;
	int32_t previousGlyphIndex;
	int32_t rangeIndex;
	stbtt_packedchar *rangeData;
	float rangeFontSize;
	stbtt_aligned_quad charQuad;
	uint32_t vertexBufferIndex;
	uint32_t indexBufferIndex;
	Wellspring_Rectangle bounds;
	uint32_t i, j;

	y += Wellspring_INTERNAL_GetVerticalAlignOffset(myPacker->font, verticalAlignment, myPacker->scale);

	/* FIXME: If we horizontally align, we have to decode and process glyphs twice, very inefficient. */
	if (horizontalAlignment == WELLSPRING_HORIZONTALALIGNMENT_RIGHT)
	{
		if (!Wellspring_TextBounds(textBatch, x, y, horizontalAlignment, verticalAlignment, strBytes, strLengthInBytes, &bounds))
		{
			/* Something went wrong while calculating bounds. */
			return 0;
		}

		x -= bounds.w;
	}
	else if (horizontalAlignment == WELLSPRING_HORIZONTALALIGNMENT_CENTER)
	{
		if (!Wellspring_TextBounds(textBatch, x, y, horizontalAlignment, verticalAlignment, strBytes, strLengthInBytes, &bounds))
		{
			/* Something went wrong while calculating bounds. */
			return 0;
		}

		x -= bounds.w * 0.5f;
	}

	for (i = 0; i < strLengthInBytes; i += 1)
	{
		if (decode(&decodeState, &codepoint, strBytes[i]))
		{
			if (decodeState == UTF8_REJECT)
			{
				/* Something went wrong while decoding UTF-8. */
				return 0;
			}

			continue;
		}

		rangeData = NULL;

		/* Find the packed char data */
		for (j = 0; j < myPacker->rangeCount; j += 1)
		{
			if (
				codepoint >= myPacker->ranges[j].firstCodepoint &&
				codepoint < myPacker->ranges[j].firstCodepoint + myPacker->ranges[j].charCount
			) {
				rangeData = myPacker->ranges[j].data;
				rangeIndex = codepoint - myPacker->ranges[j].firstCodepoint;
				rangeFontSize = myPacker->ranges[j].fontSize;
				break;
			}
		}

		if (rangeData == NULL)
		{
			/* Requested char wasn't packed! */
			return 0;
		}
		
		glyphIndex = stbtt_FindGlyphIndex(&myPacker->font->fontInfo, codepoint);

		if (i > 0)
		{
			x += myPacker->scale * stbtt_GetGlyphKernAdvance(&myPacker->font->fontInfo, previousGlyphIndex, glyphIndex);
		}

		stbtt_GetPackedQuad(
			rangeData,
			myPacker->width,
			myPacker->height,
			rangeIndex,
			&x,
			&y,
			&charQuad,
			0
		);

		if (batch->vertexCount >= batch->vertexCapacity)
		{
			batch->vertexCapacity *= 2;
			batch->vertices = Wellspring_realloc(batch->vertices, sizeof(Wellspring_Vertex) * batch->vertexCapacity);
		}

		if (batch->indexCount >= batch->indexCapacity)
		{
			batch->indexCapacity *= 2;
			batch->indices = Wellspring_realloc(batch->indices, sizeof(uint32_t) * batch->indexCapacity);
		}

		/* TODO: kerning and alignment */

		vertexBufferIndex = batch->vertexCount;
		indexBufferIndex = batch->indexCount;

		batch->vertices[vertexBufferIndex].x = charQuad.x0;
		batch->vertices[vertexBufferIndex].y = charQuad.y0;
		batch->vertices[vertexBufferIndex].z = depth;
		batch->vertices[vertexBufferIndex].u = charQuad.s0;
		batch->vertices[vertexBufferIndex].v = charQuad.t0;
		batch->vertices[vertexBufferIndex].r = color->r;
		batch->vertices[vertexBufferIndex].g = color->g;
		batch->vertices[vertexBufferIndex].b = color->b;
		batch->vertices[vertexBufferIndex].a = color->a;

		batch->vertices[vertexBufferIndex + 1].x = charQuad.x0;
		batch->vertices[vertexBufferIndex + 1].y = charQuad.y1;
		batch->vertices[vertexBufferIndex + 1].z = depth;
		batch->vertices[vertexBufferIndex + 1].u = charQuad.s0;
		batch->vertices[vertexBufferIndex + 1].v = charQuad.t1;
		batch->vertices[vertexBufferIndex + 1].r = color->r;
		batch->vertices[vertexBufferIndex + 1].g = color->g;
		batch->vertices[vertexBufferIndex + 1].b = color->b;
		batch->vertices[vertexBufferIndex + 1].a = color->a;

		batch->vertices[vertexBufferIndex + 2].x = charQuad.x1;
		batch->vertices[vertexBufferIndex + 2].y = charQuad.y0;
		batch->vertices[vertexBufferIndex + 2].z = depth;
		batch->vertices[vertexBufferIndex + 2].u = charQuad.s1;
		batch->vertices[vertexBufferIndex + 2].v = charQuad.t0;
		batch->vertices[vertexBufferIndex + 2].r = color->r;
		batch->vertices[vertexBufferIndex + 2].g = color->g;
		batch->vertices[vertexBufferIndex + 2].b = color->b;
		batch->vertices[vertexBufferIndex + 2].a = color->a;

		batch->vertices[vertexBufferIndex + 3].x = charQuad.x1;
		batch->vertices[vertexBufferIndex + 3].y = charQuad.y1;
		batch->vertices[vertexBufferIndex + 3].z = depth;
		batch->vertices[vertexBufferIndex + 3].u = charQuad.s1;
		batch->vertices[vertexBufferIndex + 3].v = charQuad.t1;
		batch->vertices[vertexBufferIndex + 3].r = color->r;
		batch->vertices[vertexBufferIndex + 3].g = color->g;
		batch->vertices[vertexBufferIndex + 3].b = color->b;
		batch->vertices[vertexBufferIndex + 3].a = color->a;

		batch->indices[indexBufferIndex]     = vertexBufferIndex;
		batch->indices[indexBufferIndex + 1] = vertexBufferIndex + 1;
		batch->indices[indexBufferIndex + 2] = vertexBufferIndex + 2;
		batch->indices[indexBufferIndex + 3] = vertexBufferIndex + 2;
		batch->indices[indexBufferIndex + 4] = vertexBufferIndex + 1;
		batch->indices[indexBufferIndex + 5] = vertexBufferIndex + 3;

		batch->vertexCount += 4;
		batch->indexCount += 6;

		previousGlyphIndex = glyphIndex;
	}

	return 1;
}

void Wellspring_GetBufferData(
	Wellspring_TextBatch *textBatch,
	Wellspring_Vertex **pVertexBuffer,
	uint32_t *pVertexBufferLengthInBytes,
	uint32_t **pIndexBuffer,
	uint32_t *pIndexBufferLengthInBytes
) {
	Batch *batch = (Batch*) textBatch;
	*pVertexBuffer = batch->vertices;
	*pVertexBufferLengthInBytes = batch->vertexCount * sizeof(Wellspring_Vertex);
	*pIndexBuffer = batch->indices;
	*pIndexBufferLengthInBytes = batch->indexCount * sizeof(uint32_t);
}

void Wellspring_DestroyTextBatch(Wellspring_TextBatch *textBatch)
{
	Batch *batch = (Batch*) textBatch;
	Wellspring_free(batch->vertices);
	Wellspring_free(batch->indices);
	Wellspring_free(batch);
}

void Wellspring_DestroyPacker(Wellspring_Packer *packer)
{
	Packer* myPacker = (Packer*) packer;
	uint32_t i;

	stbtt_PackEnd(myPacker->context);

	for (i = 0; i < myPacker->rangeCount; i += 1)
	{
		Wellspring_free(myPacker->ranges[i].data);
	}

	Wellspring_free(myPacker->ranges);
	Wellspring_free(myPacker->context);
	Wellspring_free(myPacker->pixels);
}

void Wellspring_DestroyFont(Wellspring_Font* font)
{
	Font *myFont = (Font*) font;

	Wellspring_free(myFont->fontBytes);
	Wellspring_free(myFont);
}
