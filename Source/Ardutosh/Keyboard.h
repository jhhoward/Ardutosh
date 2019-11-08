#pragma once

#include <stdint.h>

class Keyboard
{
public:
	static void Update();
	static bool IsVisible() { return keyboardVisible; }

	static char GetLastKeyPressed() { return lastKeyPressed;  }
private:
	static void DrawSelectionBox(int x, int y, int w, int h);
	static int CountKeysOnRow(const char* c);
	static int GetSpacebarWidth(const char* c);

	static const char* currentLayout;
	static uint8_t selectedKeyX, selectedKeyY;
	static char lastKeyPressed;
	static bool keyboardVisible;
};