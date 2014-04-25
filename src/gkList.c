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

#include <stdlib.h>

#include "gkTypes.h"

gkList* gkCreateList() 
{
	gkList* list = (gkList*)malloc(sizeof(gkList));
	gkInitList(list);
	return list;
}

void gkInitList(gkList* list)
{
	list->first = list->last = 0;
	list->length = 0;
}

void gkClearList(gkList* list, void (*destructor)(void*))
{
	gkListNode* c = list->first, *p;
	while (c) {
		p = c->next;
		if(destructor) 
			destructor(c->data);
		free(c);
		c = p;
	}
	list->first = list->last = 0;
	list->length = 0;
}

void gkDestroyList(gkList* list, void (*destructor)(void*))
{
	gkClearList(list, destructor);
	free(list);
}

void gkListAddMiddle(gkList* list, gkListPosition index, gkListNode* node)
{
	int curIndex = 0;
	gkListNode *p, *r = 0;
	for (p = list->first; p; p = p->next, curIndex++) {

		if (curIndex == index)
			r = p->prev;
		else if (p == list->last)
			r = p;

		if (r) {
			node->prev = r;
			node->next = r->next;
			r->next = node;
			if (node->next == 0)	
				list->last = node;

			break;
		}
	}
}

void gkListAdd(gkList* list, gkListPosition index, void *data)
{
	gkListNode* node = (gkListNode*)malloc(sizeof(gkListNode));
	node->data = data;
	if (list->first == 0 && list->last == 0) {
		node->prev = node->next = 0;
		list->first = list->last = node;
	} else {
		if (index == GK_LIST_BEGIN) {
			node->prev = 0;
			node->next = list->first;
			list->first->prev = node;
			list->first = node;
		} else if(index == GK_LIST_END) {
			node->prev = list->last;
			node->next = 0;
			list->last->next = node;
			list->last = node;
		} else {
			gkListAddMiddle(list, index, node);
		}
	}
	list->length++;
}

gkListNode* gkListGetNode(gkList* list, gkListPosition index)
{
	if (index == GK_LIST_BEGIN) {
		return list->first;
	} else if (index == GK_LIST_END) {
		return list->last;
	} else {
		int c = 0;
		gkListNode *p;
		for (p = list->first; p; p = p->next, c++)
			if (c == index) 
				return p;
	}
	return 0;
}

void* gkListRemove(gkList *list, gkListPosition index)
{
	gkListNode *node = gkListGetNode(list, index);
	void* data = 0;

	if (node) {
		data = node->data;
		gkListRemoveNode(list, node);
	}
	return data;
}

void* gkListGet(gkList* list, gkListPosition index)
{
	gkListNode* node = gkListGetNode(list, index);
	if (node)
		return node->data;
	return 0;
}

gkListNode* gkListRemoveNode(gkList* list, gkListNode* node)
{
	gkListNode *prevNode = node->prev, *nextNode = node->next;

	if (prevNode)
		prevNode->next = nextNode;
	else
		list->first = nextNode;

	if (nextNode)
		nextNode->prev = prevNode;
	else
		list->last = prevNode;

	list->length--;

	free(node);
	return nextNode;
}

gkListNode* ListFind(gkList* list, void* data)
{
	gkListNode *c;
	for (c = list->first; c; c = c->next)
		if (c->data == data) 
			return c;
	return 0;
}

void gkListForEach(gkList* list, void (*iterator)(void*))
{
	gkListNode* c;
	for (c = list->first; c; c = c->next)
		iterator(c->data);
}

static gkListNode* merge(gkListNode* a, gkListNode *b, int (*less)(void*, void*))
{
	gkListNode head, *c = &head;
	while (a && b) {
		if (less(a->data, b->data)) {
			c->next = a; 
			c = a; 
			a = a->next;
		} else {
			c->next = b; 
			c = b; 
			b = b->next;
		}
	}
	c->next = (a == 0) ? b : a;
	return head.next;
}

static gkListNode* mergeSort(gkListNode* c, int (*less)(void*, void*))
{
	gkListNode *a, *b;

	if (c == 0 || c->next == 0) 
		return c;

	a = c; 
	b = c->next;

	while (b != 0 && b->next != 0) {
		c = c->next;
		b = b->next->next;
	}

	b = c->next; 
	c->next = 0;
	return merge(mergeSort(a, less), mergeSort(b, less), less);
}

void gkListSort(gkList* list, int (*less)(void*, void*))
{
	gkListNode* f;
	list->first = f = mergeSort(list->first, less);
	while(f->next) 
		f = f->next;
	list->last = f;
}