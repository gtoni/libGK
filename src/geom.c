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

#include <math.h>

void sincosf(float a, float* s, float* c);

gkMatrix GK_IDENTIY_MATRIX = {1,0,0,
							  0,1,0,
							  0,0,1};

gkPoint	GK_POINT(float x, float y){ gkPoint p = {x,y}; return p;};
gkSize GK_SIZE(float width, float height){ gkSize s = {width,height}; return s;};
gkRect GK_RECT(float x, float y, float width, float height){ gkRect r = {x,y,width,height}; return r;};

gkMatrix gkMatrixCreateIdentity(){
	return GK_IDENTIY_MATRIX;
}

gkMatrix gkMatrixCreateTranslation(float x, float y){
	gkMatrix res = { 1, 0, 0,
					 0, 1, 0,
					 x, y, 1
				   };
	return res;
}

gkMatrix gkMatrixCreateScale(float sx, float sy){
	gkMatrix res = { sx, 0, 0,
					 0, sy, 0,
					 0, 0, 1
				   };
	return res;
}

void sincosf(const float a, float* s, float* c){
#ifdef _WIN32
	_asm{
		mov eax, c
		mov esi, s
		fld a
		fwait
		fsincos
		fwait
		fstp [eax]
		fstp [esi]
	}
#else
    *s = sinf(a);
    *c = cosf(a);
#endif
}

gkMatrix gkMatrixCreateRotation(float angle){
	float cosa, sina;
	sincosf(angle, &sina, &cosa);
	{
		gkMatrix res = { cosa, -sina, 0,
						 sina, cosa, 0,
						 0, 0, 1
					   };
		return res;
	}
}

void gkMatrixMult(gkMatrix* dst, gkMatrix mat){
	gkMatrixMultPtr(dst, &mat);
}
void gkMatrixMultPtr(gkMatrix* dst, gkMatrix *mat){		// dst = dst * mat
	float *B = (float*)mat->data, *A;
	gkMatrix tmp = *dst;
	int c,r,r3;
	A = tmp.data;
	for(r = 0; r<3; r++){
		r3 = r*3;
		for(c = 0; c<3; c++){
			dst->data[r3 + c] = B[c]*A[r3] + B[c + 3]*A[r3 + 1] + B[c + 6]*A[r3 + 2];
		}
	}
}

float gkMatrixDeterminant(gkMatrix* mat){
	float* m = mat->data;
	return m[0]*m[4]*m[8] + m[3]*m[7]*m[2] + m[6]*m[1]*m[5] -
		   m[6]*m[4]*m[2] - m[3]*m[1]*m[8] - m[0]*m[7]*m[5];
}

void gkMatrixInverse(gkMatrix* mat){
	float d = gkMatrixDeterminant(mat);
	if(d == 0) *mat = GK_IDENTIY_MATRIX;
	else{
		int i;
		float* m = mat->data;
		gkMatrix inv = {
				(m[4]*m[8] - m[7]*m[5]), -(m[1]*m[8] - m[7]*m[2]), (m[1]*m[5] - m[4]*m[2]),
				-(m[3]*m[8] - m[6]*m[5]), (m[0]*m[8] - m[6]*m[2]), -(m[0]*m[5] - m[3]*m[2]),
				(m[3]*m[7] - m[6]*m[4]), -(m[0]*m[7] - m[6]*m[1]), (m[0]*m[4] - m[3]*m[1])
		};
		for(i = 0; i<9; i++) m[i] /= d;
		*mat = inv;
	}
}

void gkMatrixTranspose(gkMatrix* mat){
	int i,j, i3, j3;
	float *m = mat->data, tmp;
	for(i = 0; i<3; i++){
		i3 = i*3;
		for(j = i; j<3; j++){
			j3 = j*3;
			tmp = m[i3 + j];
			m[i3 + j] = m[j3 + i];
			m[j3 + i] = tmp;
		}
	}
}

gkPoint gkTransformPoint(gkPoint p, gkMatrix* mat){
	float *m = mat->data;
	gkPoint result;
//	z = p.x*m[6] + p.y*m[7] + m[8];
	result.x = p.x*m[0] + p.y*m[3] + m[6];
	result.y = p.x*m[1] + p.y*m[4] + m[7];
	return result;
}
