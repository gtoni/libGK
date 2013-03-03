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


#ifdef GK_WIN
#include <windows.h>
#endif

#include <stdlib.h>
#include <GL/gl.h>

#include "gk.h"

#define FABS(x) x<0?-x:x

typedef struct gkPanelStructEx gkPanelEx;
struct gkPanelStructEx{
	gkListenerList listeners;
	float x,y,width,height;
	uint16_t autosizeMask;
	gkMatrix transform;
	gkColor colorFilter;
	GK_BOOL mouseEnabled, mouseChildren, keyboardEnabled, keyboardChildren;
	GK_BOOL visible;
	GK_BOOL mouseOver;
	float mouseX, mouseY;
	void* data;
	gkPanelResizeFunc resizeFunc;
	gkPanelUpdateFunc updateFunc;
	gkPanelDrawFunc drawFunc;
	gkPanel* parent;
	int16_t numChildren;
	//exteneded information goes here
	struct{
		gkPanelEx *first, *last;
	}children;
	gkPanelEx* next;
	gkPanelEx* nextChild;
	GK_BOOL inIteration;
	GK_BOOL mustDestroy;
	GK_BOOL viewport;
	float oldWidth, oldHeight;
};

gkPanelEx* gkFocusPanel = 0;

gkPanel* gkCreatePanel(){
	gkPanelEx* panel = (gkPanelEx*)malloc(sizeof(gkPanelEx));
	gkInitListenerList(&panel->listeners);
	panel->x = 0;
	panel->y = 0;
	panel->width = 0;
	panel->height = 0;
	panel->autosizeMask = GK_START_LEFT|GK_START_RIGHT|GK_START_TOP|GK_START_BOTTOM;
	panel->transform = gkMatrixCreateIdentity();
	panel->colorFilter = GK_COLOR(1,1,1,1);
	panel->data = 0;
	panel->resizeFunc = gkResizePanel;
	panel->updateFunc = 0;
	panel->drawFunc = 0;
	panel->parent = 0;
	panel->numChildren = 0;
	panel->children.first = panel->children.last = 0;
	panel->next = 0;
	panel->nextChild = 0;
	panel->inIteration = GK_FALSE;
	panel->mustDestroy = GK_FALSE;
	panel->oldWidth = panel->oldHeight = 0;
	panel->mouseOver = GK_FALSE;
	panel->mouseX = panel->mouseY = 0;
	panel->mouseEnabled = panel->mouseChildren = panel->keyboardEnabled = panel->keyboardChildren = GK_TRUE;
	panel->visible = GK_TRUE;
	panel->viewport = GK_FALSE;
	return (gkPanel*)panel;
}
gkPanel* gkCreateViewportPanel(){
	gkPanelEx* panel = (gkPanelEx*)gkCreatePanel();
	panel->viewport = GK_TRUE;
	return (gkPanel*)panel;
}
void gkDestroyPanel(gkPanel* ptr){
	gkPanelEx* panel = (gkPanelEx*)ptr, *p;
	if(panel->inIteration){
		panel->mustDestroy = GK_TRUE;
	}else{
		if(panel->parent) gkRemoveChild((gkPanel*)panel);
		if(gkFocusPanel == panel) gkSetFocus(0);
		gkCleanupListenerList(&panel->listeners);
		for(p = panel->children.first; p; p = panel->nextChild){
			panel->nextChild = p->next;
			p->parent = 0;
			p->next = 0;
		}
		free(panel);
	}
}

float resizeEdge(float edge, uint8_t mask, float oldStart, float oldEnd, float start, float end){
	if(mask == 1){
		return edge;
	}else if(mask == 2){
		float offset = (oldEnd - oldStart) - edge;
		return (end - start) - offset;
	}else if(mask == 3){
		float k = edge/(oldEnd - oldStart);
		return (end - start)*k;
	}
	return edge;
}

void gkResizePanel(gkPanel* panelPtr, float width, float height){
	gkEvent evt;
	gkPanelEx *panel = (gkPanelEx*)panelPtr, *p;
	float left, right, top, bottom, nleft, nright, ntop, nbottom;
	if(panel->oldWidth == 0) panel->oldWidth = panel->width;
	if(panel->oldHeight == 0) panel->oldHeight = panel->height;
	panel->width = width;
	panel->height = height;
	left = panel->x; right = panel->x + panel->width;	top = panel->y; 	bottom = panel->y + panel->height;
	panel->inIteration = GK_TRUE;
	for(p = panel->children.first; p; p = panel->nextChild){
		panel->nextChild = p->next;
		nleft = resizeEdge(p->x,				 p->autosizeMask&3,		left, panel->x + panel->oldWidth ,left, right);
		nright = resizeEdge(p->x + p->width,	(p->autosizeMask>>2)&3,	left, panel->x + panel->oldWidth, left, right);
		ntop = resizeEdge(p->y,					(p->autosizeMask>>4)&3,	top,  panel->y + panel->oldHeight, top, bottom);
		nbottom = resizeEdge(p->y + p->height,	(p->autosizeMask>>6)&3, top,  panel->y + panel->oldHeight, top, bottom);
		p->x = nleft;
		p->y = ntop;
		if(p->resizeFunc){
			p->resizeFunc((gkPanel*)p, nright - nleft, nbottom - ntop);
		}
	}
	panel->oldWidth = panel->width;
	panel->oldHeight = panel->height;
	evt.type = GK_ON_PANEL_RESIZED;
	evt.target = evt.currentTarget = panel;
	gkDispatch(panel, &evt);
	panel->inIteration = GK_FALSE;
	if(panel->mustDestroy) gkDestroyPanel((gkPanel*)panel);
}

void gkAddChild(gkPanel* parentPtr, gkPanel* childPtr){
	gkPanelEx* parent = (gkPanelEx*)parentPtr, *child = (gkPanelEx*)childPtr;
	gkEvent evt;
	if(child->parent) gkRemoveChild(childPtr);
	child->parent = parentPtr;
	child->next = 0;
	if(parent->children.last){
		parent->children.last->next = child;
		parent->children.last = child;
	}else{
		parent->children.first = parent->children.last = child;
	}
	parent->numChildren++;

	evt.type = GK_ON_PANEL_ADDED;
	evt.target = evt.currentTarget = child;
	gkDispatch(child, &evt);
}

void gkAddChildAt(gkPanel* parentPtr, gkPanel* childPtr, int index){
	gkPanelEx* parent = (gkPanelEx*)parentPtr, *child = (gkPanelEx*)childPtr, *p;
	gkEvent evt;
	int i = 1;
	if(child->parent) gkRemoveChild(childPtr);
	child->parent = parentPtr;
	child->next = 0;
	if(parent->children.last){
		if(index>0){
			for(p = parent->children.first; p && i<index; p = p->next) i++;
			child->next = p->next;
			p->next = child;
		}else{
			child->next = parent->children.first;
			parent->children.first = child;
		}
	}else{
		parent->children.first = parent->children.last = child;
	}
	parent->numChildren++;

	evt.type = GK_ON_PANEL_ADDED;
	evt.target = evt.currentTarget = child;
	gkDispatch(child, &evt);
}

void gkRemoveChild(gkPanel* childPtr){
	gkPanelEx *child = (gkPanelEx*)childPtr, *panel = (gkPanelEx*)child->parent;
	if(panel){
		gkEvent evt;
		gkPanelEx* p, *prev = 0;
		for(p = panel->children.first; p && p != child; p = p->next) prev = p;
		if(p != 0){
			if(p == panel->children.last) panel->children.last = prev;
			if(prev){
				prev->next = p->next;
			}else{
				panel->children.first = p->next;
			}
			if(p == panel->nextChild) panel->nextChild = p->next;
			panel->numChildren--;
		}
		child->parent = 0;
		child->next = 0;
		if(gkFocusPanel == child) gkSetFocus(0);
		evt.type = GK_ON_PANEL_REMOVED;
		evt.target = evt.currentTarget = child;
		gkDispatch(child, &evt);
	}
}

void gkRemoveChildAt(gkPanel* parent, int childIndex){
	gkRemoveChild(gkGetChildAt(parent, childIndex));
}

int gkGetChildIndex(gkPanel* childPtr){
	gkPanelEx *child = (gkPanelEx*)childPtr, *parent = (gkPanelEx*)child->parent, *p;
	int index = 0;
	if(parent){
		for(p = parent->children.first; p && p != child; p = p->next)		index++;
	}
	return index;
}

gkPanel* gkGetChildAt(gkPanel* parentPtr, int childIndex){
	gkPanelEx *parent = (gkPanelEx*)parentPtr, *p;
	int index = 0;
	if(parent){
		for(p = parent->children.first; p && index != childIndex; p = p->next)		index++;
	}
	return (gkPanel*)p;
}

void gkProcessUpdatePanel(gkPanelEx* panel){
	gkPanelEx* p;
	panel->inIteration = GK_TRUE;
	if(panel->updateFunc){
		panel->updateFunc((gkPanel*)panel);
	}
	for(p = panel->children.first; p; p = panel->nextChild){
		panel->nextChild = p->next;
		gkProcessUpdatePanel(p);
	}
	panel->inIteration = GK_FALSE;
	if(panel->mustDestroy) gkDestroyPanel((gkPanel*)panel);
}

void gkProcessDrawPanel(gkPanelEx* panel){
	gkPanelEx* p;
	gkMatrix t = gkMatrixCreateTranslation(panel->x, panel->y);
	if(!panel->visible) return;	/* Don't draw invisible panels */
	panel->inIteration = GK_TRUE;
	gkPushColorFilter(panel->colorFilter.r, panel->colorFilter.g, panel->colorFilter.b, panel->colorFilter.a);
	gkPushTransform(&t);
	gkPushTransform(&panel->transform);
	if(panel->viewport){
		gkMatrix m = gkLocalToGlobal((gkPanel*)panel);
		gkPoint topLeft = gkTransformPoint(GK_POINT(0,0), &m);
		gkPoint bottomRight = gkTransformPoint(GK_POINT(panel->width, panel->height), &m);
		float h = FABS(bottomRight.y - topLeft.y);
		glPushMatrix();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glPushAttrib(GL_ALL_ATTRIB_BITS);
		glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
		glViewport((GLint)topLeft.x, (GLint)gkGetScreenSize().height - topLeft.y - h, FABS(bottomRight.x - topLeft.x), h);
		if(panel->drawFunc){
			panel->drawFunc((gkPanel*)panel);
		}
		glPopClientAttrib();
		glPopAttrib();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}else{
		if(panel->drawFunc){
			panel->drawFunc((gkPanel*)panel);
		}
		for(p = panel->children.first; p; p = panel->nextChild){
			panel->nextChild = p->next;
			gkProcessDrawPanel(p);
		}
	}
	gkPopTransform();
	gkPopTransform();
	gkPopColorFilter();
	panel->inIteration = GK_FALSE;
	if(panel->mustDestroy) gkDestroyPanel((gkPanel*)panel);
}

gkMatrix gkGlobalToLocal(gkPanel* panel){
	gkMatrix m = gkLocalToGlobal(panel);
	gkMatrixInverse(&m);
	return m;
}

gkMatrix gkLocalToGlobal(gkPanel* panel){
	gkMatrix m = gkMatrixCreateIdentity(), t;
	gkPanel* p = panel;
	do{
		t = gkMatrixCreateTranslation(p->x, p->y);
		gkMatrixMultPtr(&m, &p->transform);
		gkMatrixMultPtr(&m, &t);
		p = p->parent;
	}while(p);
	return m;
}

/*** Panel input handling ***/

gkPanelEx* gkMouseTarget = 0;
gkPoint gkMousePosition;

#define BYTE_OFFSET(obj, prop) ((char*)&obj->prop - (char*)obj)

gkPanelEx* gkGetMouseTarget(gkPanelEx* panel, gkPoint pos, size_t enabledOffset, size_t enabledChildrenOffset){
	gkPanelEx** children, *p;
	int i = panel->numChildren;
	GK_BOOL *enabled = (GK_BOOL*)((char*)panel + enabledOffset);
	GK_BOOL *enabledChildren = (GK_BOOL*)((char*)panel + enabledChildrenOffset);
	if(!panel->visible) return 0;	/* ignore invisible panels */
	if(*enabledChildren){
		children = (gkPanelEx**)calloc(panel->numChildren, sizeof(gkPanelEx*));
		for(p = panel->children.first; p && i>0; p = p->next) children[--i] = p;	/* also reverses the order so in the next loop it starts from the last and goes to first */
		for(i = 0; i<panel->numChildren; i++){
			gkMatrix t = children[i]->transform;
			gkPoint tmpPos = pos;
			p = children[i];
			gkMatrixInverse(&t);
			tmpPos.x -= p->x;
			tmpPos.y -= p->y;
			tmpPos = gkTransformPoint(tmpPos, &t);
			if((p = gkGetMouseTarget(p, tmpPos, enabledOffset, enabledChildrenOffset)) != 0){
				panel->mouseX = pos.x;
				panel->mouseY = pos.y;
				free(children);
				return p;
			}
		}
		free(children);
	}
	if(pos.x>=0 && pos.y>=0 && pos.x<=panel->width && pos.y<=panel->height && *enabled){
		panel->mouseX = pos.x;
		panel->mouseY = pos.y;
		return panel;
	}else return 0;
}

void gkCheckFocusedPanel(){
	if(gkFocusPanel){
		gkPanel* p = (gkPanel*)gkFocusPanel;
		GK_BOOL focusable = p->keyboardEnabled && p->visible;
		if(focusable){
			p = p->parent;
			while(p && focusable){
				if(!(p->keyboardChildren && p->visible)) focusable = GK_FALSE;
				p = p->parent;
			}
		}
		if(!focusable) gkSetFocus(0);
	}
}

void gkUpdateMouseTarget(gkPanel* panelPtr){
	gkPanelEx* mainPanel = (gkPanelEx*)panelPtr, *oldMouseTarget, *p, *lastCommon;
	gkPoint pos = gkMousePosition;
	if(!mainPanel){
		gkMouseTarget = 0;
		return;
	}
	oldMouseTarget = gkMouseTarget;
	gkMouseTarget = gkGetMouseTarget(mainPanel, pos, BYTE_OFFSET(mainPanel, mouseEnabled), BYTE_OFFSET(mainPanel, mouseChildren));
	if(oldMouseTarget != gkMouseTarget){
		gkMouseEvent enter, leave;
		enter.type = GK_ON_MOUSE_ENTER;
		enter.target = gkMouseTarget;
		enter.position = gkMousePosition;
		leave.type = GK_ON_MOUSE_LEAVE;
		leave.target = oldMouseTarget;
		leave.position = gkMousePosition;

		for(p = oldMouseTarget; p; p = (gkPanelEx*)p->parent) p->mouseOver = GK_FALSE;	/* reset mouse over */
		for(p = gkMouseTarget; p; p = (gkPanelEx*)p->parent) p->mouseOver = GK_TRUE;	/* set mouse over to all panels under the mouse */
		for(p = oldMouseTarget; p && !p->mouseOver; p = (gkPanelEx*)p->parent){
			leave.currentTarget = p;
			gkDispatch(p, &leave);	/* dispatch GK_ON_MOUSE_LEAVE */
		}
		lastCommon = p;
		for(p = gkMouseTarget; p != 0 && p != lastCommon; p = (gkPanelEx*)p->parent){
			enter.currentTarget = p;
			gkDispatch(p, &enter);	/* dispatch GK_ON_MOUSE_ENTER */
		}
	}
}

void gkProcessMouseEvent(gkMouseEvent* mouseEvent){
	gkPanel* current, *newFocusTarget = 0, *curFocusTarget = (gkPanel*)gkFocusPanel;
	gkMousePosition = mouseEvent->position;
	if(mouseEvent->type == GK_ON_MOUSE_DOWN){
		gkPanel* mainPanel = gkGetMainPanel();
		if(mainPanel){
			newFocusTarget = (gkPanel*)gkGetMouseTarget((gkPanelEx*)mainPanel, gkMousePosition, BYTE_OFFSET(mainPanel, keyboardEnabled), BYTE_OFFSET(mainPanel, keyboardChildren));
		}
	}
	if(gkMouseTarget){
		mouseEvent->target = current = (gkPanel*)gkMouseTarget;
		do{
			gkMatrix m = gkGlobalToLocal(current);
			mouseEvent->position = gkTransformPoint(gkMousePosition, &m);
			mouseEvent->currentTarget = current;
		}while(gkDispatch(current, mouseEvent) && (current = current->parent));
	}
	if(newFocusTarget && curFocusTarget == (gkPanel*)gkFocusPanel){
		gkSetFocus(newFocusTarget);
	}
}


void gkSetFocus(gkPanel* panel){
	static gkPanelEx *tmpNextFocused = 0;
	gkPanelEx* tmp = gkFocusPanel;
	if(panel == (gkPanel*)gkFocusPanel) return;
	tmpNextFocused = (gkPanelEx*)panel;
	gkFocusPanel = 0;
	if(tmp){
		gkEvent evt;
		evt.type = GK_ON_PANEL_FOCUS_OUT;
		evt.target = evt.currentTarget = tmp;
		gkDispatch(tmp, &evt);
	}
	if(gkFocusPanel != tmpNextFocused){
		if(tmpNextFocused->keyboardEnabled && tmpNextFocused->visible){
			gkFocusPanel = tmpNextFocused;
			if(gkFocusPanel){
				gkEvent evt;
				evt.type = GK_ON_PANEL_FOCUS_IN;
				evt.target = evt.currentTarget = gkFocusPanel;
				gkDispatch(gkFocusPanel, &evt);
			}
		}
	}
}

gkPanel* gkGetFocus(){
	return (gkPanel*)gkFocusPanel;
}

void gkProcessKeyboardEvent(gkKeyboardEvent* keyboardEvent){
	if(!gkFocusPanel) return;
	keyboardEvent->target = keyboardEvent->currentTarget = (gkPanel*)gkFocusPanel;
	gkDispatch(gkFocusPanel, keyboardEvent);
}
