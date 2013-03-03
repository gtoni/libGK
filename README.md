libGK
=====
The purpose of this library is to provide the basic things that a game needs. 
Such as loading of images, fonts and sounds, drawing things on the screen, 
capturing the user input and provide tweening and timing functions. 


A "Hello World" app
-------------------
The code below creates a resizeable OpenGL window, which is ready for action.

	#include <gk.h>

	void main()
	{
		if(gkInit())
		{
			gkSetScreenSize(GK_SIZE(800,600));
			gkSetWindowTitle(L"Hello World");
			gkSetWindowResizable(GK_TRUE);
			
			/* initialize game */
			
			gkRun();
			
			/* destroy game */
		}
	}