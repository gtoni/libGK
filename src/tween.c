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

#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#define GK_TWEEN_FREQ	15
#define GK_FLOAT_PI     3.14159265358979323846f

struct gkTweenStruct{
	gkListenerList listeners;
	void* var;
	uint32_t transitionType;
	uint64_t transitionTime;
	uint32_t varType;
	union{
		struct{
			uint8_t start;
			uint8_t end;
		}ub;
		struct{
			int8_t start;
			int8_t end;
		}b;
		struct{
			uint16_t start;
			uint16_t end;
		}us;
		struct{
			int16_t start;
			int16_t end;
		}s;
		struct{
			uint32_t start;
			uint32_t end;
		}ui;
		struct{
			int32_t start;
			int32_t end;
		}i;
		struct{
			float start;
			float end;
		}f;
		struct{
			double start;
			double end;
		}d;
	}value;
	uint64_t startTime;
};
typedef struct gkTweenStruct gkTweenEx;

typedef struct gkTweenRefStruct gkTweenRef;
struct gkTweenRefStruct{
	gkTweenRef* next;
	gkTweenEx* tween;
}*gkTweens, *gkTweensLast;

uint64_t gkLastTweenUpdate = 0;

GK_BOOL gkProcessTween(gkTweenEx* tween);

void gkInitTweens(){
	gkTweens = gkTweensLast = 0;
	gkLastTweenUpdate = gkMilliseconds();
}

void gkDestroyTween(gkTweenRef* tweenRef){
	gkTweenEx* tween = tweenRef->tween;
	gkCleanupListenerList(&tween->listeners);
	free(tween);
	tweenRef->tween = 0;
}

gkTween* CDECL gkAddTween(void* var, uint32_t transitionType, uint64_t transitionTime, uint32_t varType, ...){
	va_list arglist;
	gkTweenEx* tween = (gkTweenEx*)malloc(sizeof(gkTweenEx));
	gkTweenRef* tweenRef = (gkTweenRef*)malloc(sizeof(gkTweenRef));
	gkRemoveTweens(var, sizeof(void*));	/* Remove any previous tweens on the same variable */
	gkInitListenerList(&tween->listeners);
	tween->var = var;
	tween->transitionType = transitionType;
	tween->transitionTime = transitionTime;
	tween->varType = varType;
	va_start(arglist, varType);
	switch(varType){
#if defined(_WIN32)
		case GK_UNSIGNED_BYTE:
			tween->value.ub.start = va_arg(arglist, uint8_t);
			tween->value.ub.end = va_arg(arglist, uint8_t);
		break;
		case GK_BYTE:
			tween->value.b.start = va_arg(arglist, int8_t);
			tween->value.b.end = va_arg(arglist, int8_t);
		break;
		case GK_UNSIGNED_SHORT:
			tween->value.us.start = va_arg(arglist, uint16_t);
			tween->value.us.end = va_arg(arglist, uint16_t);
		break;
		case GK_SHORT:
			tween->value.s.start = va_arg(arglist, int16_t);
			tween->value.s.end = va_arg(arglist, int16_t);
		break;
#else
		case GK_UNSIGNED_BYTE:
			tween->value.ub.start = (uint8_t)va_arg(arglist, int);
			tween->value.ub.end = (uint8_t)va_arg(arglist, int);
		break;
		case GK_BYTE:
			tween->value.b.start = (int8_t)va_arg(arglist, int);
			tween->value.b.end = (int8_t)va_arg(arglist, int);
		break;
		case GK_UNSIGNED_SHORT:
			tween->value.us.start = (uint16_t)va_arg(arglist, int);
			tween->value.us.end = (uint16_t)va_arg(arglist, int);
		break;
		case GK_SHORT:
			tween->value.s.start = (int16_t)va_arg(arglist, int);
			tween->value.s.end = (int16_t)va_arg(arglist, int);
		break;
#endif
		case GK_UNSIGNED_INT:
			tween->value.ui.start = va_arg(arglist, uint32_t);
			tween->value.ui.end = va_arg(arglist, uint32_t);
		break;
		case GK_INT:
			tween->value.i.start = va_arg(arglist, int32_t);
			tween->value.i.end = va_arg(arglist, int32_t);
		break;
		case GK_FLOAT:
			/* In MVC va_arg doesn't work with float */
			tween->value.f.start = (float)va_arg(arglist, double);
			tween->value.f.end = (float)va_arg(arglist, double);
		break;
		case GK_DOUBLE:
			tween->value.d.start = va_arg(arglist, double);
			tween->value.d.end = va_arg(arglist, double);
		break;
	}
	tween->startTime = gkMilliseconds();
	va_end(arglist);
	tweenRef->next = 0;
	tweenRef->tween = tween;
	if(gkTweens){
		gkTweensLast->next = tweenRef;
		gkTweensLast = tweenRef;
	}else{
		gkTweens = gkTweensLast = tweenRef;
	}
	return (gkTween*)tween;
}

void gkRemoveTweens(void* memOffset, size_t size){
	gkTweenRef* ref = gkTweens;
	while(ref){
		if(ref->tween){
			if(ref->tween->var >= memOffset && ref->tween->var <= (void*)((uint8_t*)memOffset + size)){
				gkDestroyTween(ref);
			}
		}
		ref = ref->next;
	}
}

void gkUpdateTweens(){
	gkTweenRef* ref = gkTweens, *prev = 0, *p;
	if(gkMilliseconds() - gkLastTweenUpdate < GK_TWEEN_FREQ) return;
	gkLastTweenUpdate = gkMilliseconds();
	while(ref){
		if(ref->tween == 0){
			if(prev){
				prev->next = ref->next;
			}else{
				gkTweens = ref->next;
			}
			ref = (p = ref)->next;
			free(p);
		}else{
			gkTweenEvent evt;
			GK_BOOL finished = gkProcessTween(ref->tween);
			evt.type = GK_ON_TWEEN_UPDATE;
			evt.currentTarget = evt.target = ref->tween;
			evt.var = ref->tween->var;
			gkDispatch(ref->tween, &evt);
			if(finished){
				evt.type = GK_ON_TWEEN_FINISHED;
				gkDispatch(ref->tween, &evt);
				gkDestroyTween(ref);
			}
			prev = ref;
			ref = ref->next;
		}
	}
}

void gkCleanupTweens(){
	gkTweenRef	*ref = gkTweens, *p;
	while(ref){
		if(ref->tween)	gkDestroyTween(ref);
		ref = (p = ref)->next;
		free(p);
	}
	gkTweensLast = gkTweens = 0;
}


float gkInterpolate(float start, float end, float time, int type){
	switch(type){
		case GK_TWEEN_LINEAR:
			return start + (end - start)*time;
		case GK_TWEEN_EASE_IN_SINE:
			return start + (end - start)*(sinf(time*GK_FLOAT_PI/2.0f - GK_FLOAT_PI/2.0f) + 1.0f);
		case GK_TWEEN_EASE_OUT_SINE:
			return start + (end - start)*(sinf(time*GK_FLOAT_PI/2.0f));
		case GK_TWEEN_EASE_INOUT_SINE:
			return start + (end - start)*(sinf((time - 0.5f)*GK_FLOAT_PI) + 1.0f)/2.0f;
		case GK_TWEEN_EASE_OUTIN_SINE:
			return start + (end - start)*(time<0.5f?sinf(time*GK_FLOAT_PI)/2.0f:sinf(time*GK_FLOAT_PI)/-2.0f + 1.0f);
		case GK_TWEEN_EASE_IN_ELASTIC:{
			float density = 6;
			return start + (end - start)*(sinf((time*GK_FLOAT_PI)*density + GK_FLOAT_PI/2.0f)*(time*time) + (powf(time,10.0f)))/2.0f;
		}
		case GK_TWEEN_EASE_OUT_ELASTIC:{
			float density = 6;
			float rtime = 1 - time;
			return start + (end - start)*(1.0f- (sinf((rtime*GK_FLOAT_PI)*density + GK_FLOAT_PI/2.0f)*(rtime*rtime) + (powf(rtime,10)))/2);
		}
		case GK_TWEEN_EASE_IN_BOUNCE:{
			float k;
			float t = (1.0f - time);
			if (t < (1.0f/2.75f)) {
				k = (7.5625f*t*t);
			} else if (t < (2.0f/2.75f)) {
				t-=(1.5f/2.75f);
				k = (7.5625f*t*t + 0.75f);
			} else if (t < (2.5f/2.75f)) {
				t-=(2.25f/2.75f);
				k = (7.5625f*t*t + 0.9375f);
			} else {
				t-=(2.625f/2.75f);
				k = (7.5625f*t*t + 0.984375f);
			}
			return start + (end - start)*(1.0f -k);
		}
		case GK_TWEEN_EASE_OUT_BOUNCE:{
			float k;
			float t = time;
			if (t < (1.0f/2.75f)) {
				k = (7.5625f*t*t);
			} else if (t < (2.0f/2.75f)) {
				t-=(1.5f/2.75f);
				k = (7.5625f*t*t + 0.75f);
			} else if (t < (2.5f/2.75f)) {
				t-=(2.25f/2.75f);
				k = (7.5625f*t*t + 0.9375f);
			} else {
				t-=(2.625f/2.75f);
				k = (7.5625f*t*t + 0.984375f);
			}
			return start + (end - start)*k;
		}
		default:
			return end;
	}
}

GK_BOOL gkProcessTween(gkTweenEx* tween){
	float t = (float)(gkLastTweenUpdate - tween->startTime)/(float)tween->transitionTime;
	if(t > 1.0f) t = 1.0f;
	switch(tween->varType){
		case GK_UNSIGNED_BYTE:
			*((uint8_t*)tween->var) = (uint8_t)gkInterpolate((float)tween->value.ub.start, (float)tween->value.ub.end, t, tween->transitionType);
		break;
		case GK_BYTE:
			*((int8_t*)tween->var) = (int8_t)gkInterpolate((float)tween->value.b.start, (float)tween->value.b.end, t, tween->transitionType);
		break;
		case GK_UNSIGNED_SHORT:
			*((uint16_t*)tween->var) = (uint16_t)gkInterpolate((float)tween->value.us.start, (float)tween->value.us.end, t, tween->transitionType);
		break;
		case GK_SHORT:
			*((int16_t*)tween->var) = (int16_t)gkInterpolate((float)tween->value.s.start, (float)tween->value.s.end, t, tween->transitionType);
		break;
		case GK_UNSIGNED_INT:
			*((uint32_t*)tween->var) = (uint32_t)gkInterpolate((float)tween->value.ui.start, (float)tween->value.ui.end, t, tween->transitionType);
		break;
		case GK_INT:
			*((int32_t*)tween->var) = (int32_t)gkInterpolate((float)tween->value.i.start, (float)tween->value.i.end, t, tween->transitionType);
		break;
		case GK_FLOAT:
			*((float*)tween->var) = (float)gkInterpolate(tween->value.f.start, tween->value.f.end, t, tween->transitionType);
		break;
		case GK_DOUBLE:
			*((double*)tween->var) = (double)gkInterpolate((float)tween->value.d.start, (float)tween->value.d.end, t, tween->transitionType);
		break;
	}
	return t == 1.0f;
}
