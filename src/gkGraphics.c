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

#include "gkGL.h"

#include <stdlib.h>

#define M_PI_OVER_180 0.017453292519943295769f
#define M_180_OVER_PI 57.29577951308232087684f

void sincosf(float a, float* s, float* c);  /*in geom.c*/

/* Color filter stack */

typedef struct gkColorNodeStruct gkColorNode;
struct gkColorNodeStruct{
	gkColor color;
	gkColorNode* parent;
}gkColorFilterRoot = {{1.0f,1.0f,1.0f,1.0f}, 0}, *gkColorFilterTop = &gkColorFilterRoot;

void gkResetColorFilter(){
	gkColorNode *p;
	while(gkColorFilterTop->parent){
		gkColorFilterTop = (p = gkColorFilterTop)->parent;
		free(p);
	}
}

void gkPushColorFilter(float r, float g, float b, float a){
	gkColorNode* n = (gkColorNode*)malloc(sizeof(gkColorNode));
	n->color = GK_COLOR(gkColorFilterTop->color.r*r, gkColorFilterTop->color.g*g, gkColorFilterTop->color.b*b, gkColorFilterTop->color.a*a);
	n->parent = gkColorFilterTop;
	gkColorFilterTop = n;
}

gkColor gkGetFilteredColor(gkColor c){
	c.r *= gkColorFilterTop->color.r;
	c.g *= gkColorFilterTop->color.g;
	c.b *= gkColorFilterTop->color.b;
	c.a *= gkColorFilterTop->color.a;
	return c;
}

void gkPopColorFilter(){
	if(gkColorFilterTop->parent){
		gkColorNode* p = gkColorFilterTop;
		gkColorFilterTop = gkColorFilterTop->parent;
		free(p);
	}
}

/* Transform stack */

typedef struct gkTransformNodeStruct gkTransformNode;
struct gkTransformNodeStruct{
	gkMatrix transform;
	gkTransformNode *parent;
}gkTransformRoot = {{1,0,0,0,1,0,0,0,1}, 0}, *gkTransformTop = &gkTransformRoot;

void gkUpdateGLMatrix(){
	float *p = gkTransformTop->transform.data, m[16] = {
				p[0], p[1], 0, p[2],
				p[3], p[4], 0, p[5],
				0   , 0   , 1,    0,
				p[6], p[7], 0, p[8]
	};
	glLoadMatrixf(m);
}

void gkResetTransform(){
	gkTransformNode *p;
	while(gkTransformTop->parent){
		gkTransformTop = (p = gkTransformTop)->parent;
		free(p);
	}
	glLoadIdentity();
}

void gkPushTransform(gkMatrix* matrix){
	gkTransformNode* n = (gkTransformNode*)malloc(sizeof(gkTransformNode));
	n->transform = *matrix;
	gkMatrixMultPtr(&n->transform, &gkTransformTop->transform);
	n->parent = gkTransformTop;
	gkTransformTop = n;
	gkUpdateGLMatrix();
}

void gkPopTransform(){
	if(gkTransformTop->parent){
		gkTransformNode* p = gkTransformTop;
		gkTransformTop = gkTransformTop->parent;
		free(p);
	}
	gkUpdateGLMatrix();
}

/***********************/

gkColor gkFillColor = {0.0f,0.0f,0.0f,1.0f};
gkColor gkLineColor = {1.0f,1.0f,1.0f,1.0f};
float gkLineWidth = 0;

gkColor GK_COLOR(float r, float g, float b, float a){
	gkColor c = {r,g,b,a};
	return c;
}

void gkSetFillColor(float r, float g, float b, float a){
	gkFillColor.r = r;
	gkFillColor.g = g;
	gkFillColor.b = b;
	gkFillColor.a = a;
}

void gkSetLineColor(float r, float g, float b, float a){
	gkLineColor.r = r;
	gkLineColor.g = g;
	gkLineColor.b = b;
	gkLineColor.a = a;
}

void gkSetLineWidth(float w){
	gkLineWidth = w;
}

GK_BOOL gkCheckLineProperties(){
	if(gkLineWidth > 0){
		gkColor lineColor = gkGetFilteredColor(gkLineColor);
		glColor4f(lineColor.r, lineColor.g, lineColor.b, lineColor.a);
		glLineWidth(gkLineWidth);
		return GK_TRUE;
	}
	return GK_FALSE;
}

void gkSetClipRect(float x, float y, float w, float h){
	if(w != 0 && h != 0){
		glClipPlaneVarType tClip[] = {0, 1, 0, -y};
		glClipPlaneVarType bClip[] = {0, -1, 0, (y + h)};
		glClipPlaneVarType lClip[] = {1, 0, 0, -x};
		glClipPlaneVarType rClip[] = {-1, 0, 0, (x + w)};
		glClipPlanef(GL_CLIP_PLANE0, tClip);
		glClipPlanef(GL_CLIP_PLANE1, bClip);
		glClipPlanef(GL_CLIP_PLANE2, lClip);
		glClipPlanef(GL_CLIP_PLANE3, rClip);
		glEnable(GL_CLIP_PLANE0);
		glEnable(GL_CLIP_PLANE1);
		glEnable(GL_CLIP_PLANE2);
		glEnable(GL_CLIP_PLANE3);
	}else{
		glDisable(GL_CLIP_PLANE0);
		glDisable(GL_CLIP_PLANE1);
		glDisable(GL_CLIP_PLANE2);
		glDisable(GL_CLIP_PLANE3);
	}
}


void gkDrawPoint(float x, float y, float size){
	float v[] = {x,y};
	gkColor fillColor = gkGetFilteredColor(gkFillColor);
	glPointSize(size);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, v);
	glColor4f(fillColor.r, fillColor.g, fillColor.b, fillColor.a);
	glDrawArrays(GL_POINTS, 0,1);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void gkDrawLine(float sx, float sy, float ex, float ey){
	float v[] = {sx, sy, ex, ey};
	if(gkCheckLineProperties()){
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, v);
		glDrawArrays(GL_LINES, 0, 2);
		glDisableClientState(GL_VERTEX_ARRAY);
	}
}

void gkDrawRect(float x, float y, float w, float h){
	float v[] = {x,y,
				 x + w, y,
				 x + w, y + h,
				 x, y + h};
	gkColor fillColor = gkGetFilteredColor(gkFillColor);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2,GL_FLOAT, 0, v);
	glColor4f(fillColor.r, fillColor.g, fillColor.b, fillColor.a);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	if(gkCheckLineProperties()){
		glDrawArrays(GL_LINE_LOOP, 0, 4);
	}
	glDisableClientState(GL_VERTEX_ARRAY);
}


void gkDrawRoundRect(float x, float y, float w, float h, float a, float b){
	gkPoint roundRect[74], *current = roundRect + 1;
	float sina, cosa, angle = 0, step = M_PI_OVER_180*5;
	int i;
	gkColor fillColor = gkGetFilteredColor(gkFillColor);

	roundRect[0].x = x + w/2;
	roundRect[0].y = y + h/2;
	if(w<a*2) a = w/2;
	if(h<b*2) b = h/2;

	for(i = 0; i<18; i++, current++){
		sincosf(angle, &sina, &cosa);
		current->x = (x + w - a) + a*cosa;
		current->y = (y + b)	 - b*sina;
		angle += step;
	}
	for(i = 0; i<18; i++, current++){
		sincosf(angle, &sina, &cosa);
		current->x = (x + a)	+ a*cosa;
		current->y = (y + b)	- b*sina;
		angle += step;
	}
	for(i = 0; i<18; i++, current++){
		sincosf(angle, &sina, &cosa);
		current->x = (x + a)		+ a*cosa;
		current->y = (y + h - b)	- b*sina;
		angle += step;
	}
	for(i = 0; i<18; i++, current++){
		sincosf(angle, &sina, &cosa);
		current->x = (x + w - a)	+ a*cosa;
		current->y = (y + h - b)	- b*sina;
		angle += step;
	}
	current->x = roundRect[1].x;
	current->y = roundRect[1].y;


	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, roundRect);
	glColor4f(fillColor.r, fillColor.g, fillColor.b, fillColor.a);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 74);
	if(gkCheckLineProperties()){
		glDrawArrays(GL_LINE_LOOP, 1, 73);
	}
	glDisableClientState(GL_VERTEX_ARRAY);
}

void gkDrawCircle(float x, float y, float r){
	static const float sincosTable[] = {	/* pre-calculated 62 (cos, sin) pairs for angles from 0 to (M_PI_OVER_180 * 6) * 62   */
		1.000000f,0.000000f,0.994522f,0.104528f,0.978148f,0.207912f,0.951057f,0.309017f,0.913545f,
		0.406737f,0.866025f,0.500000f,0.809017f,0.587785f,0.743145f,0.669131f,0.669131f,0.743145f,
		0.587785f,0.809017f,0.500000f,0.866025f,0.406737f,0.913545f,0.309017f,0.951057f,0.207912f,
		0.978148f,0.104528f,0.994522f,-0.000000f,1.000000f,-0.104529f,0.994522f,-0.207912f,0.978148f,
		-0.309017f,0.951056f,-0.406737f,0.913545f,-0.500000f,0.866025f,-0.587785f,0.809017f,
		-0.669131f,0.743145f,-0.743145f,0.669131f,-0.809017f,0.587786f,-0.866025f,0.500000f,-0.913545f,
		0.406737f,-0.951056f,0.309018f,-0.978147f,0.207913f,-0.994522f,0.104529f,-1.000000f,
		0.000001f,-0.994522f,-0.104527f,-0.978148f,-0.207910f,-0.951057f,-0.309016f,-0.913546f,
		-0.406735f,-0.866026f,-0.499999f,-0.809018f,-0.587784f,-0.743146f,-0.669129f,-0.669132f,
		-0.743143f,-0.587787f,-0.809016f,-0.500002f,-0.866024f,-0.406739f,-0.913544f,-0.309019f,
		-0.951056f,-0.207914f,-0.978147f,-0.104531f,-0.994522f,-0.000003f,-1.000000f,0.104526f,
		-0.994522f,0.207909f,-0.978148f,0.309014f,-0.951057f,0.406734f,-0.913547f,0.499997f,-0.866027f,
		0.587782f,-0.809019f,0.669128f,-0.743147f,0.743142f,-0.669133f,0.809015f,-0.587788f,
		0.866023f,-0.500003f,0.913544f,-0.406740f,0.951055f,-0.309021f,0.978147f,-0.207916f,
		0.994521f,-0.104533f,1.000000f,-0.000005f
	};
	const float *sincosPtr = sincosTable;
//
	gkPoint circle[62], *current;
	gkColor fillColor = gkGetFilteredColor(gkFillColor);
	circle[0].x = x;
	circle[0].y = y;
	for(current = circle + 1; current-circle<62; current++){
		current->x = *(sincosPtr++)*r + x;
		current->y = *(sincosPtr++)*r + y;
	}
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, circle);
	glColor4f(fillColor.r, fillColor.g, fillColor.b, fillColor.a);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 62);
	if(gkCheckLineProperties()){
		glDrawArrays(GL_LINE_LOOP, 1, 61);
	}
	glDisableClientState(GL_VERTEX_ARRAY);
}

static void drawPolyInternal(gkPoint* path, int length);

void gkDrawPath(gkPoint* path, int length, int polygon){
	gkColor fillColor = gkGetFilteredColor(gkFillColor);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, path);
	if(polygon){
		glColor4f(fillColor.r, fillColor.g, fillColor.b, fillColor.a);
		drawPolyInternal(path, length);
	}
	if(gkCheckLineProperties()){
		glDrawArrays(polygon?GL_LINE_LOOP:GL_LINE_STRIP, 0, length);
	}
	glDisableClientState(GL_VERTEX_ARRAY);
}


#ifdef GK_CONCAVE_POLYGONS
/* 
 *	This one works with many concave polygons as well
 */

#define MAX_INDICES 1000

static void drawPolyInternal(gkPoint* path, int length)
{
	int r;
	int maxCount = 0;
	int minBrk = 1000;
	int ind[MAX_INDICES];

	if (length>256) {
		glDrawArrays(GL_TRIANGLE_FAN, 0, length);
	}

	//	rotate points order to find the best match. 
	//	This algorithm has O(N*N) complexity. May be slow.

	for (r = 0; r<length; r++) {
		//	Simple triangulation 
		//	works for a lot of polygons (even concave)
		int i, i2, p;
		int indices[MAX_INDICES];
		int pathStack[MAX_INDICES], cur, pathTop = length-1;
		float c;
		int center = r, brk = 0, count = 0;
		gkPoint v1, v2;

		for (i = 0; i<pathTop; i++) {
			pathStack[i] = (i + 1 + r)%length;
		}

		for (cur = 0; cur < pathTop-1; cur++) {
			i = pathStack[cur];
			i2 = pathStack[cur+1];
			v1.x = (path[i].x - path[center].x);
			v1.y = (path[i].y - path[center].y);
			v2.x = (path[i2].x - path[center].x);
			v2.y = (path[i2].y - path[center].y);
			c = v1.x*v2.y - v1.y*v2.x;
			if (c<0) {
				p = count*3;
				indices[p] = center;
				indices[p+1] = i;
				indices[p+2] = i2;
				count++;
			} else {
				if (cur>length-1) 
					continue;
				brk++;
				pathStack[pathTop] = center;
				pathTop++;
				center = i;
			}
		}
		if (brk < minBrk) {
			maxCount = count;
			minBrk = brk;
			memcpy(ind, indices, sizeof(int)*count*3);
		}
		if (length>16)
			break;
	}

	glDrawElements(GL_TRIANGLES, maxCount*3, GL_UNSIGNED_INT, ind);
}
#else
static void drawPolyInternal(gkPoint* path, int length)
{
	glDrawArrays(GL_TRIANGLE_FAN, 0, length);
}
#endif
