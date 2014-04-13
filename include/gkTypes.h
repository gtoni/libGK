/* Copyright (c) 2014 Toni Georgiev
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

#ifndef _GK_TYPES_H_
#define _GK_TYPES_H_

/*
 * Basic data types used through the library
 */

#ifdef _MSC_VER
	#if (_MSC_VER<1300)
		typedef unsigned char uint8_t;
		typedef unsigned short uint16_t;
		typedef unsigned int uint32_t;
		typedef unsigned long long int uint64_t;
		typedef signed char int8_t;
		typedef signed short int16_t;
		typedef signed int	int32_t;
		typedef signed long long int int64_t;
	#else
		typedef unsigned __int8 uint8_t;
		typedef unsigned __int16 uint16_t;
		typedef unsigned __int32 uint32_t;
		typedef unsigned __int64 uint64_t;
		typedef signed __int8 int8_t;
		typedef signed __int16 int16_t;
		typedef signed __int32 int32_t;
		typedef signed __int64 int64_t;
	#endif
#else
	#include <stdint.h>
#endif


#define GK_BOOL		int
#define GK_TRUE		1
#define GK_FALSE	0

#define GK_BYTE			0
#define GK_SHORT		1
#define GK_INT			2
#define GK_UNSIGNED_BYTE	3
#define GK_UNSIGNED_SHORT	4
#define GK_UNSIGNED_INT		5
#define GK_FLOAT		6
#define GK_DOUBLE		7


typedef struct gkListNode{
	struct gkListNode *next, *prev;
	void *data;
}gkListNode;

typedef struct gkList{
	gkListNode *first, *last;
	uint32_t length;
}gkList;

typedef enum gkListPosition{
	GK_LIST_BEGIN = 0,
	GK_LIST_END = -1
}gkListPosition;

gkList*	gkCreateList();
void	gkInitList(gkList* list);
void	gkClearList(gkList* list, void (*destructor)(void*));
void	gkDestroyList(gkList* list, void (*destructor)(void*));

void	gkListAdd(gkList* list, gkListPosition index, void* data);
void*	gkListRemove(gkList* list, gkListPosition index);
void*	gkListGet(gkList* list, gkListPosition index);

#define gkListLength(list) ((list)->length)
#define gkListEmpty(list) ((list)->first == 0)

gkListNode*	gkListRemoveNode(gkList* list, gkListNode* node);
gkListNode*	gkListFind(gkList* list, void *data);
void		gkListForEach(gkList* list, void (*iterator)(void* ));

void	gkListSort(gkList* list, int (*less)(void*, void*));


#include <string.h>

size_t	gkUtf8Length(char* string);
char*	gkUtf8CharCode(char* string, uint32_t* dstCharCode);
char*	gkUtf8Char(char* dst, uint32_t charCode, size_t dstSize);

size_t	gkUtf8ToWcs(wchar_t* dst, char* src, size_t dstSize);
size_t	gkWcsToUtf8(char* dst, wchar_t* src, size_t dstSize);

wchar_t* gkWcsFromUtf8(char* src);
char*	 gkUtf8FromWcs(wchar_t* src);

#endif