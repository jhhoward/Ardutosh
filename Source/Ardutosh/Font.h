#pragma once

#include <stdint.h>
#include "Defines.h"
#include "StringWrapper.h"

class Font
{
public:
	static constexpr int glyphWidth = 4;
	static constexpr int glyphHeight = 6;
	static constexpr int firstGlyphIndex = 32;

	static void DrawString(const xString& str, int16_t x, int16_t y, uint8_t colour);
	static void DrawStringWindowed(const xString& str, int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t colour);
	static void DrawInt(int16_t val, int16_t x, int16_t y, uint8_t colour);
	static void DrawHexByte(uint8_t val, int16_t x, int16_t y, uint8_t colour);
	static void DrawHexInt(uint16_t val, int16_t x, int16_t y, uint8_t colour);
	static void DrawCaret(uint8_t colour);

	static void DrawInt(int16_t val, uint8_t colour);
	static void DrawChar(char c, uint8_t colour);
	static void DrawHexNibble(uint8_t val, uint8_t colour);

private:
	static int16_t cursorX, cursorY;
};
