#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__DemoChooseLevelScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__DemoChooseLevelScreen_H__

#include "ChooseLevelScreen.h"
#include "../components/Button.h"

class SimpleChooseLevelScreen: public ChooseLevelScreen
{
public:
	SimpleChooseLevelScreen(const std::string& levelName);

	virtual ~SimpleChooseLevelScreen();

	void init();
	void setupPositions();
	void render(int xm, int ym, float a);
	void buttonClicked(Button* button);
	bool handleBackEvent(bool isDown);

private:
	// Header / nav
	Touch::THeader* bTitle;
	Touch::TButton* bBack;

	// Step 1: game mode
	Touch::TButton* bCreative;
	Touch::TButton* bSurvival;

	// Step 2: world type
	Touch::TButton* bOldWorld;
	Touch::TButton* bInfiniteWorld;

	bool hasChosen;
	int chosenGameType; // -1 until chosen

	std::string levelName;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__DemoChooseLevelScreen_H__*/
