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

#include <stdlib.h>
#include "gk.h"

typedef struct gkEventListener listenerNode;
struct gkEventListener
{
    int type;
    short priority;
    gkEventListenerFunc func;
    void* param;
    struct gkEventListener* next;
};


void gkInitListenerList(gkListenerList* listeners)
{
    listeners->ptr = 0;
}

void gkCleanupListenerList(gkListenerList* listeners)
{
    if(listeners->ptr)
    {
        listenerNode* node = listeners->ptr, *p;
        while(node)
        {
            node = (p = node)->next;
            free(p);
        }
    }
}

void gkAddListener(gkListenerListPointer pListeners, int type, short priority, gkEventListenerFunc func, void* param)
{
    gkListenerList* listeners = (gkListenerList*) pListeners;
    listenerNode* listener = (listenerNode*)malloc(sizeof(listenerNode));
    listener->type = type;
    listener->priority = priority;
    listener->func = func;
    listener->param = param;
    listener->next = 0;
    gkRemoveListener(listeners, type, func, param);
    if(listeners->ptr)
    {
        listenerNode* node, *prev = 0;
        for(node = listeners->ptr; node; node = (prev = node)->next)
        {
            if(listener->priority > node->priority)
            {
                if(prev)
                {
                    prev->next = listener;
                    listener->next = node;
                }
                else
                {
                    listeners->ptr = listener;
                    listener->next = node;
                }
                break;
            }
            if(node->next == 0)
            {
                node->next = listener;
                break;
            }
        }
    }
    else
    {
        listeners->ptr = listener;
    }
}

void gkRemoveListener(gkListenerListPointer pListeners, int type, gkEventListenerFunc func, void* param)
{
    gkListenerList* listeners = (gkListenerList*) pListeners;
    if(listeners->ptr)
    {
        listenerNode* node, *prev = 0;
        for(node = listeners->ptr; node; node = (prev = node)->next)
        {
            if(node->type == type && node->func == func && node->param == param)
            {
                if(prev)
                {
                    prev->next = node->next;
                    free(node);
                }
                else
                {
                    listeners->ptr = node->next;
                    free(node);
                }
                break;
            }
        }
    }
}

int gkHasListeners(gkListenerListPointer pListeners, int type)
{
    gkListenerList* listeners = (gkListenerList*) pListeners;
    if(listeners->ptr)
    {
        listenerNode* listener;
        for(listener = (listenerNode*)listeners->ptr; listener; listener = listener->next)
        {
            if(listener->type == type) return 1;
        }
    }
    return 0;
}

GK_BOOL gkDispatch(gkListenerListPointer pListeners, gkEventPointer eventPtr)
{
    gkListenerList* listeners = (gkListenerList*) pListeners;
    GK_BOOL result = GK_TRUE;
    gkEvent* eventData = (gkEvent*)eventPtr;
    if(listeners->ptr)
    {
        struct callNode
        {
            gkEventListenerFunc func;
            void* param;
            struct callNode* next;
        };
        struct callNode* callList = 0, *pnode = 0, *cnode;
        listenerNode* listener;
        for(listener = (listenerNode*)listeners->ptr; listener; listener = listener->next)
        {
            if(listener->type == eventData->type)
            {
                cnode = (struct callNode*)malloc(sizeof(struct callNode));
                cnode->func = listener->func;
                cnode->param = listener->param;
                cnode->next = 0;
                if(callList == 0)
                {
                    callList = cnode;
                }
                else
                {
                    pnode->next = cnode;
                }
                pnode = cnode;
            }
        }
        for(cnode = callList; cnode; cnode = cnode->next)
        {
            if(!cnode->func(eventData, cnode->param))
            {
                result = GK_FALSE;
                break;
            }
        }
        while(callList)
        {
            callList = (cnode = callList)->next;
            free(cnode);
        }
    }
    return result;
}
