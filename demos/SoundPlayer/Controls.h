#ifndef _CONTROLS_H_
#define _CONTROLS_H_

#include <gk.h>

gkPanel* CreateLabel(char* label);

gkPanel* CreateButton(char* label);

typedef struct Slider Slider;

#define ON_SLIDER_CHANGE	0x8001

Slider* CreateSlider();
void SetSliderRange(Slider* slider, float min, float max);
void GetSliderRange(Slider* slider, float *min, float *max);
void SetSliderPos(Slider* slider, float value);
void GetSliderPos(Slider* slider, float *value);
void DestroySlider(Slider* slider);

#endif