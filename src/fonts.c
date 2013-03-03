/* Copyright (c) 2012 Toni Georgiev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "gk.h"
#include "gk_internal.h"

#include <GL/gl.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_STROKER_H

#ifdef GK_WIN
#define stricmp _stricmp
#else
#define stricmp strcasecmp
#endif

FT_Library ftlib;

#define GK_FONT_TOTAL_STYLES 4

gkTextFormat gkDefaultTextFormat = {
	GK_TEXT_ALIGN_LEFT,		/*	align */
	GK_TEXT_VALIGN_TOP,		/*	valign */
	GK_FALSE,				/*	wordWrap */
	4,						/*	tab space */
	GK_FALSE,				/*	underline */
	0,						/*	width */
	0,						/*	height	*/
	0,						/*	stroke size	*/
	0,						/*	line spacing */
	{1,1,1,1},				/*	text color	*/
	{0,0,0,1},				/*	stroke color */
	GK_FALSE				/*	vertical */
};

typedef struct gkBBoxStruct gkBBox;
struct gkBBoxStruct{
	float minX;
	float minY;
	float maxX;
	float maxY;
};

gkBBox gkCreateBBox(float minx, float miny, float maxx, float maxy){
	gkBBox r = {minx, miny, maxx, maxy};
	return r;
}
void gkBBoxAdd(gkBBox* dst, gkBBox *src){
	if(dst->minX > src->minX) dst->minX = src->minX;
	if(dst->minY > src->minY) dst->minY = src->minY;
	if(dst->maxX < src->maxX) dst->maxX = src->maxX;
	if(dst->maxY < src->maxY) dst->maxY = src->maxY;
}
void gkBBoxTranslate(gkBBox* dst, float tx, float ty){
	dst->minX += tx;
	dst->maxX += tx;
	dst->minY += ty;
	dst->maxY += ty;
}

typedef struct gkGlyphStruct gkGlyph;
struct gkGlyphStruct{
	int index;
	gkSize size;
	gkPoint offset;
	gkPoint advance;
	gkBBox bbox;
	uint32_t texId;
	gkRect texCoords;
};

typedef struct gkGlyphSetStruct gkGlyphSet;
struct gkGlyphSetStruct{
	uint32_t texId;
	gkGlyph** glyphs;
};

typedef struct gkGlyphCollectionStruct gkGlyphCollection;
struct gkGlyphCollectionStruct{
	uint16_t size;
	float strokeSize;
	uint16_t setBits;
	uint16_t texWidth;
	uint16_t texHeight;
	uint16_t cellWidth;
	uint16_t cellHeight;
	uint16_t glyphSetCount;
	gkGlyphSet** glyphSets;
	gkGlyphCollection* next;
};

struct gkFontFaceStructEx{
	char* fontFamily;
	uint8_t style;
	/*Extended info*/
	FT_Face ftface;
	gkGlyphCollection* collections;
};
typedef struct gkFontFaceStructEx gkFontFaceEx;

typedef struct gkFontResourceStructEx gkFontResourceEx;
struct gkFontResourceStructEx{
	uint8_t numFaces;
	gkFontFace **faces;
};

typedef struct gkFontRcRefStruct gkFontRcRef;
struct gkFontRcRefStruct{
	gkFontResourceEx* resource;
	gkFontRcRef* next;
};
gkFontRcRef* gkFontResources, *gkFontResourcesTop;

struct gkFontStructEx{
	gkFontFace *face;
	uint16_t size;
};
typedef struct gkFontStructEx gkFontEx;

void gkInitFont(gkFontEx* font);
void gkDestroyFace(gkFontFaceEx* face);

gkGlyphCollection* gkGetGlyphCollection(gkFont* font, float strokeSize);
gkGlyphSet* gkGetGlyphSet(gkGlyphCollection* collection, int index);
gkGlyph* gkGetGlyph(FT_Face face, gkGlyphCollection* collection, gkGlyphSet* set, uint32_t index);

void gkDestroyGlyphCollection(gkGlyphCollection* collection);
void gkDestroyGlyphSet(gkGlyphSet* set, int setSize);
void gkDestroyGlyph(gkGlyph* glyph);

/* functions */

void gkInitFonts(){
	FT_Error err = FT_Init_FreeType(&ftlib);
	if(err){
		printf("FreeType2 could not be initializeed.");
		gkExit();
	}
	gkFontResources = 0;
	gkFontResourcesTop = 0;
}

gkFontResource* gkAddFontResource(char* filename){
	FT_Face face;
	gkFontResourceEx *resource = 0;
	gkFontRcRef* ref;
	gkFontFaceEx* f;
	int i = 0;
	FT_Error error;
	do{
		error = FT_New_Face(ftlib, filename, i, &face);
		if(error){
			return 0;
		}
		if(resource == 0){
			resource = (gkFontResourceEx*)malloc(sizeof(gkFontResourceEx));
			resource->numFaces = (uint8_t)face->num_faces;
			resource->faces = (gkFontFace**)calloc(face->num_faces, sizeof(gkFontFaceEx*));
			ref = (gkFontRcRef*)malloc(sizeof(gkFontRcRef));
			ref->resource = resource;
			ref->next = 0;
			if(gkFontResources){
				gkFontResourcesTop->next = ref;
				gkFontResourcesTop = ref;
			}else{
				gkFontResources = gkFontResourcesTop = ref;
			}
		}
		f = (gkFontFaceEx*)(resource->faces[i] = (gkFontFace*)malloc(sizeof(gkFontFaceEx)));
		f->fontFamily = face->family_name;
		f->style = (uint8_t)face->style_flags;
		f->ftface = face;
		f->collections = 0;
		i++;
	}while(i<resource->numFaces);
	return (gkFontResource*)resource;
}

void gkRemoveFontResource(gkFontResource* rc){
	gkFontRcRef* ref = gkFontResources, *prev = 0, *p;
	gkFontFaceEx* face;
	int i;
	while(ref){
		if(ref->resource == (gkFontResourceEx*)rc){
			p = ref;
			for(i = 0; i<rc->numFaces; i++){
				face = (gkFontFaceEx*)rc->faces[i];
				gkDestroyFace(face);
			}
			free(rc->faces);
			free(p->resource);
			if(prev){
				prev->next = p->next;
			}else{
				gkFontResources = p->next;
			}
			ref = ref->next;
			free(p);
		}else{
			prev = ref;
			ref = ref->next;
		}
	}
}

gkFont* gkCreateFont(char* family, uint16_t size, uint8_t style){
	gkFontEx* font;
	gkFontRcRef* p = gkFontResources;
	gkFontResourceEx* resource;
	gkFontFaceEx* face;
	int i;
	while(p){
		resource = p->resource;
		for(i = 0; i<resource->numFaces; i++){
			face = (gkFontFaceEx*)resource->faces[i];
			if(stricmp(face->fontFamily, family) == 0 && face->style == style){
				font = (gkFontEx*)malloc(sizeof(gkFontEx));
				font->face = (gkFontFace*)face;
				font->size = size;
				gkInitFont(font);
				return (gkFont*)font;
			}
		}
		p = p->next;
	}
	return 0;
}

void gkDestroyFont(gkFont* font){
	free(font);
}

void gkCleanupFonts(){
	gkFontRcRef* ref = gkFontResources, *p;
	gkFontFaceEx* face;
	int i;
	while(ref){
		p = ref;
		for(i = 0; i<p->resource->numFaces; i++){
			face = (gkFontFaceEx*)p->resource->faces[i];
			FT_Done_Face(face->ftface);
			free(face);
		}
		free(p->resource->faces);
		free(p->resource);
		ref = ref->next;
		free(p);
	}
	gkFontResources = gkFontResourcesTop = 0;
	FT_Done_FreeType(ftlib);
	ftlib = 0;
}

void gkInitFont(gkFontEx* font){
}


/*
	Glyph caching
*/

#define GK_MIN_FONT_TEX_SIZE	64
#define GK_MAX_FONT_TEX_SIZE	1024

void gkInitGlyphCollection(gkGlyphCollection* collection, gkFontEx* font, float strokeSize);
GK_BOOL gkTestCollection(gkGlyphCollection* collection, int glyphW, int glyphH);
gkGlyph* gkMakeGlyph(FT_Face face, gkGlyphCollection* collection, gkGlyphSet* glyphSet, int index);
void gkGetCellPos(gkGlyphCollection* collection, int index, int *x, int *y);

gkGlyph* gkGetGlyph(FT_Face face, gkGlyphCollection* collection, gkGlyphSet* glyphSet, uint32_t index){
	int glyphIndex = index&(0xFF>>(8 - collection->setBits));
	gkGlyph* glyph = glyphSet->glyphs[glyphIndex];
	if(!glyph){
		glyph = glyphSet->glyphs[glyphIndex] = gkMakeGlyph(face, collection, glyphSet, index);
	}
	return glyph;
}


gkGlyphCollection* gkGetGlyphCollection(gkFont* font, float strokeSize){
	gkFontFaceEx* face = (gkFontFaceEx*)font->face;
	gkGlyphCollection* p = face->collections;
	if(p){
		while(p){
			if(p->size == font->size && p->strokeSize == strokeSize) return p;
			p = p->next;
		}
	}
	p = (gkGlyphCollection*)malloc(sizeof(gkGlyphCollection));
	gkInitGlyphCollection(p, (gkFontEx*)font, strokeSize);
	if(face->collections){
		p->next = face->collections;
	}
	face->collections = p;
	return p;
}

void gkInitGlyphCollection(gkGlyphCollection* collection, gkFontEx* font, float strokeSize){
	int Gw, Gh;
	FT_Face face = ((gkFontFaceEx*)font->face)->ftface;
	Gw = FT_MulFix(face->bbox.xMax - face->bbox.xMin, face->size->metrics.x_scale)/64 + (int)strokeSize;
	Gh = FT_MulFix(face->bbox.yMax - face->bbox.yMin, face->size->metrics.y_scale)/64 + (int)strokeSize;
	collection->glyphSets = 0;
	collection->glyphSetCount = 0;
	collection->size = font->size;
	collection->strokeSize = strokeSize;
	collection->next = 0;
	collection->setBits = 8;
	collection->texWidth = GK_MIN_FONT_TEX_SIZE;
	collection->texHeight = GK_MIN_FONT_TEX_SIZE;
	collection->cellWidth = Gw;
	collection->cellHeight = Gh;
	while(!gkTestCollection(collection, Gw, Gh)){
		if(collection->texWidth<GK_MAX_FONT_TEX_SIZE){
			collection->texWidth <<=1;
		}else if(collection->texHeight<GK_MAX_FONT_TEX_SIZE){
			collection->texWidth = GK_MIN_FONT_TEX_SIZE;
			collection->texHeight <<=1;
		}else if(collection->setBits>1){
			collection->setBits--;
			collection->texWidth = collection->texHeight = GK_MIN_FONT_TEX_SIZE;
		}else{
			printf("font error\n");
		}
	}
}

GK_BOOL gkTestCollection(gkGlyphCollection* collection, int glyphW, int glyphH){
	int setSize = 0xFF>>(8 - collection->setBits);
	int glyphsPerRow = collection->texWidth / glyphW;
	if(glyphsPerRow>0){
		int rows = setSize / glyphsPerRow;
		return rows * glyphH <= collection->texHeight;
	}else{
		return GK_FALSE;
	}
}

gkGlyphSet* gkGetGlyphSet(gkGlyphCollection* collection, int index){
	gkGlyphSet* glyphSet;
	int setIndex = index>>collection->setBits;
	if(setIndex >= collection->glyphSetCount){
		int newSetCount = setIndex + 1;
		collection->glyphSets = (gkGlyphSet**)realloc(collection->glyphSets, newSetCount*sizeof(gkGlyphSet*));
		memset(collection->glyphSets + collection->glyphSetCount, 0, (newSetCount - collection->glyphSetCount)*sizeof(gkGlyphSet*));
		collection->glyphSetCount = newSetCount;
	}
	if(collection->glyphSets[setIndex] == 0){
		int setSize = (0xFF>>(8 - collection->setBits)) + 1;
		glyphSet = (gkGlyphSet*)malloc(sizeof(gkGlyphSet));
		glGenTextures(1, &glyphSet->texId);
		glBindTexture(GL_TEXTURE_2D, glyphSet->texId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, collection->texWidth, collection->texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glyphSet->glyphs = (gkGlyph**)calloc(setSize, sizeof(gkGlyph*));
		memset(glyphSet->glyphs, 0, setSize*sizeof(gkGlyph*));
		collection->glyphSets[setIndex] = glyphSet;
	}else{
		glyphSet = collection->glyphSets[setIndex];
	}
	return glyphSet;
}

void gkGetCellPos(gkGlyphCollection* collection, int index, int *x, int *y){
	int glyphIndex = index&(0xFF>>(8 - collection->setBits));
	int cols = (collection->texWidth/collection->cellWidth);
	int column = glyphIndex % cols;
	int row = glyphIndex / cols;
	*x = column*collection->cellWidth;
	*y = row*collection->cellHeight;
}

gkGlyph* gkMakeGlyph(FT_Face face, gkGlyphCollection* collection, gkGlyphSet* glyphSet, int index){
	gkGlyph* glyph;
	FT_GlyphSlot slot = face->glyph;
	FT_BBox cbox;
	FT_Glyph ftglyph;
	FT_Stroker stroker;
	FT_BitmapGlyph bglyph;
	uint8_t *buf, *pbuf;
	int r, tr, tx, ty;
	float texWidth = (float)collection->texWidth, texHeight = (float)collection->texHeight;

	if(collection->strokeSize == 0){

		if(FT_Load_Glyph(face, index, FT_LOAD_DEFAULT)) return 0;
		if(FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) return 0;

		glyph = (gkGlyph*)malloc(sizeof(gkGlyph));

		pbuf = buf = (uint8_t*)malloc(slot->bitmap.width*slot->bitmap.rows*4*sizeof(uint8_t));
		for(r = 0; r<slot->bitmap.rows; r++){
			uint8_t *bmp = slot->bitmap.buffer + r*slot->bitmap.pitch;
			pbuf = buf + r*slot->bitmap.width*4;
			for(tr = 0; tr<slot->bitmap.width; tr++){
				*pbuf++ = 0xFF;
				*pbuf++ = 0xFF;
				*pbuf++ = 0xFF;
				*pbuf++ = *bmp++;
			}
		}

		glyph->size.width = (float)slot->bitmap.width;
		glyph->size.height = (float)slot->bitmap.rows;

		glyph->offset.x = (float)slot->bitmap_left;
		glyph->offset.y = (float)slot->bitmap_top;

		glyph->advance.x = ((float)slot->metrics.horiAdvance)/64.0f;
		glyph->advance.y = ((float)slot->metrics.vertAdvance)/64.0f;

		FT_Get_Glyph(slot, &ftglyph);
		FT_Glyph_Get_CBox(ftglyph, ft_glyph_bbox_pixels, &cbox);
		FT_Done_Glyph(ftglyph);

		glyph->bbox = gkCreateBBox((float)(cbox.xMin), (float)(-cbox.yMax), (float)(cbox.xMax), (float)(-cbox.yMin));

		glyph->texId = glyphSet->texId;

		gkGetCellPos(collection, index, &tx, &ty);

		glBindTexture(GL_TEXTURE_2D, glyphSet->texId);
		glTexSubImage2D(GL_TEXTURE_2D, 0, tx, ty, slot->bitmap.width, slot->bitmap.rows, GL_RGBA, GL_UNSIGNED_BYTE, buf);

		glyph->texCoords.x = (float)tx/texWidth;
		glyph->texCoords.y = (float)ty/texHeight;
		glyph->texCoords.width = (float)(tx + slot->bitmap.width)/texWidth - glyph->texCoords.x;
		glyph->texCoords.height = (float)(ty + slot->bitmap.rows)/texHeight - glyph->texCoords.y;
		free(buf);

	}else{
		if(FT_Load_Glyph(face, index, FT_LOAD_DEFAULT)) return 0;

		FT_Stroker_New(ftlib, &stroker);
		FT_Stroker_Set(stroker, (FT_Fixed)(collection->strokeSize*16.0f), FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);

		FT_Get_Glyph(slot, &ftglyph);
		if(FT_Glyph_Stroke(&ftglyph, stroker, GK_TRUE) != 0){
			FT_Done_Glyph(ftglyph);
			return 0;
		}

		FT_Glyph_To_Bitmap(&ftglyph, FT_RENDER_MODE_NORMAL, 0, 1);
		bglyph = (FT_BitmapGlyph)ftglyph;

		glyph = (gkGlyph*)malloc(sizeof(gkGlyph));

		pbuf = buf = (uint8_t*)malloc(bglyph->bitmap.width*bglyph->bitmap.rows*4*sizeof(uint8_t));
		for(r = 0; r<bglyph->bitmap.rows; r++){
			uint8_t *bmp = bglyph->bitmap.buffer + r*bglyph->bitmap.pitch;
			pbuf = buf + r*bglyph->bitmap.width*4;
			for(tr = 0; tr<bglyph->bitmap.width; tr++){
				*pbuf++ = 0xFF;
				*pbuf++ = 0xFF;
				*pbuf++ = 0xFF;
				*pbuf++ = *bmp++;
			}
		}

		glyph->size.width = (float)bglyph->bitmap.width;
		glyph->size.height = (float)bglyph->bitmap.rows;

		glyph->offset.x = (float)bglyph->left;
		glyph->offset.y = (float)bglyph->top;

		glyph->advance.x = ((float)slot->metrics.horiAdvance)/64.0f;
		glyph->advance.y = ((float)slot->metrics.vertAdvance)/64.0f;

		glyph->texId = glyphSet->texId;

		gkGetCellPos(collection, index, &tx, &ty);

		glBindTexture(GL_TEXTURE_2D, glyphSet->texId);
		glTexSubImage2D(GL_TEXTURE_2D, 0, tx, ty, bglyph->bitmap.width, bglyph->bitmap.rows, GL_RGBA, GL_UNSIGNED_BYTE, buf);

		glyph->texCoords.x = (float)tx/texWidth;
		glyph->texCoords.y = (float)ty/texHeight;
		glyph->texCoords.width = (float)(tx + bglyph->bitmap.width)/texWidth - glyph->texCoords.x;
		glyph->texCoords.height = (float)(ty + bglyph->bitmap.rows)/texHeight - glyph->texCoords.y;

		free(buf);
	}
	glyph->index = index;
	return glyph;
}

void gkDestroyFace(gkFontFaceEx* face){
	gkGlyphCollection* p = face->collections, *r;
	while(p){
		p = (r = p)->next;
		gkDestroyGlyphCollection(r);
	}
	FT_Done_Face(face->ftface);
	free(face);
}

void gkDestroyGlyphCollection(gkGlyphCollection* collection){
	int i;
	int setSize = 0xFF>>(8 - collection->setBits);
	for(i = 0; i<collection->glyphSetCount; i++){
		gkGlyphSet* set = collection->glyphSets[i];
		if(set) gkDestroyGlyphSet(set, setSize);
	}
	free(collection->glyphSets);
	free(collection);
}

void gkDestroyGlyphSet(gkGlyphSet* set, int setSize){
	int i;
	for(i = 0; i<setSize; i++){
		if(set->glyphs[i]) free(set->glyphs[i]);
	}
	free(set->glyphs);
	glDeleteTextures(1, &set->texId);
	free(set);
}

/*
	Drawing and measuring
*/

typedef struct gkSentenceElementStruct gkSentenceElement;
struct gkSentenceElementStruct{
	uint8_t type;
	gkPoint advance;
	gkBBox bbox;
	gkGlyph** glyphStart;
	gkGlyph** glyphEnd;
	gkGlyph** strokeStart;
	gkGlyph** strokeEnd;
	gkSentenceElement* next;
};
typedef struct gkSentenceLineStruct gkSentenceLine;
struct gkSentenceLineStruct{
	gkSentenceElement* first;
	gkSentenceElement* last;
	gkBBox bbox;
	gkSentenceLine* next;
};

#define GK_SE_WORD		0
#define GK_SE_TAB		1
#define GK_SE_SPACE		2
#define GK_SE_NEWLINE	3

gkSentenceElement* gkParseSentenceElements(gkFont* font, wchar_t* text, gkTextFormat* format);
void gkFreeSentenceElements(gkSentenceElement* elements);
gkSentenceLine* gkParseSentenceLines(gkSentenceElement* elements, gkTextFormat* format);
void gkFreeSentenceLines(gkSentenceLine* lines);
gkBBox gkGetTextBBox(gkSentenceLine* lines, gkTextFormat* format, float leading);
gkPoint gkAlignLine(gkSentenceLine* line, gkTextFormat* format, gkBBox* textBBox);
gkPoint gkDrawSentenceLine(FT_Face face, gkSentenceLine* line, gkTextFormat* format);

gkSize gkMeasureText(gkFont* font, wchar_t* text, gkTextFormat* format){
	return GK_SIZE(0,0);
}

gkPoint gkDrawText(gkFont* font, wchar_t* text, float x, float y, gkTextFormat* format){
	FT_Face face = ((gkFontFaceEx*)font->face)->ftface;
	gkSentenceElement* elements;
	gkSentenceLine* lines, *currentLine;
	float oldTextFormatWidth;
	if(format == 0) format = &gkDefaultTextFormat;
	oldTextFormatWidth = format->width;
	if(elements = gkParseSentenceElements(font, text, format)){
		gkPoint align;
		float tx = x, ty = y;
		float leading = ((float)face->size->metrics.height)/64.0f + format->lineSpacing;
		gkBBox textBBox;
		currentLine = lines = gkParseSentenceLines(elements, format);
		textBBox = gkGetTextBBox(lines, format, leading);
		while(currentLine){
			glPushMatrix();
			align = gkAlignLine(currentLine, format, &textBBox);
			glTranslatef(tx + align.x,ty + align.y,0);
			gkDrawSentenceLine(face, currentLine, format);
			glPopMatrix();
			currentLine = currentLine->next;
			if(format->vertical){
				tx -= leading;
			}else{
				ty += leading;
			}
		}
		gkFreeSentenceLines(lines);
		gkFreeSentenceElements(elements);
	}
	format->width = oldTextFormatWidth;
	return GK_POINT(0,0);
}

gkSentenceElement* gkParseSentenceElements(gkFont* font, wchar_t* text, gkTextFormat* format){
	FT_Face face = ((gkFontFaceEx*)font->face)->ftface;
	GK_BOOL hasKerning = FT_HAS_KERNING(face);

	size_t totalGlyphs = 0, currentGlyph = 0;
	gkGlyph** glyphs, **strokes = 0;
	gkGlyphCollection* collection, *strokeCollection;
	gkSentenceElement* firstElement = 0, *lastElement = firstElement, *currentElement = 0;
	wchar_t* c = text, lastChar;

	while(*c){
		if(*c != L' ' && *c != L'\t' && *c != L'\r' && *c != L'\n') totalGlyphs++;
		c++;
	}
	if(totalGlyphs > 0){
		int index, prevIndex = 0;
		gkPoint spaceAdvance;
		gkGlyph* glyph = 0;
		glyphs = (gkGlyph**)calloc(totalGlyphs, sizeof(gkGlyph*));
		c = text;
		FT_Set_Char_Size(face, 0, font->size*64, 0, 96);
		collection = gkGetGlyphCollection(font, 0);
		if(format->strokeSize>0.0f){
			strokes = (gkGlyph**)calloc(totalGlyphs, sizeof(gkGlyph*));
			strokeCollection = gkGetGlyphCollection(font, format->strokeSize);
		}
		FT_Load_Char(face, L' ', FT_LOAD_DEFAULT);
		spaceAdvance.x = ((float)face->glyph->advance.x)/64.0f;
		spaceAdvance.y = ((float)face->glyph->advance.y)/64.0f;
		if(spaceAdvance.y == 0) spaceAdvance.y = spaceAdvance.x;
		lastChar = *c;
		while(*c){
			if(*c != L' ' && *c != L'\t' && *c != L'\r' && *c != L'\n'){
				index = FT_Get_Char_Index(face, *c);
				if(strokes) strokes[currentGlyph] = gkGetGlyph(face, strokeCollection, gkGetGlyphSet(strokeCollection, index), index);
				glyphs[currentGlyph] = glyph = gkGetGlyph(face, collection, gkGetGlyphSet(collection, index), index);
				if(currentElement && currentElement->type != GK_SE_WORD){
					lastElement->next = currentElement;
					lastElement = currentElement;
					currentElement = 0;
				}
				if(currentElement == 0){
					currentElement = (gkSentenceElement*)malloc(sizeof(gkSentenceElement));
					currentElement->type = GK_SE_WORD;
					currentElement->glyphStart = glyphs + currentGlyph;
					currentElement->glyphEnd = glyphs + currentGlyph;
					currentElement->strokeStart = strokes + currentGlyph;
					currentElement->strokeEnd = strokes + currentGlyph;
					currentElement->advance.x = glyph->advance.x;
					currentElement->advance.y = glyph->advance.y;
					currentElement->bbox = glyph->bbox;
					currentElement->next = 0;
				}else{
					gkBBox tbbox = glyph->bbox;
					currentElement->glyphEnd = glyphs + currentGlyph;
					currentElement->strokeEnd = strokes + currentGlyph;
					if(format->vertical){
						gkBBoxTranslate(&tbbox, 0, currentElement->advance.y);
					}else{
						gkBBoxTranslate(&tbbox, currentElement->advance.x, 0);
					}
					gkBBoxAdd(&currentElement->bbox, &tbbox);
					currentElement->advance.x += glyph->advance.x;
					currentElement->advance.y += glyph->advance.y;
					if(hasKerning){
						FT_Vector delta;
						FT_Get_Kerning(face, prevIndex, index, FT_KERNING_DEFAULT, &delta);
						currentElement->advance.x += ((float)delta.x)/64.0f;
						currentElement->advance.y += ((float)delta.y)/64.0f;
					}
				}
				prevIndex = index;
				currentGlyph++;
			}else{
				if(currentElement && currentElement->type == GK_SE_WORD){
					lastElement->next = currentElement;
					lastElement = currentElement;
				}
				currentElement = 0;
				if(*c == L'\t'){
					currentElement = (gkSentenceElement*)malloc(sizeof(gkSentenceElement));
					currentElement->type = GK_SE_TAB;
					currentElement->advance = spaceAdvance;
					if(format->vertical){
						currentElement->bbox = gkCreateBBox(0, 0, 0, spaceAdvance.y);
					}else{
						currentElement->bbox = gkCreateBBox(0, 0, spaceAdvance.x, 0);
					}
					currentElement->next = 0;
				}else if(*c == L' '){
					currentElement = (gkSentenceElement*)malloc(sizeof(gkSentenceElement));
					currentElement->type = GK_SE_SPACE;
					currentElement->advance = spaceAdvance;
					if(format->vertical){
						currentElement->bbox = gkCreateBBox(0, 0, 0, spaceAdvance.y);
					}else{
						currentElement->bbox = gkCreateBBox(0, 0, spaceAdvance.x, 0);
					}
					currentElement->next = 0;
				}else if((*c == L'\n' && lastChar != '\r') || *c == L'\r'){
					currentElement = (gkSentenceElement*)malloc(sizeof(gkSentenceElement));
					currentElement->type = GK_SE_NEWLINE;
					currentElement->bbox = gkCreateBBox(0,0,0,0);
					currentElement->next = 0;
				}
				if(currentElement && lastElement){
					lastElement->next = currentElement;
					lastElement = currentElement;
					currentElement = 0;
				}
				prevIndex  = 0;
			}
			if(firstElement == 0 && currentElement){
				firstElement = lastElement = currentElement;
			}
			lastChar = *c;
			c++;
		}
		if(currentElement && currentElement != lastElement){
			lastElement->next = currentElement;
			lastElement = currentElement;
			currentElement = 0;
		}
	}
	return firstElement;
}

void gkFreeSentenceElements(gkSentenceElement* elements){
	gkSentenceElement* p = elements, *r;
	free(p->strokeStart);
	free(p->glyphStart);
	while(p){
		p = (r = p)->next;
		free(r);
	}
}

gkSentenceLine* gkParseSentenceLines(gkSentenceElement* elements, gkTextFormat* format){
	gkSentenceElement* currentElement = elements;
	gkSentenceLine *firstLine = (gkSentenceLine*)malloc(sizeof(gkSentenceLine)), *current = firstLine;
	gkPoint advance = {0,0};
	memset(firstLine, 0, sizeof(gkSentenceLine));
	while(currentElement){
		if(currentElement->type == GK_SE_NEWLINE){
			gkSentenceLine* newLine = (gkSentenceLine*)malloc(sizeof(gkSentenceLine));
			memset(newLine, 0, sizeof(gkSentenceLine));
			current->next = newLine;
			current = newLine;
		}else{
			if(current->first == 0){
				current->first = current->last = currentElement;
				current->bbox = currentElement->bbox;
				advance = currentElement->advance;
			}else{
				gkBBox tbbox = currentElement->bbox;
				if(format->vertical){
					gkBBoxTranslate(&tbbox, 0, advance.y);
				}else{
					gkBBoxTranslate(&tbbox, advance.x, 0);
				}
				gkBBoxAdd(&current->bbox, &tbbox);
				current->last = currentElement;
				advance.x += currentElement->advance.x;
				advance.y += currentElement->advance.y;
			}
		}
		currentElement = currentElement->next;
	}
	return firstLine;
}

void gkFreeSentenceLines(gkSentenceLine* lines){
	gkSentenceLine* p;
	while(lines){
		lines = (p = lines)->next;
		free(p);
	}
}

gkBBox gkGetTextBBox(gkSentenceLine* lines, gkTextFormat* format, float leading){
	gkBBox res = lines->bbox;
	gkSentenceLine* currentLine = lines;
	float tx = 0, ty = 0;
	while(currentLine){
		gkBBox b = currentLine->bbox;
		gkBBoxTranslate(&b, tx, ty);
		gkBBoxAdd(&res, &b);
		if(format->vertical){
			tx -= leading;
		}else{
			ty += leading;
		}
		currentLine = currentLine->next;
	}
	return res;
}
gkPoint gkAlignLine(gkSentenceLine* line, gkTextFormat* format, gkBBox* textBBox){
	gkPoint result = {0,0};
	float width;
	float height;
	if(format->width == 0){
		width = textBBox->maxX - textBBox->minX;
	}else{
		width = format->width;
	}
	if(format->height == 0){
		height = textBBox->maxY - textBBox->minY;
	}else{
		height = format->height;
	}
	if(format->vertical){
		if(format->align == GK_TEXT_ALIGN_LEFT){
			result.x = -textBBox->minX;
		}else if(format->align == GK_TEXT_ALIGN_CENTER){
			result.x = (width - (textBBox->maxX - textBBox->minX))/2 - textBBox->minX;
		}else if(format->align == GK_TEXT_ALIGN_RIGHT){
			result.x = (width - (textBBox->maxX - textBBox->minX)) - textBBox->minX;
		}
		if(format->valign == GK_TEXT_VALIGN_TOP){
			result.y = -line->bbox.minY;
		}else if(format->valign == GK_TEXT_VALIGN_MIDDLE){
			result.y = (height - (line->bbox.maxY - line->bbox.minY))/2 - line->bbox.minY;
		}else if(format->valign == GK_TEXT_VALIGN_BOTTOM){
			result.y = (height - (line->bbox.maxY - line->bbox.minY)) - line->bbox.minY;
		}
	}else{
		if(format->align == GK_TEXT_ALIGN_LEFT){
			result.x = -line->bbox.minX;
		}else if(format->align == GK_TEXT_ALIGN_CENTER){
			result.x = (width - (line->bbox.maxX - line->bbox.minX))/2 - line->bbox.minX;
		}else if(format->align == GK_TEXT_ALIGN_RIGHT){
			result.x = (width - (line->bbox.maxX - line->bbox.minX)) - line->bbox.minX;
		}
		if(format->valign == GK_TEXT_VALIGN_TOP){
			result.y = -textBBox->minY;
		}else if(format->valign == GK_TEXT_VALIGN_MIDDLE){
			result.y = (height - (textBBox->maxY - textBBox->minY))/2 - textBBox->minY;
		}else if(format->valign == GK_TEXT_VALIGN_BOTTOM){
			result.y = (height - (textBBox->maxY - textBBox->minY)) - textBBox->minY;
		}
	}
	return result;
}

gkPoint gkDrawSentenceLine(FT_Face face, gkSentenceLine* line, gkTextFormat* format){
	gkGlyph *g, **c, **firstGlyph, **lastGlyph;
	gkPoint b = GK_POINT(0,0), p;
	GLuint texImage = 0;
	int prevIndex = 0;
	GK_BOOL stroke = format->strokeSize>0;
	GK_BOOL hasKerning = FT_HAS_KERNING(face);
	gkSentenceElement* currentElement;
	gkColor tmpColor;
	if(line->first == 0) return GK_POINT(0,0);
draw:
	if(stroke){
		tmpColor = gkGetFilteredColor(format->strokeColor);
	}else{
		tmpColor = gkGetFilteredColor(format->textColor);
	}
	if(format->underline && format->vertical == GK_FALSE){
		float underlinePos = ((float)FT_MulFix(face->underline_position, face->size->metrics.y_scale))/64.0f;
		float underlineThickness = ((float)FT_MulFix(face->underline_thickness, face->size->metrics.y_scale))/64.0f;
		if(stroke){
			float lw = format->strokeSize/3.0f;
			underlineThickness += lw;
			gkSetLineWidth(lw);
			gkSetLineStipple(1,0xffff);
			gkSetLineColor(tmpColor.r, tmpColor.g, tmpColor.b, tmpColor.a);
			gkSetFillColor(0,0,0,0);
		}else{
			gkSetLineWidth(0);
			gkSetFillColor(tmpColor.r, tmpColor.g, tmpColor.b, tmpColor.a);
		}
		gkDrawRect(-underlineThickness*0.5f + line->bbox.minX, -underlinePos - underlineThickness*0.5f, underlineThickness + (line->bbox.maxX - line->bbox.minX), underlineThickness);
	}
	glColor4f(tmpColor.r, tmpColor.g, tmpColor.b, tmpColor.a);
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	currentElement = line->first;
	while(1){
		if(currentElement->type == GK_SE_WORD){
			if(stroke){
				firstGlyph = currentElement->strokeStart;
				lastGlyph = currentElement->strokeEnd;
			}else{
				firstGlyph = currentElement->glyphStart;
				lastGlyph = currentElement->glyphEnd;
			}
			for(c = firstGlyph; c <= lastGlyph; c++){
				g = *c;
				if(hasKerning){
					FT_Vector delta;
					FT_Get_Kerning(face, prevIndex, g->index, FT_KERNING_DEFAULT, &delta);
					if(format->vertical){
						b.y += ((float)delta.y)/64.0f;
					}else{
						b.x += ((float)delta.x)/64.0f;
					}
				}
				p.x = b.x + g->offset.x;
				p.y = b.y - g->offset.y;
				if(texImage == 0 || texImage != g->texId){
					texImage = g->texId;
					glBindTexture(GL_TEXTURE_2D, texImage);
				}
				{
					float v[] = {
						p.x, p.y,
						p.x + g->size.width, p.y,
						p.x + g->size.width, p.y + g->size.height,
						p.x, p.y + g->size.height
					};
					float c[] = {
						g->texCoords.x, g->texCoords.y,
						g->texCoords.x + g->texCoords.width, g->texCoords.y,
						g->texCoords.x + g->texCoords.width, g->texCoords.y + g->texCoords.height,
						g->texCoords.x, g->texCoords.y + g->texCoords.height,
					};
					glVertexPointer(2, GL_FLOAT, 0, v);
					glTexCoordPointer(2, GL_FLOAT, 0, c);
					glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
				}
				if(format->vertical){
					b.y += g->advance.y;
				}else{
					b.x += g->advance.x;
				}
				prevIndex = g->index;
			}
		}else if(currentElement->type == GK_SE_SPACE){
			if(format->vertical){
				b.y += currentElement->advance.y;
			}else{
				b.x += currentElement->advance.x;
			}
		}else if(currentElement->type == GK_SE_TAB){
			//make tabulations
		}
		if(currentElement == line->last) break;
		currentElement = currentElement->next;
	}
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);

	if(stroke){
		b = GK_POINT(0,0);
		prevIndex = 0;
		stroke = GK_FALSE;
		goto draw;
	}
	return GK_POINT(0,0);
}
