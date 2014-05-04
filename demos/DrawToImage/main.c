#include <gk.h>

gkFont* font;
gkTextFormat tf;

void CreateOfflinePanel();
void DestroyOfflinePanel();

void CreateUpdatePanel();
void DestroyUpdatePanel();

void CreateDrawPanel();
void DestroyDrawPanel();

void CreateBackgroundPanel();
void DestroyBackgroundPanel();

void CreateForegroundPanel();
void DestroyForegroundPanel();

GK_BOOL init()
{
	gkAddFontResource("meiryo.ttc");
	font = gkCreateFont("Meiryo", 16, GK_FONT_NORMAL);

	tf = gkDefaultTextFormat;
	tf.width = 400;
	tf.height = 50;
	tf.strokeColor = GK_COLOR(0,0,0,1);
	tf.strokeSize = 4;
	tf.align = GK_TEXT_ALIGN_CENTER;
	tf.valign = GK_TEXT_VALIGN_MIDDLE;

	gkSetTargetFps(GK_VSYNC);
	gkSetScreenSize(GK_SIZE(1200, 400));

	CreateBackgroundPanel();

	CreateOfflinePanel();
	CreateUpdatePanel();
	CreateDrawPanel();

	CreateForegroundPanel();

	return GK_TRUE;
}

void cleanup()
{
	DestroyForegroundPanel();

	DestroyDrawPanel();
	DestroyUpdatePanel();
	DestroyOfflinePanel();

	DestroyBackgroundPanel();

	gkDestroyFont(font);
}

void main()
{
	gkMain(init, cleanup);
}

/* Draw panel content */

float rotation = 0;

void DrawContent(char* message)
{
	gkMatrix mat;

	gkSetFillColor(1.0f,0,0,0.5f);
	gkDrawRect(0,0, 400, 400);
	gkSetFillColor(0,0,0.5f, 0.35f);
	gkDrawRect(50,50, 300, 300);

	mat = gkMatrixCreateTranslation(-100.0f, -100.0f);
	gkMatrixMult(&mat, gkMatrixCreateRotation(rotation * (3.14f/180.0f)));
	gkMatrixMult(&mat, gkMatrixCreateTranslation(200.0f, 200.0f));

	gkPushTransform(&mat);
	gkSetFillColor(0,0.5f,0,1);
	gkDrawRect(0,0, 200, 200);
	gkPopTransform();

	gkSetFillColor(0, 1.0, 0, 1.0);
	gkDrawText(font, message, 0, 0, &tf);
}


/* 
	The first panel draws an image when it's initialized. 
	That's why it's called offline.
*/

gkPanel* offlinePanel;
gkImage* offlineImage;

void DrawOfflinePanel(gkPanel* p)
{
	gkDrawImage(offlineImage, 0, 0);
}

void CreateOfflinePanel()
{
	offlinePanel = gkCreatePanel();
	offlinePanel->drawFunc = DrawOfflinePanel;
	offlineImage = gkCreateImage(400, 400);

	if (gkBeginDrawToImage(offlineImage, GK_TRUE)) {
		DrawContent("Offline panel");
		gkEndDrawToImage();
	}

	gkAddChild(gkMainPanel, offlinePanel);
}

void DestroyOfflinePanel()
{
	gkDestroyImage(offlineImage);
	gkDestroyPanel(offlinePanel);
}

/* 
	The second panel draws an image in the update method. 
	That's why it's called 'update panel'.
*/

gkPanel* updatePanel;
gkImage* updateImage;

void UpdateUpdatePanel(gkPanel* p)
{
	rotation += 0.5f;

	if (gkBeginDrawToImage(updateImage, GK_TRUE)) {
		DrawContent("Update panel");
		gkEndDrawToImage();
	}
}

void DrawUpdatePanel(gkPanel* p)
{
	gkDrawImage(updateImage, 0, 0);
}

void CreateUpdatePanel()
{
	updatePanel = gkCreatePanel();
	updatePanel->updateFunc = UpdateUpdatePanel;	//weird naming
	updatePanel->drawFunc = DrawUpdatePanel;
	updateImage = gkCreateImage(400, 400);

	updatePanel->x = 400;
	gkAddChild(gkMainPanel, updatePanel);
}

void DestroyUpdatePanel()
{
	gkDestroyImage(updateImage);
	gkDestroyPanel(updatePanel);
}

/* 
	The third panel draws an image in the draw method. 
	That's why it's called 'draw panel'.
*/

gkPanel* drawPanel;
gkImage* drawImage;


void DrawDrawPanel(gkPanel* p)
{
	if (gkBeginDrawToImage(drawImage, GK_TRUE)) {
		DrawContent("Draw panel");
		gkEndDrawToImage();
	}
	gkDrawImage(drawImage, 0, 0);
}

void CreateDrawPanel()
{
	drawPanel = gkCreatePanel();
	drawPanel->drawFunc = DrawDrawPanel;
	drawImage = gkCreateImage(400, 400);

	drawPanel->x = 800;
	gkAddChild(gkMainPanel, drawPanel);
}

void DestroyDrawPanel()
{
	gkDestroyImage(drawImage);
	gkDestroyPanel(drawPanel);
}

/*
	Background panel
*/

#define PATTERN_SIZE 100

gkImage* bgPattern[2];

void DrawPatternImage(GK_BOOL invert, float width, float height)
{
	gkPoint path[] = {
		{width*0.5f, 0},
		{0, height*0.5f},
		{width*0.5f, height},
		{width, height*0.5f}};

	if(invert)
		gkSetFillColor(0,0,0,1.0);
	else
		gkSetFillColor(0.5,0.5,0.5,1);

	gkDrawRect(0, 0, width, height);


	if(invert)
		gkSetFillColor(0.5,0.5,0.5,1);
	else
		gkSetFillColor(0,0,0,1.0);

	gkDrawPath(path, 4, GK_TRUE);

	if(invert)
		gkSetFillColor(0,0,0,1.0);
	else
		gkSetFillColor(0.5,0.5,0.5,1);

	gkDrawCircle(width*0.5f, height*0.5f, width*0.25f);
}

void CreateBackgroundPatterns()
{
	bgPattern[0] = gkCreateImage(PATTERN_SIZE,PATTERN_SIZE);
	bgPattern[1] = gkCreateImage(PATTERN_SIZE,PATTERN_SIZE);
	if (gkBeginDrawToImage(bgPattern[0], GK_TRUE)) {
		DrawPatternImage(GK_FALSE, PATTERN_SIZE, PATTERN_SIZE);
		gkEndDrawToImage();
	}
	if (gkBeginDrawToImage(bgPattern[1], GK_TRUE)) {
		DrawPatternImage(GK_TRUE, PATTERN_SIZE, PATTERN_SIZE);
		gkEndDrawToImage();
	}
}

void DestroyBackgroundPatterns()
{
	gkDestroyImage(bgPattern[0]);
	gkDestroyImage(bgPattern[1]);
}


gkPanel* bgPanel;

float offset = 0;

void UpdateBackgroundPanel(gkPanel* p)
{
	offset += 0.15f;
}

void DrawBackgroundPanel(gkPanel* p)
{
	float tx, ty;
	int d = 1, off = (int)offset;

	for (ty = 0.0f; ty < p->height; ty += PATTERN_SIZE) {
		int i = (off/PATTERN_SIZE)%2;
		tx = -PATTERN_SIZE + (float)((off*d)%PATTERN_SIZE);
		for (; tx < p->width + PATTERN_SIZE; tx += PATTERN_SIZE) {
			gkDrawImage(bgPattern[i++%2], tx, ty);
		}
		d *= -1;
	}
}

void CreateBackgroundPanel()
{
	gkSize size = gkGetScreenSize();
	CreateBackgroundPatterns();
	bgPanel = gkCreatePanel();
	bgPanel->width = size.width;
	bgPanel->height = size.height;
	bgPanel->updateFunc = UpdateBackgroundPanel;
	bgPanel->drawFunc = DrawBackgroundPanel;

	gkAddChild(gkMainPanel, bgPanel);
}

void DestroyBackgroundPanel()
{
	gkDestroyPanel(bgPanel);
	DestroyBackgroundPatterns();
}

/*
	Foreground panel
*/

gkPanel* frPanel;
gkPoint ballPos;
gkPoint ballVel;

#define BALL_RADIUS 30.0f

void UpdateForegroundPanel(gkPanel* p)
{
	gkPoint nPos = {ballPos.x + ballVel.x, ballPos.y + ballVel.y};

	if ((nPos.x + BALL_RADIUS) >= p->width) {
		ballPos.x = p->width - BALL_RADIUS;
		ballVel.x *= -1;
	} else if (nPos.x < BALL_RADIUS) {
		ballPos.x = BALL_RADIUS;
		ballVel.x *= -1;
	} else {
		ballPos.x = nPos.x;
	}

	if ((nPos.y + BALL_RADIUS) >= p->height) {
		ballPos.y = p->height - BALL_RADIUS;
		ballVel.y *= -1;
	} else if (nPos.y < BALL_RADIUS) {
		ballPos.y = BALL_RADIUS;
		ballVel.y *= -1;
	} else {
		ballPos.y = nPos.y;
	}
}

void DrawForegroundPanel(gkPanel* p)
{
	gkSetFillColor(1.0f, 0.6f, 0.1f, 1.0f);
	gkDrawCircle(ballPos.x, ballPos.y, BALL_RADIUS);
}

void CreateForegroundPanel()
{
	gkSize size = gkGetScreenSize();
	frPanel = gkCreatePanel();
	frPanel->width = size.width;
	frPanel->height = size.height;
	frPanel->updateFunc = UpdateForegroundPanel;
	frPanel->drawFunc = DrawForegroundPanel;

	ballPos = GK_POINT(size.width*0.5f, size.height*0.5f);
	ballVel = GK_POINT(0.5f, 0.5f);

	gkAddChild(gkMainPanel, frPanel);
}

void DestroyForegroundPanel()
{
	gkDestroyPanel(frPanel);
}