#include "Slider.h"
#include "../../Minecraft.h"
#include "../../renderer/Textures.h"
#include "../Screen.h"
#include "../../../util/Mth.h"
#include <algorithm>
#include <assert.h>
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

void Slider::render( Minecraft* minecraft, int xm, int ym ) {
	int xSliderStart = x + 5;
	int xSliderEnd = x + width - 5;
	int ySliderStart = y + 6;
	int ySliderEnd = y + 9;
	int handleSizeX = 9;
	int handleSizeY = 15;
	int barWidth = xSliderEnd - xSliderStart;
	//fill(x, y + 8, x + (int)(width * percentage), y + height, 0xffff00ff);
	fill(xSliderStart, ySliderStart, xSliderEnd, ySliderEnd, 0xff606060);
	if(sliderType == SliderStep) {
		int stepDistance = barWidth / (numSteps -1);
		for(int a = 0; a <= numSteps - 1; ++a) {
			int renderSliderStepPosX = xSliderStart + a * stepDistance + 1;
			fill(renderSliderStepPosX - 1, ySliderStart - 2, renderSliderStepPosX + 1, ySliderEnd + 2, 0xff606060);
		}
	}
	minecraft->textures->loadAndBindTexture("gui/touchgui.png");
	blit(xSliderStart + (int)(percentage * barWidth) - handleSizeX / 2, y, 226, 126, handleSizeX, handleSizeY, handleSizeX, handleSizeY);
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
			const int sliderWidth = width - 10;
			if (sliderWidth <= 0) {
				return;
			}
			percentage = float(xm - xSliderStart) / float(sliderWidth);
			percentage = Mth::clamp(percentage, 0.0f, 1.0f);
			setOption(minecraft);
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
