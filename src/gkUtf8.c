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

#include <gkTypes.h>
#include <stdlib.h>

size_t	gkUtf8Length(char* str)
{
	size_t len = 0;
	char* p = str;
	while (*p)
		if ((*p++ & 0xC0) !=  0x80) len++;
	return len;
}

char*	gkUtf8CharCode(char* str, unsigned int* dstCharCode)
{
	int result = 0, bytesLeft = 0;
	char* p = str, c = *p;
	if (!c) {
		*dstCharCode = 0;
		return p;
	}
	do{
		if ((c & 0xC0) == 0x80) {
			result = (result << 6) | (c & 0x7F);
		} else {
			bytesLeft = 0;
			while (c&0x80) {
				bytesLeft++;
				c = c<<1;
			}
			result = c>>bytesLeft;
		}
		c = *++p;
	}while (c && --bytesLeft>0);
	*dstCharCode = result;
	return p;
}

size_t	gkUtf8ToWcs(wchar_t* dst, char* src, size_t dstSize)
{
	size_t pos, maxPos = dstSize/sizeof(wchar_t);
	wchar_t* d = dst;
	char* s = src;
	unsigned int uc;

	if (d) {
		for (pos = 0, uc = 1; pos < maxPos && uc; pos++) {
			s = gkUtf8CharCode(s, &uc);
			*d++ = (wchar_t)uc;
		}
	} else {
		return (gkUtf8Length(src) + 1)*sizeof(wchar_t);
	}
	return pos*sizeof(wchar_t);
}

static size_t utf8RequiredBytes(unsigned int c)
{
	if (c<0x80)
		return 1;
	else if (c<0x800)
		return 2;
	else if (c<0x10000)
		return 3;
	else if (c<0x200000)
		return 4;
	else if (c<0x4000000)
		return 5;
	return 6;
}

char* gkUtf8Char(char* dst, uint32_t c, size_t dstSize)
{
	size_t bytesRequired = utf8RequiredBytes(c);
	int i;
	char r = 0;

	if(bytesRequired>dstSize)
		return 0;

	if (bytesRequired == 1) {	
		*dst = (char)c;
		return dst + 1;
	}

	for(i = bytesRequired - 1; i>=0; i--)
	{
		dst[i] = 0x80 | (c&0x3F);
		c = c>>6;
		r |= (r>>1) | 0x80;
	}
	*dst |= r;
	return dst + bytesRequired;
}

size_t	gkWcsToUtf8(char* dst, wchar_t* src, size_t dstSize)
{
	size_t pos, dstLen = 0;
	char* d = dst;
	wchar_t* s = src;

	if (d) {
		for (pos = 0; d && pos<dstSize; pos++, s++) {
			d = gkUtf8Char(d, *s, dstSize - dstLen);
			dstLen = d - dst;
			if (!*s) break;
		}
	} else {
		pos = 1;
		while (*s)
			pos += utf8RequiredBytes((uint32_t)*s++);
	}

	return pos;
}


wchar_t* gkWcsFromUtf8(char* src)
{
	size_t size = gkUtf8ToWcs(0, src, 0);
	wchar_t* res = (wchar_t*)malloc(size);
	gkUtf8ToWcs(res, src, size);
	return res;
}

char*	gkUtf8FromWcs(wchar_t* src)
{
	size_t size = gkWcsToUtf8(0, src, 0);
	char* res = (char*)malloc(size);
	gkWcsToUtf8(res, src, size);
	return res;
}