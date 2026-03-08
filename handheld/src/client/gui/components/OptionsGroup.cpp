#include "OptionsGroup.h"
#include "../../Minecraft.h"
#include "ImageButton.h"
#include "OptionsItem.h"
#include "Slider.h"
#include "../../../locale/I18n.h"

OptionsGroup::OptionsGroup( std::string labelID )  {
	label = I18n::get(labelID);
}

void OptionsGroup::setupPositions() {
	// Leave 18px at top for the group header label
	int curY = y + 18;
	for(std::vector<GuiElement*>::iterator it = children.begin(); it != children.end(); ++it) {
		(*it)->width = width - 2;
		(*it)->y = curY;
		(*it)->x = x + 1;
		(*it)->setupPositions();
		curY += (*it)->height + 1;
	}
	height = curY - y + 4;
}

void OptionsGroup::render( Minecraft* minecraft, int xm, int ym ) {
	// Group header background
	fill(x, y, x + width, y + 16, 0x99223344);
	// Header separator line
	fill(x, y + 16, x + width, y + 17, 0x884488aa);
	// Header label
	minecraft->font->draw(label, (float)x + 5, (float)y + 4, 0xffffff, false);
	super::render(minecraft, xm, ym);
}

OptionsGroup& OptionsGroup::addOptionItem( const Options::Option* option, Minecraft* minecraft ) {
	if(option->isBoolean())
		createToggle(option, minecraft);
	else if(option->isProgress())
		createProgressSlider(option, minecraft);
	else if(option->isInt())
		createStepSlider(option, minecraft);
	return *this;
}

void OptionsGroup::createToggle( const Options::Option* option, Minecraft* minecraft ) {
	ImageDef def;
	def.setSrc(IntRectangle(160, 206, 39, 20));
	def.name = "gui/touchgui.png";
	def.width = 39 * 0.7f;
	def.height = 20 * 0.7f;
	OptionButton* element = new OptionButton(option);
	element->setImageDef(def, true);
	element->updateImage(&minecraft->options);
	std::string itemLabel = I18n::get(option->getCaptionId());
	OptionsItem* item = new OptionsItem(itemLabel, element);
	addChild(item);
	setupPositions();
}

void OptionsGroup::createProgressSlider( const Options::Option* option, Minecraft* minecraft ) {
	Slider* element = new Slider(minecraft,
								option,
								minecraft->options.getProgrssMin(option),
								minecraft->options.getProgrssMax(option));
	element->width = 120;
	element->height = 24;
	std::string itemLabel = I18n::get(option->getCaptionId());
	OptionsItem* item = new OptionsItem(itemLabel, element);
	addChild(item);
	setupPositions();
}

void OptionsGroup::createStepSlider( const Options::Option* option, Minecraft* minecraft ) {
	std::vector<int> steps;
	if (option == &Options::Option::DIFFICULTY) {
		steps.push_back(0);
		steps.push_back(2);
	} else if (option == &Options::Option::RENDER_DISTANCE) {
		steps.push_back(0); // Far
		steps.push_back(1); // Normal
		steps.push_back(2); // Short
		steps.push_back(3); // Tiny
	} else if (option == &Options::Option::GUI_SCALE) {
		steps.push_back(0);
		steps.push_back(1);
		steps.push_back(2);
		steps.push_back(3);
	} else if (option == &Options::Option::GRAPHICS) {
		steps.push_back(0);
		steps.push_back(1);
	}

	if (steps.size() < 2) {
		return;
	}

	Slider* element = new Slider(minecraft, option, steps);
	element->width = 120;
	element->height = 24;
	std::string itemLabel = I18n::get(option->getCaptionId());
	OptionsItem* item = new OptionsItem(itemLabel, element);
	addChild(item);
	setupPositions();
}
