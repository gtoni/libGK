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

#ifndef _GK_EVENT_H_
#define _GK_EVENT_H_

#include <gkTypes.h>

/************************************
	Event model

	Types and functions used for dispatching and handling events.
	Events can be dispatched for various reasons, such as mouse clicks or key pressed. Objects that dispatch events
	are usually called event dispatchers. Listener functions are called when the event they listen for is dispatched,
	also they are given a data structure which describes the event (such as which mouse button was pressed).
*/

#define GK_EVENT(T)	typedef struct T T;\
			struct T{\
				uint16_t type;\
				void* target;\
				void* currentTarget;
#define GK_EVENT_END()	};

GK_EVENT(gkEvent)
GK_EVENT_END()

typedef struct gkDispatcher
{
	struct gkEventListener* listeners;
} gkDispatcher;

typedef void* gkDispatcherHandle;
typedef void* gkEventHandle;

typedef GK_BOOL (*gkEventListenerFunc)(gkEvent* eventData, void* param);

void	gkInitDispatcher(gkDispatcher* listeners);
void	gkCleanupDispatcher(gkDispatcher* listeners);
void	gkAddListener(gkDispatcherHandle listeners, int type, short priority, gkEventListenerFunc listener, void* param);
void	gkRemoveListener(gkDispatcherHandle listeners, int type, gkEventListenerFunc listener, void* param);
GK_BOOL	gkHasListeners(gkDispatcherHandle listeners, int type);
GK_BOOL	gkDispatch(gkDispatcherHandle listeners, gkEventHandle eventData);

#endif