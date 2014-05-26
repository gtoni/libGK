#ifndef _ASSETS_H_
#define _ASSETS_H_


#include <gk.h>

extern gkFont* font;

void loadAssets();
void cleanupAssets();

char* GetRandomStaticSound();
char* GetRandomStreamSound();

#endif