#include "OptionsItem.h"
#include "../../Minecraft.h"
#include "../../../util/Mth.h"

OptionsItem::OptionsItem( std::string label, GuiElement* element )
: GuiElementContainer(false, true, 0, 0, 24, 12),
  label(label) {
	addChild(element);
}

void OptionsItem::setupPositions() {
	int maxChildH = 0;
	for(std::vector<GuiElement*>::iterator it = children.begin(); it != children.end(); ++it)
		if ((*it)->height > maxChildH) maxChildH = (*it)->height;
	height = Mth::Max(maxChildH, 24);

	for(std::vector<GuiElement*>::iterator it = children.begin(); it != children.end(); ++it) {
		(*it)->x = x + width - (*it)->width - 4;
		(*it)->y = y + (height - (*it)->height) / 2; // vertically centered
	}
}

void OptionsItem::render( Minecraft* minecraft, int xm, int ym ) {
	// Alternating row background for readability
	bool even = ((y / 26) % 2 == 0);
	fill(x, y, x + width, y + height, even ? 0x33ffffff : 0x11ffffff);

	int yOffset = (height - 8) / 2;
	minecraft->font->draw(label, (float)x + 4, (float)y + yOffset, 0xe0e0e0, false);
	super::render(minecraft, xm, ym);
}
