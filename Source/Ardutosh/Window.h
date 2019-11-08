#pragma once
#include <stdint.h>
#include "StringWrapper.h"
#include "SystemEvent.h"
#include "Element.h"

typedef void(*WindowHandler)(class Window* window, SystemEvent event);
typedef uint8_t WindowHandle;

enum class WindowType : uint8_t
{
	Closed,
	Desktop,
	DialogBox,
	FullWindow
};

class Window
{
public:
	void Draw();

	bool Button(xString label, int16_t x, int16_t y);
	bool Item(const uint8_t* icon, const xString& label, int16_t x, int16_t y);
	void VerticalScrollBar(uint16_t& current, uint16_t max);
	void Slider(int16_t x, int16_t y, uint8_t w, uint8_t& current);
	void Label(const xString& label, int16_t x, int16_t y);
	void Label(int16_t label, int16_t x, int16_t y);
	bool RadioButton(int16_t x, int16_t y, uint8_t w, uint8_t index, uint8_t& selected);

	void OpenWithAnimation(uint8_t fromX, uint8_t fromY);
	void HandleEvent(SystemEvent eventType);

	WindowHandle GetHandle() const;

	WindowType type;
	void* data;
	WindowHandler handler;
	xString title;
	int x, y;
	uint8_t w, h;
	uint8_t originX, originY;

	uint16_t menuItemMask;

private:
	bool IsMouseOverCloseButton() const;
	Element GetCurrentElement() const;
};

