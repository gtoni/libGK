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

uint64_t gkAppStartTime = 0;

#ifdef GK_PLATFORM_WIN
#include <windows.h>

uint64_t gkMilliseconds(){
	LARGE_INTEGER count, freq;
	if(!QueryPerformanceCounter(&count)){
		return GetTickCount() - gkAppStartTime;
	}
	QueryPerformanceFrequency(&freq);
	return (count.QuadPart/(freq.QuadPart/1000)) - gkAppStartTime;
}

#elif defined(GK_PLATFORM_LINUX)
#include <unistd.h>
#include <time.h>

uint64_t gkMilliseconds(){
    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return (tv.tv_sec*1000 + tv.tv_nsec/1000000) - gkAppStartTime;
}

#elif defined(GK_PLATFORM_TIZEN)
	// implemented in gkPlatformTizen.cpp
#elif defined(GK_PLATFORM_ANDROID)
#include <unistd.h>
#include <time.h>

uint64_t gkMilliseconds(){
    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return (tv.tv_sec*1000 + tv.tv_nsec/1000000) - gkAppStartTime;
}
#else

uint64_t gkMilliseconds(){
#ifdef GK_SHOW_PLATFORM_ERRORS
#error gkMilliseconds not implemented
#else
	return 0;
#endif
}

#endif

typedef struct gkTimerRefStruct gkTimerRef;
struct gkTimerRefStruct{
	gkTimerRef *next;
	gkTimer* timer;
};

gkTimerRef *gkTimers, *gkTimersLast;

struct gkTimerStructEx{
	gkDispatcher dispatcher;
	uint64_t delay;
	uint32_t repeats;
	uint64_t interval;
	GK_BOOL running;
	GK_BOOL destroyOnComplete;
	/* hidden fields */
	gkTimerRef* ref;
	uint64_t startTime;
	uint32_t repeatsElapsed;
};
typedef struct gkTimerStructEx gkTimerEx;

gkTimer* gkCreateTimer(){
	gkTimerEx* timer = (gkTimerEx*)malloc(sizeof(gkTimerEx));
	gkTimerRef* ref = (gkTimerRef*)malloc(sizeof(gkTimerRef));
	gkInitDispatcher(&timer->dispatcher);
	timer->delay = 0;
	timer->repeats = 0;
	timer->interval = 0;
	timer->running = GK_FALSE;
	timer->destroyOnComplete = GK_FALSE;
	timer->repeatsElapsed = 0;
	ref->next = 0;
	ref->timer = (gkTimer*)timer;
	timer->ref = ref;
	if(gkTimers == 0){
		gkTimersLast = gkTimers = ref;
	}else{
		gkTimersLast->next = ref;
		gkTimersLast = ref;
	}
	return (gkTimer*)timer;
}

void gkStartTimer(gkTimer* timer, GK_BOOL destroyOnComplete){
	gkTimerEx* t = (gkTimerEx*)timer;
	t->startTime = gkMilliseconds();
	t->repeatsElapsed = 0;
	t->destroyOnComplete = destroyOnComplete;
	t->running = GK_TRUE;
}

void gkStopTimer(gkTimer* timer){
	((gkTimerEx*)timer)->running = GK_FALSE;
}

void gkDestroyTimer(gkTimer* timer){
	((gkTimerEx*)timer)->ref->timer = 0;
	gkCleanupDispatcher(&timer->dispatcher);
	free(timer);
}

void gkUpdateTimers(){
	gkTimerRef* ref = gkTimers, *prev = 0, *p;
	gkTimerEx* timer;
	uint64_t currentTime = gkMilliseconds();
	while(ref){
		if(ref->timer == 0){
			if(ref == gkTimersLast) gkTimersLast = prev;
			if(prev){
				prev->next = ref->next;
			}else{
				gkTimers = ref->next;
			}
			ref = (p = ref)->next;
			free(p);
		}else{
			timer = (gkTimerEx*)ref->timer;
			if(timer->running){
				if(	(timer->repeatsElapsed == 0 && (currentTime - timer->startTime) >= timer->delay) ||
					(timer->repeatsElapsed > 0 && (currentTime - timer->startTime) >= timer->interval) ){
					gkEvent evt;
					evt.type = GK_ON_TIMER;
					evt.currentTarget = evt.target = timer;
					gkDispatch(timer, &evt);
					if(ref->timer){
						timer->startTime = currentTime;
						timer->repeatsElapsed++;
						if(timer->repeats > 0 && timer->repeatsElapsed == timer->repeats){
							gkStopTimer((gkTimer*)timer);
							evt.type = GK_ON_TIMER_COMPLETE;
							evt.currentTarget = evt.target = timer;
							gkDispatch(timer, &evt);
							if(ref->timer && timer->destroyOnComplete){
								gkDestroyTimer((gkTimer*)timer);
							}
						}
					}
				}
			}
			prev = ref;
			ref = ref->next;
		}
	}
}

void gkInitTimers(){
	gkTimers = 0;
	gkTimersLast = 0;
}

void gkCleanupTimers(){
	gkTimerRef	*ref = gkTimers, *p;
	while(ref){
		if(ref->timer)	gkDestroyTimer(ref->timer);
		ref = (p = ref)->next;
		free(p);
	}
	gkTimersLast = gkTimers = 0;
}
