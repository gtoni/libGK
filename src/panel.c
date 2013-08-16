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

#define GK_INTERNAL

#include "gk.h"

#ifdef GK_WIN
#include <windows.h>
#endif

#include <stdlib.h>
#include <GL/gl.h>

void gkProcessChildrenLayout(gkPanel* panel);
void gkProcessLayoutPanel(gkPanel* panel, gkRect* clientRect);

#define FABS(x) x<0?-x:x

gkPanel* gkMouseTarget = 0;
gkPanel* gkFocusPanel = 0;

gkPanel* gkCreatePanel()
{
    return gkCreatePanelEx(sizeof(gkPanel));
}

gkPanel* gkCreatePanelEx(size_t panelSize){
	gkPanel* panel = (gkPanel*)malloc(panelSize);
	gkInitListenerList(&panel->listeners);
	panel->x = 0;
	panel->y = 0;
	panel->width = 0;
	panel->height = 0;
	panel->transform = gkMatrixCreateIdentity();
	panel->anchorX = 0;
	panel->anchorY = 0;
	panel->colorFilter = GK_COLOR(1,1,1,1);
	panel->data = 0;
	panel->layoutFunc = 0;
	panel->updateFunc = 0;
	panel->drawFunc = 0;
	panel->parent = 0;
	panel->numChildren = 0;
	panel->mouseOver = GK_FALSE;
	panel->mouseX = panel->mouseY = 0;
	panel->mouseEnabled = panel->mouseChildren = panel->keyboardEnabled = panel->keyboardChildren = GK_TRUE;
	panel->visible = GK_TRUE;

	panel->mChildren.first = panel->mChildren.last = 0;
	panel->mNext = 0;
	panel->mNextChild = 0;
	panel->mGuardDestroy = GK_FALSE;
	panel->mMustDestroy = GK_FALSE;
	panel->mViewport = GK_FALSE;
	return panel;
}

gkPanel* gkCreateViewportPanel()
{
    return gkCreateViewportPanelEx(sizeof(gkPanel));
}

gkPanel* gkCreateViewportPanelEx(size_t panelSize){
	gkPanel* panel = gkCreatePanelEx(panelSize);
	panel->mViewport = GK_TRUE;
	return panel;
}

static void gkGuardDestroy(gkPanel* panel)
{
    panel->mGuardDestroy = GK_TRUE;
}
static void gkUnguardDestroy(gkPanel* panel)
{
    panel->mGuardDestroy = GK_FALSE;
    if(panel->mMustDestroy)
        gkDestroyPanel(panel);
}

void gkDestroyPanel(gkPanel* panel){
	gkPanel *p;
	if(panel->mGuardDestroy){
		panel->mMustDestroy = GK_TRUE;
	}else{
		if(panel->parent) gkRemoveChild(panel);
		if(gkFocusPanel == panel) gkSetFocus(0);
		gkCleanupListenerList(&panel->listeners);
		for(p = panel->mChildren.first; p; p = panel->mNextChild){
			panel->mNextChild = p->mNext;
			p->parent = 0;
			p->mNext = 0;
		}
		if(gkMouseTarget == panel)
        {
            gkMouseTarget = 0;
        }
		free(panel);
	}
}

void gkAddChild(gkPanel* parent, gkPanel* child){
    gkAddChildAt(parent, child, parent->numChildren);
}

void gkAddChildAt(gkPanel* parent, gkPanel* child, int index){
    gkPanel* p;
	gkEvent evt;
	int i = 1;
	if(child->parent) gkRemoveChild(child);

	gkProcessChildrenLayout(parent);

	child->parent = parent;
	child->mNext = 0;
	if(parent->mChildren.last){
        if(index >= parent->numChildren)
        {
            parent->mChildren.last->mNext = child;
            parent->mChildren.last = child;
        }else if(index>0){
			for(p = parent->mChildren.first; p && i<index; p = p->mNext) i++;
			child->mNext = p->mNext;
			p->mNext = child;
		}else{
			child->mNext = parent->mChildren.first;
			parent->mChildren.first = child;
		}
	}else{
		parent->mChildren.first = parent->mChildren.last = child;
	}
	parent->numChildren++;

	evt.type = GK_ON_PANEL_ADDED;
	evt.target = evt.currentTarget = child;
	gkDispatch(child, &evt);
}

void gkRemoveChild(gkPanel* child){
	gkPanel *panel = child->parent;
	if(panel){
		gkEvent evt;
		gkPanel* p, *prev = 0;
		for(p = panel->mChildren.first; p && p != child; p = p->mNext) prev = p;
		if(p != 0){
			if(p == panel->mChildren.last) panel->mChildren.last = prev;
			if(prev){
				prev->mNext = p->mNext;
			}else{
				panel->mChildren.first = p->mNext;
			}
			if(p == panel->mNextChild) panel->mNextChild = p->mNext;
			panel->numChildren--;
		}
		child->parent = 0;
		child->mNext = 0;
		if(gkFocusPanel == child) gkSetFocus(0);
		evt.type = GK_ON_PANEL_REMOVED;
		evt.target = evt.currentTarget = child;
		gkDispatch(child, &evt);
	}
}

void gkRemoveChildAt(gkPanel* parent, int childIndex){
	gkRemoveChild(gkGetChildAt(parent, childIndex));
}

int gkGetChildIndex(gkPanel* child){
	gkPanel *parent = child->parent, *p;
	int index = 0;
	if(parent){
		for(p = parent->mChildren.first; p && p != child; p = p->mNext)		index++;
	}
	return index;
}

gkPanel* gkGetChildAt(gkPanel* parent, int childIndex){
	gkPanel *p;
	int index = 0;
	if(parent){
		for(p = parent->mChildren.first; p && index != childIndex; p = p->mNext)		index++;
	}
	return p;
}

void gkProcessLayoutMainPanel(gkPanel* panel, float width, float height)
{
    float oldWidth = panel->width;
    float oldHeight = panel->height;
    gkRect clientRect = {0,0, width, height};

	if(panel->layoutFunc)
    {
		panel->layoutFunc(panel, &clientRect);
    }

	panel->width = width;
	panel->height = height;

    gkProcessChildrenLayout(panel);

    if(oldWidth != width || oldHeight != height)
    {
        gkEvent changedEvent;
        changedEvent.type = GK_ON_PANEL_RESIZED;
        changedEvent.target = panel;
        changedEvent.currentTarget = panel;
        gkDispatch(panel, &changedEvent);
    }
}

void gkProcessLayoutPanel(gkPanel* panel, gkRect* clientRect)
{
    float oldWidth = panel->width;
    float oldHeight = panel->height;

	if(panel->layoutFunc)
    {
		panel->layoutFunc(panel, clientRect);
	}

    gkProcessChildrenLayout(panel);

    if(oldWidth != panel->width || oldHeight != panel->height)
    {
        gkEvent changedEvent;
        changedEvent.type = GK_ON_PANEL_RESIZED;
        changedEvent.target = panel;
        changedEvent.currentTarget = panel;
        gkDispatch(panel, &changedEvent);
    }
}
void gkProcessChildrenLayout(gkPanel* panel)
{
    gkPanel* p;
    gkRect clientRect = { 0, 0, panel->width, panel->height };
	gkGuardDestroy(panel);
	for(p = panel->mChildren.first; p; p = panel->mNextChild){
		panel->mNextChild = p->mNext;
        gkProcessLayoutPanel(p, &clientRect);
    }
	gkUnguardDestroy(panel);
}

void gkProcessUpdatePanel(gkPanel* panel){
	gkPanel* p;
	gkGuardDestroy(panel);
	if(panel->updateFunc){
		panel->updateFunc(panel);
	}
	for(p = panel->mChildren.first; p; p = panel->mNextChild){
		panel->mNextChild = p->mNext;
		gkProcessUpdatePanel(p);
	}
	gkUnguardDestroy(panel);
}

void gkProcessDrawPanel(gkPanel* panel)
{
	gkPanel* p;
	gkMatrix t = gkMatrixCreateTranslation(panel->x, panel->y);
	gkMatrix t2 = gkMatrixCreateTranslation(-panel->anchorX*panel->width, -panel->anchorY*panel->height);
	if(!panel->visible)
        return;	/* Don't draw invisible panels */
	gkGuardDestroy(panel);
	gkPushColorFilter(panel->colorFilter.r, panel->colorFilter.g, panel->colorFilter.b, panel->colorFilter.a);
	gkPushTransform(&t);
	gkPushTransform(&panel->transform);
	gkPushTransform(&t2);
	if(panel->mViewport)
    {
		gkMatrix m = gkLocalToGlobal(panel);
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
			panel->drawFunc(panel);
		}
		glPopClientAttrib();
		glPopAttrib();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}else{
		if(panel->drawFunc){
			panel->drawFunc(panel);
		}
		for(p = panel->mChildren.first; p; p = panel->mNextChild){
			panel->mNextChild = p->mNext;
			gkProcessDrawPanel(p);
		}
	}
	gkPopTransform();
	gkPopTransform();
	gkPopTransform();
	gkPopColorFilter();
	gkUnguardDestroy(panel);
}

gkMatrix gkGlobalToLocal(gkPanel* panel){
	gkMatrix m = gkLocalToGlobal(panel);
	gkMatrixInverse(&m);
	return m;
}

gkMatrix gkLocalToGlobal(gkPanel* panel){
	gkMatrix m = gkMatrixCreateIdentity(), t, t2;
	gkPanel* p = panel;
	do{
		t = gkMatrixCreateTranslation(p->x, p->y);
		t2 = gkMatrixCreateTranslation(-p->anchorX*p->width, -p->anchorY*p->height);
		gkMatrixMultPtr(&m, &t2);
		gkMatrixMultPtr(&m, &p->transform);
		gkMatrixMultPtr(&m, &t);
		p = p->parent;
	}while(p);
	return m;
}

/*** Panel input handling ***/

gkPoint gkMousePosition;

#define BYTE_OFFSET(obj, prop) ((char*)&obj->prop - (char*)obj)

gkPanel* gkGetMouseTarget(gkPanel* panel, gkPoint pos, size_t enabledOffset, size_t enabledChildrenOffset){
	gkPanel** children, *p;
	int i = panel->numChildren;
	GK_BOOL *enabled = (GK_BOOL*)((char*)panel + enabledOffset);
	GK_BOOL *enabledChildren = (GK_BOOL*)((char*)panel + enabledChildrenOffset);
	if(!panel->visible) return 0;	/* ignore invisible panels */
	if(*enabledChildren){
		children = (gkPanel**)calloc(panel->numChildren, sizeof(gkPanel*));
		for(p = panel->mChildren.first; p && i>0; p = p->mNext) children[--i] = p;	/* also reverses the order so in the next loop it starts from the last and goes to first */
		for(i = 0; i<panel->numChildren; i++){
            gkMatrix translate, t;
			gkPoint tmpPos = pos;
			p = children[i];
            translate = gkMatrixCreateTranslation(p->x, p->y);
            t = gkMatrixCreateTranslation(-p->anchorX*p->width, -p->anchorY*p->height);
            gkMatrixMultPtr(&t, &p->transform);
			gkMatrixMultPtr(&t, &translate);
			gkMatrixInverse(&t);
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
		gkPanel* p = gkFocusPanel;
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

void gkUpdateMouseTarget(gkPanel* mainPanel){
	gkPanel *oldMouseTarget, *p, *lastCommon;
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

		for(p = oldMouseTarget; p; p = p->parent) p->mouseOver = GK_FALSE;	/* reset mouse over */
		for(p = gkMouseTarget; p; p = p->parent) p->mouseOver = GK_TRUE;	/* set mouse over to all panels under the mouse */
		for(p = oldMouseTarget; p && !p->mouseOver; p = p->parent){
			leave.currentTarget = p;
			gkDispatch(p, &leave);	/* dispatch GK_ON_MOUSE_LEAVE */
		}
		lastCommon = p;
		for(p = gkMouseTarget; p != 0 && p != lastCommon; p = p->parent){
			enter.currentTarget = p;
			gkDispatch(p, &enter);	/* dispatch GK_ON_MOUSE_ENTER */
		}
	}
}

void gkProcessMouseEvent(gkMouseEvent* mouseEvent){
	gkPanel* current, *newFocusTarget = 0, *curFocusTarget = gkFocusPanel;
	gkMousePosition = mouseEvent->position;
	if(mouseEvent->type == GK_ON_MOUSE_DOWN){
		newFocusTarget = gkGetMouseTarget(gkMainPanel, gkMousePosition, BYTE_OFFSET(gkMainPanel, keyboardEnabled), BYTE_OFFSET(gkMainPanel, keyboardChildren));
	}
	if(gkMouseTarget){
		mouseEvent->target = current = gkMouseTarget;
		do{
			gkMatrix m = gkGlobalToLocal(current);
			mouseEvent->position = gkTransformPoint(gkMousePosition, &m);
			mouseEvent->currentTarget = current;
		}while(gkDispatch(current, mouseEvent) && (current = current->parent));
	}
	if(newFocusTarget && curFocusTarget == gkFocusPanel){
		gkSetFocus(newFocusTarget);
	}
}


void gkSetFocus(gkPanel* panel){
	static gkPanel *tmpNextFocused = 0;
	gkPanel* tmp = gkFocusPanel;
	if(panel == gkFocusPanel) return;
	tmpNextFocused = panel;
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
	return gkFocusPanel;
}

void gkProcessKeyboardEvent(gkKeyboardEvent* keyboardEvent){
	if(!gkFocusPanel) return;
	keyboardEvent->target = keyboardEvent->currentTarget = gkFocusPanel;
	gkDispatch(gkFocusPanel, keyboardEvent);
}
