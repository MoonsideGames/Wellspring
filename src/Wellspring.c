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

#include "Wellspring.h"

 /* Function defines */

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

#define SHEREDOM_JSON_H_malloc Wellspring_malloc
#define SHEREDOM_JSON_H_free Wellspring_free

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

#include "json.h"

#pragma GCC diagnostic warning "-Wunused-function"

#define INITIAL_QUAD_CAPACITY 128

/* Structs */

typedef struct PackedChar
{
	float atlasLeft, atlasTop, atlasRight, atlasBottom;
	float planeLeft, planeTop, planeRight, planeBottom;
	float xAdvance;
} PackedChar;

typedef struct CharRange
{
	PackedChar *data;
	uint32_t firstCodepoint;
	uint32_t charCount;
} CharRange;

typedef struct Packer
{
	uint32_t width;
	uint32_t height;

	CharRange *ranges;
	uint32_t rangeCount;
} Packer;

typedef struct Font
{
	uint8_t *fontBytes;
	stbtt_fontinfo fontInfo;

	float ascender;
	float descender;
	float lineHeight;
	float pixelsPerEm;
	float distanceRange;

	float scale;
	float kerningScale; // kerning values from stb_tt are in a different scale

	Packer packer;
} Font;

typedef struct Batch
{
	Wellspring_Vertex *vertices;
	uint32_t vertexCount;
	uint32_t vertexCapacity;

	uint32_t *indices;
	uint32_t indexCount;
	uint32_t indexCapacity;

	Font *currentFont;
} Batch;

typedef struct Quad
{
   float x0,y0,s0,t0; // top-left
   float x1,y1,s1,t1; // bottom-right
} Quad;

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

/* JSON helpers */

static uint8_t json_object_has_key(const json_object_t *object, const char* name)
{
	json_object_element_t *currentElement = object->start;
	const char* currentName = currentElement->name->string;

	while (SDL_strcmp(currentName, name) != 0)
	{
		if (currentElement->next == NULL)
		{
			return 0;
		}

		currentElement = currentElement->next;
		currentName = currentElement->name->string;
	}

	return 1;
}

static json_object_element_t* json_object_get_element_by_name(const json_object_t *object, const char* name)
{
	json_object_element_t *currentElement = object->start;
	const char* currentName = currentElement->name->string;

	while (SDL_strcmp(currentName, name) != 0)
	{
		if (currentElement->next == NULL)
		{
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Key %s not found in JSON!", name);
			return NULL;
		}

		currentElement = currentElement->next;
		currentName = currentElement->name->string;
	}

	return currentElement;
}

static json_object_t* json_object_get_object(const json_object_t *object, const char* name)
{
	json_object_element_t *element = json_object_get_element_by_name(object, name);

	if (element == NULL)
	{
		return NULL;
	}

	json_object_t *obj = json_value_as_object(element->value);

	if (obj == NULL)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Value with key %s was not an object!", name);
	}

	return obj;
}

static const char* json_object_get_string(const json_object_t *object, const char* name)
{
	json_object_element_t *element = json_object_get_element_by_name(object, name);

	if (element == NULL)
	{
		return NULL;
	}

	json_string_t *str = json_value_as_string(element->value);

	if (str == NULL)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Value with key %s was not a string!", name);
		return NULL;
	}

	return str->string;
}

static uint32_t json_object_get_uint(const json_object_t *object, const char* name)
{
	json_object_element_t *element = json_object_get_element_by_name(object, name);

	if (element == NULL)
	{
		return 0;
	}

	json_number_t *num = json_value_as_number(element->value);

	if (num == NULL)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Value with key %s was not a number!", name);
		return 0;
	}

	return (uint32_t) SDL_strtoul(num->number, NULL, 10);
}

static double json_object_get_double(const json_object_t *object, const char* name)
{
	json_object_element_t *element = json_object_get_element_by_name(object, name);

	if (element == NULL)
	{
		return 0;
	}

	json_number_t *num = json_value_as_number(element->value);

	if (num == NULL)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Value with key %s was not a string!", name);
		return 0;
	}

	return SDL_atof(num->number);
}

/* API */

uint32_t Wellspring_LinkedVersion(void)
{
	return WELLSPRING_COMPILED_VERSION;
}

Wellspring_Font* Wellspring_CreateFont(
	const uint8_t* fontBytes,
	uint32_t fontBytesLength,
	const uint8_t *atlasJsonBytes,
	uint32_t atlasJsonBytesLength,
	float *pPixelsPerEm,
	float *pDistanceRange
) {
	Font *font = Wellspring_malloc(sizeof(Font));

	font->fontBytes = Wellspring_malloc(fontBytesLength);
	Wellspring_memcpy(font->fontBytes, fontBytes, fontBytesLength);
	stbtt_InitFont(&font->fontInfo, font->fontBytes, 0);
	int stbAscender, stbDescender, stbLineHeight;
	stbtt_GetFontVMetrics(&font->fontInfo, &stbAscender, &stbDescender, &stbLineHeight);

	json_value_t *jsonRoot = json_parse(atlasJsonBytes, atlasJsonBytesLength);
	json_object_t *jsonObject = jsonRoot->payload;

	if (jsonObject == NULL)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Atlas JSON is invalid! Bailing!");
		Wellspring_free(font->fontBytes);
		Wellspring_free(font);
		return NULL;
	}

	if (SDL_strcmp(jsonObject->start->name->string, "atlas") != 0)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Atlas JSON is invalid! Bailing!");
		Wellspring_free(jsonRoot);
		Wellspring_free(font->fontBytes);
		Wellspring_free(font);
		return NULL;
	}

	json_object_t *atlasObject = json_object_get_object(jsonObject, "atlas");
	if (atlasObject == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", "atlas object not found!");
		Wellspring_free(jsonRoot);
		Wellspring_free(font->fontBytes);
		Wellspring_free(font);
		return NULL;
	}

	json_object_t *metricsObject = json_object_get_object(jsonObject, "metrics");
	if (metricsObject == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", "atlas object not found!");
		Wellspring_free(jsonRoot);
		Wellspring_free(font->fontBytes);
		Wellspring_free(font);
		return NULL;
	}

	json_array_t *glyphsArray = json_value_as_array(json_object_get_element_by_name(jsonObject, "glyphs")->value);
	if (glyphsArray == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", "atlas object not found!");
		Wellspring_free(jsonRoot);
		Wellspring_free(font->fontBytes);
		Wellspring_free(font);
		return NULL;
	}

	const char* atlasType = json_object_get_string(atlasObject, "type");

	if (SDL_strcmp(atlasType, "msdf") != 0)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Atlas is not MSDF! Bailing!");
		Wellspring_free(jsonRoot);
		Wellspring_free(font->fontBytes);
		Wellspring_free(font);
		return NULL;
	}

	font->packer.width = json_object_get_uint(atlasObject, "width");
	font->packer.height = json_object_get_uint(atlasObject, "height");
	font->pixelsPerEm = json_object_get_double(atlasObject, "size");
	font->distanceRange = json_object_get_double(atlasObject, "distanceRange");

	font->ascender = json_object_get_double(metricsObject, "ascender");
	font->descender = json_object_get_double(metricsObject, "descender");
	font->lineHeight = json_object_get_double(metricsObject, "lineHeight");

	font->scale = font->pixelsPerEm * 4 / 3; // converting from "points" (dpi) to pixels

	/* Pack unicode ranges */

	font->packer.ranges = Wellspring_malloc(sizeof(CharRange));
	font->packer.rangeCount = 1;
	font->packer.ranges[0].data = NULL;
	font->packer.ranges[0].charCount = 0;

	int32_t charRangeIndex = 0;

	json_array_element_t *currentGlyphElement = glyphsArray->start;
	while (currentGlyphElement != NULL)
	{
		json_object_t *currentGlyphObject = json_value_as_object(currentGlyphElement->value);

		uint32_t codepoint = json_object_get_uint(currentGlyphObject, "unicode");

		if (font->packer.ranges[charRangeIndex].charCount == 0)
		{
			// first codepoint on first range
			font->packer.ranges[charRangeIndex].firstCodepoint = codepoint;
		}
		else if (codepoint != font->packer.ranges[charRangeIndex].firstCodepoint + font->packer.ranges[charRangeIndex].charCount)
		{
			// codepoint is not continuous, start a new range
			charRangeIndex += 1;
			font->packer.rangeCount += 1;

			font->packer.ranges = Wellspring_realloc(font->packer.ranges, sizeof(PackedChar) * (charRangeIndex + 1));
			font->packer.ranges[charRangeIndex].firstCodepoint = codepoint;
		}

		font->packer.ranges[charRangeIndex].charCount += 1;
		font->packer.ranges[charRangeIndex].data = Wellspring_realloc(font->packer.ranges[charRangeIndex].data, sizeof(PackedChar) * font->packer.ranges[charRangeIndex].charCount);

		PackedChar *packedChar = &font->packer.ranges[charRangeIndex].data[font->packer.ranges[charRangeIndex].charCount - 1];
		packedChar->atlasLeft = 0;
		packedChar->atlasRight = 0;
		packedChar->atlasTop = 0;
		packedChar->atlasBottom = 0;
		packedChar->planeLeft = 0;
		packedChar->planeRight = 0;
		packedChar->planeTop = 0;
		packedChar->planeBottom = 0;

		packedChar->xAdvance = json_object_get_double(currentGlyphObject, "advance");

		if (json_object_has_key(currentGlyphObject, "atlasBounds"))
		{
			json_object_t *boundsObject = json_object_get_object(currentGlyphObject, "atlasBounds");

			packedChar->atlasLeft = json_object_get_double(boundsObject, "left");
			packedChar->atlasRight = json_object_get_double(boundsObject, "right");
			packedChar->atlasTop = json_object_get_double(boundsObject, "top");
			packedChar->atlasBottom = json_object_get_double(boundsObject, "bottom");

			json_object_t *planeObject = json_object_get_object(currentGlyphObject, "planeBounds");

			packedChar->planeLeft = json_object_get_double(planeObject, "left");
			packedChar->planeRight = json_object_get_double(planeObject, "right");
			packedChar->planeTop = json_object_get_double(planeObject, "top");
			packedChar->planeBottom = json_object_get_double(planeObject, "bottom");
		}

		currentGlyphElement = currentGlyphElement->next;
	}

	int advanceWidth, bearing;
	stbtt_GetCodepointHMetrics(&font->fontInfo, font->packer.ranges[0].firstCodepoint, &advanceWidth, &bearing);

	font->kerningScale = font->packer.ranges[0].data[0].xAdvance / advanceWidth;

	Wellspring_free(jsonRoot);

	*pPixelsPerEm = font->pixelsPerEm;
	*pDistanceRange = font->distanceRange;

	return (Wellspring_Font*) font;
}

Wellspring_TextBatch* Wellspring_CreateTextBatch(void)
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
	Wellspring_Font *font
) {
	Batch *batch = (Batch*) textBatch;
	batch->currentFont = (Font*) font;
	batch->vertexCount = 0;
	batch->indexCount = 0;
}

static float Wellspring_INTERNAL_GetVerticalAlignOffset(
	Font *font,
	Wellspring_VerticalAlignment verticalAlignment,
	float scale
) {
	if (verticalAlignment == WELLSPRING_VERTICALALIGNMENT_BASELINE)
	{
		return 0;
	}
	else if (verticalAlignment == WELLSPRING_VERTICALALIGNMENT_TOP)
	{
		return scale * font->ascender;
	}
	else if (verticalAlignment == WELLSPRING_VERTICALALIGNMENT_MIDDLE)
	{
		return scale * (font->ascender + font->descender) / 2.0f;
	}
	else /* BOTTOM */
	{
		return scale * font->descender;
	}
}

static inline uint32_t IsWhitespace(uint32_t codepoint)
{
	switch (codepoint)
	{
		case 0x0020:
		case 0x00A0:
		case 0x1680:
		case 0x202F:
		case 0x205F:
		case 0x3000:
			return 1;

		default:
			if (codepoint > 0x2000 && codepoint <= 0x200A)
			{
				return 1;
			}

			return 0;
	}
}

static void GetPackedQuad(PackedChar *charData, float scale, int packerWidth, int packerHeight, int charIndex, float *xPos, float *yPos, Quad *q)
{
	float texelWidth = 1.0f / packerWidth, texelHeight = 1.0f / packerHeight;
	PackedChar *b = charData + charIndex;

	float pl, pb, pr, pt;
	float il, ib, ir, it;

	pl = *xPos + b->planeLeft * scale;
	pb = *yPos + b->planeBottom * scale;
	pr = *xPos + b->planeRight * scale;
	pt = *yPos + b->planeTop * scale;

	il = b->atlasLeft * texelWidth;
	ib = b->atlasBottom * texelHeight;
	ir = b->atlasRight * texelWidth;
	it = b->atlasTop * texelHeight;

	q->x0 = pl;
	q->y0 = pt;
	q->x1 = pr;
	q->y1 = pb;

	q->s0 = il;
	q->t0 = it;
	q->s1 = ir;
	q->t1 = ib;

	*xPos += b->xAdvance * scale;
}

static uint8_t Wellspring_Internal_TextBounds(
	Font* font,
	int pixelSize,
	Wellspring_HorizontalAlignment horizontalAlignment,
	Wellspring_VerticalAlignment verticalAlignment,
	const uint8_t* strBytes,
	uint32_t strLengthInBytes,
	Wellspring_Rectangle *pRectangle
) {
	uint32_t decodeState = 0;
	uint32_t codepoint;
	int32_t glyphIndex;
	int32_t previousGlyphIndex = -1;
	int32_t rangeIndex;
	PackedChar* rangeData;
	Quad charQuad;
	uint32_t i, j;
	float x = 0, y = 0;
	float minX = x;
	float minY = y;
	float maxX = x;
	float maxY = y;
	float startX = x;
	float advance = 0;
	float sizeFactor = pixelSize / font->pixelsPerEm;

	y -= Wellspring_INTERNAL_GetVerticalAlignOffset(font, verticalAlignment, sizeFactor * font->scale);

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

		Packer *packer = &font->packer;

		/* Find the packed char data */
		for (j = 0; j < packer->rangeCount; j += 1)
		{
			if (
				codepoint >= packer->ranges[j].firstCodepoint &&
				codepoint < packer->ranges[j].firstCodepoint + packer->ranges[j].charCount
			) {
				rangeData = packer->ranges[j].data;
				rangeIndex = codepoint - packer->ranges[j].firstCodepoint;
				break;
			}
		}

		if (rangeData == NULL)
		{
			/* Requested char wasn't packed! */
			return 0;
		}

		if (IsWhitespace(codepoint))
		{
			PackedChar *packedChar = rangeData + rangeIndex;
			x += sizeFactor * font->scale * packedChar->xAdvance;
			maxX += sizeFactor * font->scale * packedChar->xAdvance;
			previousGlyphIndex = -1;
			continue;
		}

		glyphIndex = stbtt_FindGlyphIndex(&font->fontInfo, codepoint);

		if (previousGlyphIndex != -1)
		{
			x += sizeFactor * font->kerningScale * font->scale * stbtt_GetGlyphKernAdvance(&font->fontInfo, previousGlyphIndex, glyphIndex);
		}

		GetPackedQuad(
			rangeData,
			sizeFactor * font->scale,
			packer->width,
			packer->height,
			rangeIndex,
			&x,
			&y,
			&charQuad
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

uint8_t Wellspring_TextBounds(
	Wellspring_Font *font,
	int pixelSize,
	Wellspring_HorizontalAlignment horizontalAlignment,
	Wellspring_VerticalAlignment verticalAlignment,
	const uint8_t* strBytes,
	uint32_t strLengthInBytes,
	Wellspring_Rectangle* pRectangle
) {
	return Wellspring_Internal_TextBounds(
		(Font*) font,
		pixelSize,
		horizontalAlignment,
		verticalAlignment,
		strBytes,
		strLengthInBytes,
		pRectangle
	);
}

uint8_t Wellspring_AddToTextBatch(
	Wellspring_TextBatch *textBatch,
	int pixelSize,
	Wellspring_Color *color,
	Wellspring_HorizontalAlignment horizontalAlignment,
	Wellspring_VerticalAlignment verticalAlignment,
	const uint8_t *strBytes,
	uint32_t strLengthInBytes
) {
	Batch *batch = (Batch*) textBatch;
	Font *font = batch->currentFont;
	Packer *myPacker = &font->packer;
	uint32_t decodeState = 0;
	uint32_t codepoint;
	int32_t glyphIndex;
	int32_t previousGlyphIndex = -1;
	int32_t rangeIndex;
	PackedChar *rangeData;
	Quad charQuad;
	uint32_t vertexBufferIndex;
	uint32_t indexBufferIndex;
	Wellspring_Rectangle bounds;
	uint32_t i, j;
	float sizeFactor = pixelSize / font->pixelsPerEm;
	float x = 0, y = 0;

	y -= Wellspring_INTERNAL_GetVerticalAlignOffset(font, verticalAlignment, sizeFactor * font->scale);

	/* FIXME: If we horizontally align, we have to decode and process glyphs twice, very inefficient. */
	if (horizontalAlignment == WELLSPRING_HORIZONTALALIGNMENT_RIGHT)
	{
		if (!Wellspring_Internal_TextBounds(font, pixelSize, horizontalAlignment, verticalAlignment, strBytes, strLengthInBytes, &bounds))
		{
			/* Something went wrong while calculating bounds. */
			return 0;
		}

		x -= bounds.w;
	}
	else if (horizontalAlignment == WELLSPRING_HORIZONTALALIGNMENT_CENTER)
	{
		if (!Wellspring_Internal_TextBounds(font, pixelSize, horizontalAlignment, verticalAlignment, strBytes, strLengthInBytes, &bounds))
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
				break;
			}
		}

		if (rangeData == NULL)
		{
			/* Requested char wasn't packed! */
			return 0;
		}

		if (IsWhitespace(codepoint))
		{
			PackedChar *packedChar = rangeData + rangeIndex;
			x += sizeFactor * font->scale * packedChar->xAdvance;
			previousGlyphIndex = -1;
			continue;
		}

		glyphIndex = stbtt_FindGlyphIndex(&font->fontInfo, codepoint);

		if (previousGlyphIndex != -1)
		{
			x += sizeFactor * font->kerningScale * font->scale * stbtt_GetGlyphKernAdvance(&font->fontInfo, previousGlyphIndex, glyphIndex);
		}

		GetPackedQuad(
			rangeData,
			sizeFactor * font->scale,
			myPacker->width,
			myPacker->height,
			rangeIndex,
			&x,
			&y,
			&charQuad
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

		vertexBufferIndex = batch->vertexCount;
		indexBufferIndex = batch->indexCount;

		batch->vertices[vertexBufferIndex].x = charQuad.x0;
		batch->vertices[vertexBufferIndex].y = charQuad.y0;
		batch->vertices[vertexBufferIndex].z = 0;
		batch->vertices[vertexBufferIndex].u = charQuad.s0;
		batch->vertices[vertexBufferIndex].v = charQuad.t0;
		batch->vertices[vertexBufferIndex].r = color->r;
		batch->vertices[vertexBufferIndex].g = color->g;
		batch->vertices[vertexBufferIndex].b = color->b;
		batch->vertices[vertexBufferIndex].a = color->a;

		batch->vertices[vertexBufferIndex + 1].x = charQuad.x0;
		batch->vertices[vertexBufferIndex + 1].y = charQuad.y1;
		batch->vertices[vertexBufferIndex + 1].z = 0;
		batch->vertices[vertexBufferIndex + 1].u = charQuad.s0;
		batch->vertices[vertexBufferIndex + 1].v = charQuad.t1;
		batch->vertices[vertexBufferIndex + 1].r = color->r;
		batch->vertices[vertexBufferIndex + 1].g = color->g;
		batch->vertices[vertexBufferIndex + 1].b = color->b;
		batch->vertices[vertexBufferIndex + 1].a = color->a;

		batch->vertices[vertexBufferIndex + 2].x = charQuad.x1;
		batch->vertices[vertexBufferIndex + 2].y = charQuad.y0;
		batch->vertices[vertexBufferIndex + 2].z = 0;
		batch->vertices[vertexBufferIndex + 2].u = charQuad.s1;
		batch->vertices[vertexBufferIndex + 2].v = charQuad.t0;
		batch->vertices[vertexBufferIndex + 2].r = color->r;
		batch->vertices[vertexBufferIndex + 2].g = color->g;
		batch->vertices[vertexBufferIndex + 2].b = color->b;
		batch->vertices[vertexBufferIndex + 2].a = color->a;

		batch->vertices[vertexBufferIndex + 3].x = charQuad.x1;
		batch->vertices[vertexBufferIndex + 3].y = charQuad.y1;
		batch->vertices[vertexBufferIndex + 3].z = 0;
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
	uint32_t *pVertexCount,
	Wellspring_Vertex **pVertexBuffer,
	uint32_t *pVertexBufferLengthInBytes,
	uint32_t **pIndexBuffer,
	uint32_t *pIndexBufferLengthInBytes
) {
	Batch *batch = (Batch*) textBatch;
	*pVertexCount = batch->vertexCount;
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

void Wellspring_DestroyFont(Wellspring_Font* font)
{
	Font *myFont = (Font*) font;

	for (int i = 0; i < myFont->packer.rangeCount; i += 1)
	{
		Wellspring_free(myFont->packer.ranges[i].data);
	}
	Wellspring_free(myFont->packer.ranges);
	Wellspring_free(myFont->fontBytes);
	Wellspring_free(myFont);
}
