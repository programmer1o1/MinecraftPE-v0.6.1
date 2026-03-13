#include "OptionsPane.h"
#include "OptionsGroup.h"
#include "OptionsItem.h"
#include "ImageButton.h"
#include "Slider.h"
#include "../../Minecraft.h"
#include "../Gui.h"
#include "../Screen.h"
#include "../../renderer/gles.h"
#include "../../../platform/input/Mouse.h"
#include "../../../util/Mth.h"

OptionsPane::OptionsPane()
: _scrollY(0),
  _scrollVelocity(0),
  _contentHeight(0),
  _visibleHeight(0),
  _dragging(false),
  _dragStartY(0),
  _dragStartScroll(0),
  _lastDragY(0)
{
}

void OptionsPane::setupPositions() {
	int currentHeight = y + 1;
	for(std::vector<GuiElement*>::iterator it = children.begin(); it != children.end(); ++it ) {
		(*it)->width = width;
		(*it)->y = currentHeight;
		(*it)->x = x;
		currentHeight += (*it)->height + 1;
	}
	_contentHeight = currentHeight - y;
	height = currentHeight;
	super::setupPositions();
}

void OptionsPane::render(Minecraft* minecraft, int xm, int ym) {
	int screenH = (int)(minecraft->height * Gui::InvGuiScale);
	_visibleHeight = screenH - y;
	if (_visibleHeight < 1) return;

	// Clamp scroll
	int maxScroll = _contentHeight - _visibleHeight;
	if (maxScroll < 0) maxScroll = 0;
	if (_scrollY < 0) _scrollY = 0;
	if (_scrollY > maxScroll) _scrollY = (float)maxScroll;

	// Scissor clip to visible area
	float scale = Gui::GuiScale;
	int physH = minecraft->height;
	GLint sx = (GLint)(scale * x);
	GLint sy = physH - (GLint)(scale * (y + _visibleHeight));
	GLsizei sw = (GLsizei)(scale * width);
	GLsizei sh = (GLsizei)(scale * _visibleHeight);
	glEnable2(GL_SCISSOR_TEST);
	glScissor(sx, sy, sw, sh);

	// Translate children up by scroll offset
	glPushMatrix2();
	glTranslatef2(0, -(int)_scrollY, 0);

	super::render(minecraft, xm, ym + (int)_scrollY);

	glPopMatrix2();

	// Draw scrollbar if content overflows
	if (maxScroll > 0) {
		float barH = (float)_visibleHeight * _visibleHeight / _contentHeight;
		if (barH < 8) barH = 8;
		float barY = y + (_scrollY / maxScroll) * (_visibleHeight - barH);
		int barX = x + width - 3;
		fill(barX, (int)barY, barX + 2, (int)(barY + barH), 0x88ffffff);
	}

	glDisable2(GL_SCISSOR_TEST);
}

void OptionsPane::mouseClicked(Minecraft* minecraft, int mx, int my, int buttonNum) {
	if (mx < x || mx > x + width || my < y || my > y + _visibleHeight)
		return;

	_dragging = true;
	_dragStartY = my;
	_dragStartScroll = _scrollY;
	_lastDragY = my;
	_scrollVelocity = 0;

	super::mouseClicked(minecraft, mx, my + (int)_scrollY, buttonNum);
}

void OptionsPane::mouseReleased(Minecraft* minecraft, int mx, int my, int buttonNum) {
	_dragging = false;
	super::mouseReleased(minecraft, mx, my + (int)_scrollY, buttonNum);
}

void OptionsPane::tick(Minecraft* minecraft) {
	if (_dragging) {
		int mx = Mouse::getX();
		int my = Mouse::getY();
		if (minecraft->screen != NULL) {
			minecraft->screen->toGUICoordinate(mx, my);
		}
		float delta = (float)(_dragStartY - my);
		_scrollVelocity = (float)(_lastDragY - my);
		_lastDragY = my;
		_scrollY = _dragStartScroll + delta;
	} else if (_scrollVelocity > 0.5f || _scrollVelocity < -0.5f) {
		_scrollY += _scrollVelocity;
		_scrollVelocity *= 0.85f;
	}
	super::tick(minecraft);
}

OptionsGroup& OptionsPane::createOptionsGroup( std::string label ) {
	OptionsGroup* newGroup = new OptionsGroup(label);
	children.push_back(newGroup);
	return *newGroup;
}

void OptionsPane::createToggle( unsigned int group, std::string label, const Options::Option* option ) {
}

void OptionsPane::createProgressSlider( Minecraft* minecraft, unsigned int group, std::string label, const Options::Option* option, float progressMin/*=1.0f*/, float progressMax/*=1.0f */ ) {
}

void OptionsPane::createStepSlider( Minecraft* minecraft, unsigned int group, std::string label, const Options::Option* option, const std::vector<int>& stepVec ) {
}
