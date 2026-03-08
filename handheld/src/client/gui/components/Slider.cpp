#include "Slider.h"
#include "../../Minecraft.h"
#include "../../renderer/Textures.h"
#include "../Screen.h"
#include "../../../util/Mth.h"
#include "../../../locale/I18n.h"
#include <algorithm>
#include <assert.h>
#include <sstream>
Slider::Slider(Minecraft* minecraft, const Options::Option* option,  float progressMin, float progressMax)
: sliderType(SliderProgress), mouseDownOnElement(false), option(option), numSteps(0), progressMin(progressMin), progressMax(progressMax) {
	if(option != NULL) {
		const float range = progressMax - progressMin;
		if (Mth::abs(range) > 0.0001f) {
			percentage = (minecraft->options.getProgressValue(option) - progressMin) / range;
		} else {
			percentage = 0.0f;
		}
		percentage = Mth::clamp(percentage, 0.0f, 1.0f);
	}
}

Slider::Slider(Minecraft* minecraft, const Options::Option* option, const std::vector<int>& stepVec )
: sliderType(SliderStep),
  curStepValue(0),
  curStep(0),
  sliderSteps(stepVec),
  mouseDownOnElement(false),
  option(option),
  percentage(0),
  progressMin(0.0f),
  progressMax(1.0) {
	assert(stepVec.size() > 1);
	numSteps = sliderSteps.size();
	if(option != NULL) {
		curStepValue = minecraft->options.getIntValue(option);
		std::vector<int>::iterator currentItem = std::find(sliderSteps.begin(), sliderSteps.end(), curStepValue);
		if(currentItem != sliderSteps.end()) {
			curStep = (int)(currentItem - sliderSteps.begin());
		}
		curStep = Mth::Min(curStep, numSteps - 1);
		curStepValue = sliderSteps[curStep];
		percentage = float(curStep) / float(numSteps - 1);
	}
}

static std::string sliderValueText(SliderType sliderType, const Options::Option* option,
                                    float percentage, int curStepValue,
                                    float progressMin, float progressMax) {
	if (sliderType == SliderStep) {
		if (option == &Options::Option::RENDER_DISTANCE) {
			int idx = Mth::clamp(curStepValue, 0, 7);
			return I18n::get(Options::RENDER_DISTANCE_NAMES[idx]);
		} else if (option == &Options::Option::DIFFICULTY) {
			int idx = Mth::clamp(curStepValue, 0, 3);
			return I18n::get(Options::DIFFICULTY_NAMES[idx]);
		} else if (option == &Options::Option::GUI_SCALE) {
			int idx = Mth::clamp(curStepValue, 0, 3);
			return I18n::get(Options::GUI_SCALE[idx]);
		} else if (option == &Options::Option::GRAPHICS) {
			return curStepValue ? I18n::get("options.graphics.fancy") : I18n::get("options.graphics.fast");
		}
		std::stringstream ss; ss << curStepValue; return ss.str();
	} else {
		int pct = (int)(percentage * 100.0f + 0.5f);
		std::stringstream ss; ss << pct << "%"; return ss.str();
	}
}

void Slider::render( Minecraft* minecraft, int xm, int ym ) {
	// Reserve right portion for value text (~32px)
	const int textAreaW = 32;
	int xSliderStart = x + 5;
	int xSliderEnd = x + width - 5 - textAreaW;
	int barHeight = 4;
	int ySliderStart = y + (height - barHeight) / 2;
	int ySliderEnd   = ySliderStart + barHeight;
	int handleSizeX = 9;
	int handleSizeY = 15;
	int barWidth = xSliderEnd - xSliderStart;

	// Track background
	fill(xSliderStart, ySliderStart, xSliderEnd, ySliderEnd, 0xff303030);
	// Filled portion (progress indicator)
	fill(xSliderStart, ySliderStart, xSliderStart + (int)(percentage * barWidth), ySliderEnd, 0xff4488bb);

	if(sliderType == SliderStep) {
		int stepDistance = (numSteps > 1) ? barWidth / (numSteps - 1) : 0;
		for(int a = 0; a <= numSteps - 1; ++a) {
			int rx = xSliderStart + a * stepDistance;
			fill(rx, ySliderStart - 1, rx + 1, ySliderEnd + 1, 0xff888888);
		}
	}

	minecraft->textures->loadAndBindTexture("gui/touchgui.png");
	int handleX = xSliderStart + (int)(percentage * barWidth) - handleSizeX / 2;
	int handleY = y + (height - handleSizeY) / 2;
	blit(handleX, handleY, 226, 126, handleSizeX, handleSizeY, handleSizeX, handleSizeY);

	// Value label on the right
	std::string valText = sliderValueText(sliderType, option, percentage, curStepValue, progressMin, progressMax);
	int textY = y + (height - 8) / 2;
	minecraft->font->draw(valText, (float)(x + width - textAreaW + 2), (float)textY, 0xaaddff, false);
}

void Slider::mouseClicked( Minecraft* minecraft, int x, int y, int buttonNum ) {
	if(pointInside(x, y)) {
		mouseDownOnElement = true;
	}
}

void Slider::mouseReleased( Minecraft* minecraft, int x, int y, int buttonNum ) {
	mouseDownOnElement = false;
	if(sliderType == SliderStep) {
		curStep = Mth::floor((percentage * (numSteps-1) + 0.5f));
		curStepValue = sliderSteps[Mth::Min(curStep, numSteps-1)];
		percentage = float(curStep) / (numSteps - 1);
		setOption(minecraft);
	}
}

void Slider::tick(Minecraft* minecraft) {
	if(minecraft->screen != NULL) {
		int xm = Mouse::getX();
		int ym = Mouse::getY();
		minecraft->screen->toGUICoordinate(xm, ym);
		if(mouseDownOnElement) {
			const int xSliderStart = x + 5;
			const int textAreaW = 32;
			const int sliderWidth = width - 10 - textAreaW; // must match render()
			if (sliderWidth <= 0) return;
			percentage = float(xm - xSliderStart) / float(sliderWidth);
			percentage = Mth::clamp(percentage, 0.0f, 1.0f);
			if (sliderType == SliderStep) {
				// Update display live so value label tracks the handle
				int displayStep = Mth::floor(percentage * (numSteps - 1) + 0.5f);
				displayStep = Mth::Min(displayStep, numSteps - 1);
				curStepValue = sliderSteps[displayStep];
				// Commit is deferred to mouseReleased
			} else {
				setOption(minecraft);
			}
		}
	}
}

void Slider::setOption( Minecraft* minecraft ) {
	if(option != NULL) {
		if(sliderType == SliderStep) {
			if(minecraft->options.getIntValue(option) != curStepValue) {
				minecraft->options.set(option, curStepValue);
			}
		} else {
			const float value = percentage * (progressMax - progressMin) + progressMin;
			if(Mth::abs(minecraft->options.getProgressValue(option) - value) > 0.0001f) {
				minecraft->options.set(option, value);
			}
		}
	}
}
