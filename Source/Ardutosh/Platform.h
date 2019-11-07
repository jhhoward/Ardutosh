#pragma once

#include <stdint.h>

class Platform
{
public:
	static uint8_t GetInput(void);
	static void SetLED(uint8_t r, uint8_t g, uint8_t b);
	static uint8_t* GetScreenBuffer(); 

	static void PlaySound(const uint16_t* audioPattern);
	static bool IsAudioEnabled();
	static void SetAudioEnabled(bool isEnabled);

	static uint16_t GetBatteryVoltage();
	static int16_t GetTemperature();

	static void FillScreen(uint8_t col);
	static void FillRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t color);
	static void DrawRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t color);
	static void DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color);
	static void DrawFastVLine(int16_t x, int16_t y, uint8_t h, uint8_t color);
	static void DrawFastHLine(int16_t x, int16_t y, uint8_t w, uint8_t color);
	static void DrawPixel(int16_t x, int16_t y, uint8_t colour);
	static void DrawBitmap(int16_t x, int16_t y, const uint8_t *bitmap);
	static void DrawSprite(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t frame);	
};

class PlatformComm
{
public:
	static void SetBaud(uint16_t rate);
	static bool IsAvailable();
	static void Write(uint8_t data);
	static uint8_t Read();
};

class PlatformStorage
{
public:
	static uint8_t GetByte(uint16_t address);
};
