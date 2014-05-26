#include "Controls.h"
#include "Assets.h"

gkTextFormat labelFormat;

typedef struct Label
{
	gkPanel base;
	char* label;
}Label;

static void drawLabel(gkPanel* p)
{
	Label *btn = (Label*)p;

	gkSetFillColor(1.0f, 1.0f, 1.0f, 1.0f);
	gkDrawText(font, btn->label, 10, 7, &labelFormat);
}

gkPanel* CreateLabel(char* label)
{
	gkSize btnSize;
	Label* btn = (Label*)gkCreatePanelEx(sizeof(Label));
	btn->label = label;
	btn->base.drawFunc = drawLabel;
	labelFormat = gkDefaultTextFormat;
	labelFormat.strokeColor = GK_COLOR(0, 0, 0, 1.0f);
	labelFormat.strokeSize = 3;
	btnSize = gkMeasureText(font, label, &labelFormat);
	btn->base.width = btnSize.width + 20;
	btn->base.height = 30;
	return (gkPanel*)btn;
}

static void drawButton(gkPanel* p)
{
	Label *btn = (Label*)p;

	gkSetFillColor(1.0f, 1.0f, 1.0f, (btn->base.mouseOver)?0.5f:0.2f);
	gkSetLineColor(0.5f, 0.5f, 0.5f, 1.0f);
	gkSetLineWidth(1.5f);
	gkDrawRoundRect(0,0, p->width, p->height, 4,4);

	drawLabel(p);
}

gkPanel* CreateButton(char* label)
{
	Label* btn = (Label*)CreateLabel(label);
	btn->base.drawFunc = drawButton;
	return (gkPanel*)btn;
}


typedef struct Slider
{
	gkPanel base;
	float min;
	float max;
	float current;
	GK_BOOL drag;
}Slider;


static void drawSlider(gkPanel *p)
{
	Slider* slider = (Slider*)p;
	float v = (slider->current - slider->min)/(slider->max - slider->min);

	gkSetLineWidth(1.0f);
	gkSetLineColor(1.0f, 1.0f, 1.0f, 0.5f);
	gkSetFillColor(0.5f, 0.5f, 0.5f, p->mouseOver?0.35f:0.25f);

	gkDrawRoundRect(0,0,p->width, p->height, 4,4);

	if (v > 0.0f) {
		gkSetFillColor(1.0f, 0.0f, 0.0f, 1.0f);
		gkDrawRoundRect(1,1, (p->width - 2.0f)*v, (p->height - 2.0f), 4,4);
	}
}

void updateSlider(Slider* slider, float mouseX)
{
	gkEvent evt;

	float v = mouseX/slider->base.width;
	if (v < 0.0f) v = 0.0f;
	else if (v > 1.0f) v = 1.0f;
	slider->current = slider->min + v*(slider->max - slider->min);

	evt.type = ON_SLIDER_CHANGE;
	evt.target = slider;
	evt.currentTarget = slider;
	gkDispatch(slider, &evt);
}

GK_BOOL onMouseRelease(gkEvent* evt, void* p)
{
	Slider* slider = (Slider*)p;
	slider->drag = GK_FALSE;
	gkRemoveListener(gkMainPanel, 0, onMouseRelease, slider);
	return GK_TRUE;
}

GK_BOOL onSliderClicked(gkEvent* evt, void *p)
{
	gkMouseEvent* mevt = (gkMouseEvent*)evt;
	Slider* slider = (Slider*)evt->target;
	if (slider->drag)
		return GK_TRUE;

	updateSlider(slider, mevt->position.x);

	slider->drag = GK_TRUE;
	gkAddListener(gkMainPanel, GK_ON_MOUSE_UP, 0, onMouseRelease, slider);

	return GK_TRUE;
}

GK_BOOL onSliderMMove(gkEvent* evt, void *p)
{
	gkMouseEvent* mevt = (gkMouseEvent*)evt;
	Slider* slider = (Slider*)evt->target;
	if (!slider->drag)
		return GK_TRUE;

	updateSlider(slider, mevt->position.x);
	return GK_TRUE;
}

Slider* CreateSlider()
{
	Slider* slider = (Slider*)gkCreatePanelEx(sizeof(struct Slider));
	slider->base.drawFunc = drawSlider;
	slider->base.height = 25;
	slider->base.width = 100;
	slider->drag = GK_FALSE;
	gkAddListener(slider, GK_ON_MOUSE_DOWN, 0, onSliderClicked, 0);
	gkAddListener(slider, GK_ON_MOUSE_MOVE, 0, onSliderMMove, 0);
	slider->min = 0.0f;
	slider->max = 1.0f;
	slider->current = 0.5f;
	return slider;
}

void SetSliderRange(Slider* slider, float min, float max)
{
	slider->min = min;
	slider->max = max;
}

void GetSliderRange(Slider* slider, float *min, float *max)
{
	*min = slider->min;
	*max = slider->max;
}

void SetSliderPos(Slider* slider, float value)
{
	slider->current = value;
}

void GetSliderPos(Slider* slider, float *value)
{
	*value = slider->current;
}

void DestroySlider(Slider* slider)
{
	if (slider->drag) 
		gkRemoveListener(gkMainPanel, 0, onMouseRelease, slider);

	gkDestroyPanel((gkPanel*)slider);
}
