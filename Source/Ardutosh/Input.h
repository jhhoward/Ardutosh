#pragma once

#include <stdint.h>

struct Mouse
{
	union
	{
		struct
		{
			uint8_t fracX;
			uint8_t x;
		};
		uint16_t fixedX;
	};

	union
	{
		struct
		{
			uint8_t fracY;
			uint8_t y;
		};
		uint16_t fixedY;
	};

	int16_t velX, velY;
	uint8_t lastInput;
	int16_t deltaX, deltaY;

	void Tick();
	void RestoreBackgroundPixels();
	void Draw();

	static constexpr int cursorWidth = 8;
	static constexpr int cursorHeight = 12;
	uint8_t backgroundPixels[24];
};

extern Mouse mouse;
