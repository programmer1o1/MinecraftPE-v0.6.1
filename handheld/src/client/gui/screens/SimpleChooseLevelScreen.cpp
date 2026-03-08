#include "SimpleChooseLevelScreen.h"
#include "ProgressScreen.h"
#include "ScreenChooser.h"
#include "../../Minecraft.h"
#include "../../../world/level/LevelSettings.h"
#include "../../../platform/time.h"

SimpleChooseLevelScreen::SimpleChooseLevelScreen(const std::string& levelName)
:	bTitle(0),
	bBack(0),
	bCreative(0),
	bSurvival(0),
	bOldWorld(0),
	bInfiniteWorld(0),
	levelName(levelName),
	hasChosen(false),
	chosenGameType(-1)
{
}

SimpleChooseLevelScreen::~SimpleChooseLevelScreen()
{
	delete bTitle;
	delete bBack;
	delete bCreative;
	delete bSurvival;
	delete bOldWorld;
	delete bInfiniteWorld;
}

void SimpleChooseLevelScreen::init()
{
	bTitle        = new Touch::THeader(0, "Game Mode");
	bBack         = new Touch::TButton(3, "Back");
	bCreative     = new Touch::TButton(1, "Creative");
	bSurvival     = new Touch::TButton(2, "Survival");
	bOldWorld     = new Touch::TButton(4, "Old World");
	bInfiniteWorld= new Touch::TButton(5, "Infinite");

	bOldWorld->visible      = false;  bOldWorld->active      = false;
	bInfiniteWorld->visible = false;  bInfiniteWorld->active = false;

	buttons.push_back(bTitle);
	buttons.push_back(bBack);
	buttons.push_back(bCreative);
	buttons.push_back(bSurvival);
	buttons.push_back(bOldWorld);
	buttons.push_back(bInfiniteWorld);

	tabButtons.push_back(bBack);
	tabButtons.push_back(bCreative);
	tabButtons.push_back(bSurvival);
	tabButtons.push_back(bOldWorld);
	tabButtons.push_back(bInfiniteWorld);
}

void SimpleChooseLevelScreen::setupPositions()
{
	const int headerH = bTitle->height; // 26px
	const int bw = 100;
	const int bh = bBack->height; // 26px

	// Header bar: title spans between Back button and right edge
	bBack->x  = 0;
	bBack->y  = 0;
	bBack->width = bw;

	bTitle->x    = bBack->width;
	bTitle->y    = 0;
	bTitle->width = width - bBack->width;

	// Content buttons — two big buttons side by side in the center
	const int btnW  = (width / 2) - 16;
	const int btnH  = 36;
	const int btnY  = headerH + (height - headerH) / 2 - btnH / 2;

	bCreative->width  = btnW;  bCreative->height  = btnH;
	bSurvival->width  = btnW;  bSurvival->height  = btnH;
	bOldWorld->width  = btnW;  bOldWorld->height  = btnH;
	bInfiniteWorld->width  = btnW;  bInfiniteWorld->height  = btnH;

	bCreative->x  = 8;
	bCreative->y  = btnY;
	bSurvival->x  = width / 2 + 8;
	bSurvival->y  = btnY;

	bOldWorld->x      = 8;
	bOldWorld->y      = btnY;
	bInfiniteWorld->x = width / 2 + 8;
	bInfiniteWorld->y = btnY;
}

void SimpleChooseLevelScreen::render( int xm, int ym, float a )
{
	renderBackground();
    glEnable2(GL_BLEND);

	int descY = bCreative->y + bCreative->height + 6;
	if (chosenGameType == -1) {
		drawCenteredString(minecraft->font, "Unlimited resources, no damage", bCreative->x + bCreative->width / 2, descY, 0xffaaaaaa);
		drawCenteredString(minecraft->font, "Survive, gather, build",          bSurvival->x + bSurvival->width / 2, descY, 0xffaaaaaa);
	} else {
		drawCenteredString(minecraft->font, "Finite 256x256",    bOldWorld->x      + bOldWorld->width      / 2, descY, 0xffaaaaaa);
		drawCenteredString(minecraft->font, "Endless world",     bInfiniteWorld->x + bInfiniteWorld->width / 2, descY, 0xffaaaaaa);
	}

	Screen::render(xm, ym, a);
    glDisable2(GL_BLEND);
}

void SimpleChooseLevelScreen::buttonClicked( Button* button )
{
	if (button == bBack) {
		if (chosenGameType != -1) {
			// Go back to step 1
			chosenGameType = -1;
			bTitle->msg     = "Game Mode";
			bCreative->visible = true;   bCreative->active = true;
			bSurvival->visible = true;   bSurvival->active = true;
			bOldWorld->visible = false;  bOldWorld->active = false;
			bInfiniteWorld->visible = false; bInfiniteWorld->active = false;
		} else {
			minecraft->screenChooser.setScreen(SCREEN_STARTMENU);
		}
		return;
	}

	// Step 1: pick game mode
	if (chosenGameType == -1) {
		if (button == bCreative)
			chosenGameType = GameType::Creative;
		else if (button == bSurvival)
			chosenGameType = GameType::Survival;
		else
			return;

		bTitle->msg     = "World Type";
		bCreative->visible = false;  bCreative->active = false;
		bSurvival->visible = false;  bSurvival->active = false;
		bOldWorld->visible = true;   bOldWorld->active = true;
		bInfiniteWorld->visible = true;  bInfiniteWorld->active = true;
		return;
	}

	// Step 2: pick world type
	if (hasChosen) return;

	int worldType = -1;
	if (button == bOldWorld)
		worldType = WorldType::Old;
	else if (button == bInfiniteWorld)
		worldType = WorldType::Infinite;
	else
		return;

	std::string levelId = getUniqueLevelName(levelName);
	LevelSettings settings(getEpochTimeS(), chosenGameType, worldType);
	minecraft->selectLevel(levelId, levelId, settings);
	minecraft->hostMultiplayer();
	minecraft->setScreen(new ProgressScreen());
	hasChosen = true;
}

bool SimpleChooseLevelScreen::handleBackEvent(bool isDown) {
	if (!isDown) {
		if (chosenGameType != -1) {
			chosenGameType = -1;
			bTitle->msg     = "Game Mode";
			bCreative->visible = true;   bCreative->active = true;
			bSurvival->visible = true;   bSurvival->active = true;
			bOldWorld->visible = false;  bOldWorld->active = false;
			bInfiniteWorld->visible = false; bInfiniteWorld->active = false;
		} else {
			minecraft->screenChooser.setScreen(SCREEN_STARTMENU);
		}
	}
	return true;
}
