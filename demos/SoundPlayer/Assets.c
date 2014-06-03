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
	return "D:\\games\\The Elder Scrolls IV Oblivion GOTY Deluxe\\Data\\sound\\fx\\Cliffworms\\SoundsOfCyrodiil\\AtmosphereOutside\\BravilDay\\SoCDogBark01.wav";
}

char* GetRandomStreamSound()
{
	return "../demos/TestRun/Adrenaline.wav";
}