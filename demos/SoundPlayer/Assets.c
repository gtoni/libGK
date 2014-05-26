#include "Assets.h"

gkFont* font;

void loadAssets()
{
	gkAddFontResource("../demos/TestRun/meiryo.ttc");
	font = gkCreateFont("meiryo", 14, GK_FONT_NORMAL);
}

void cleanupAssets()
{
	gkDestroyFont(font);
}

char* GetRandomStaticSound()
{
	return "../demos/TestRun/Cat.wav";
}

char* GetRandomStreamSound()
{
	return "../demos/TestRun/Adrenaline.wav";
}