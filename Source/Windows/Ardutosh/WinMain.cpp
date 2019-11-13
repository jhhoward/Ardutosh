#include <SDL.h>
#include <stdio.h>
#include <sstream>
#include <iomanip>
#include <vector>
#include "Defines.h"
#include "System.h"
#include "Platform.h"
#include "lodepng.h"
#include <conio.h>

#define ZOOM_SCALE 1
#define TONES_END 0x8000

SDL_Window* AppWindow;
SDL_Renderer* AppRenderer;
SDL_Surface* ScreenSurface;
SDL_Texture* ScreenTexture;

uint8_t InputMask = 0;
uint8_t sBuffer[DISPLAY_WIDTH * DISPLAY_HEIGHT / 8];

bool isAudioEnabled = true;
bool IsRecording = false;
int CurrentRecordingFrame = 0;

struct KeyMap
{
	SDL_Scancode key;
	uint8_t mask;
};

std::vector<KeyMap> KeyMappings =
{
	{ SDL_SCANCODE_LEFT, INPUT_LEFT },
	{ SDL_SCANCODE_RIGHT, INPUT_RIGHT },
	{ SDL_SCANCODE_UP, INPUT_UP },
	{ SDL_SCANCODE_DOWN, INPUT_DOWN },
	{ SDL_SCANCODE_Z, INPUT_A },
	{ SDL_SCANCODE_X, INPUT_B },
};

constexpr int audioSampleRate = 48000;

const uint16_t* currentAudioPattern = nullptr;
int currentPatternBufferPos = 0;

void Play(const uint16_t* pattern)
{
	currentAudioPattern = pattern;
	currentPatternBufferPos = 0;
}

void swap(int16_t& a, int16_t& b)
{
	int16_t temp = a;
	a = b;
	b = temp;
}

void FillAudioBuffer(void *udata, uint8_t *stream, int len)
{
	int feedPos = 0;
	
	static int waveSamplesLeft = 0;
	static int noteSamplesLeft = 0;
	static int frequency = 0;
	static bool high = false;

	while(feedPos < len)
	{
		if(!isAudioEnabled)
		{
			while(feedPos < len)
			{
				stream[feedPos++] = 0;
			}
			return;
		}
		
		if(currentAudioPattern != nullptr)
		{
			if(noteSamplesLeft == 0)
			{
				frequency = currentAudioPattern[currentPatternBufferPos];
				uint16_t duration = currentAudioPattern[currentPatternBufferPos + 1];
				
				noteSamplesLeft = (audioSampleRate * duration) / 1000;
				
				waveSamplesLeft = frequency > 0 ? audioSampleRate / frequency : noteSamplesLeft;
				
				currentPatternBufferPos += 2;
				if(currentAudioPattern[currentPatternBufferPos] == TONES_END)
				{
					currentAudioPattern = nullptr;
				}
			}
		}
		
		if(frequency == 0)
		{
			while(feedPos < len && (!currentAudioPattern || noteSamplesLeft > 0))
			{
				stream[feedPos++] = 0;
				
				if(noteSamplesLeft > 0)
					noteSamplesLeft--;
			}
		}
		else
		{
			while(feedPos < len && waveSamplesLeft > 0 && noteSamplesLeft > 0)
			{
				int volume = 32;
				stream[feedPos++] = high ? 128 + volume : 128 - volume;
				waveSamplesLeft--;
				noteSamplesLeft--;
			}
			
			if(waveSamplesLeft == 0)
			{
				high = !high;
				waveSamplesLeft = audioSampleRate / frequency;
			}
		}
		
	}
}

void Platform::SetLED(uint8_t r, uint8_t g, uint8_t b)
{

}

void Platform::FillScreen(uint8_t colour)
{
	for (int y = 0; y < DISPLAY_HEIGHT; y++)
	{
		for (int x = 0; x < DISPLAY_WIDTH; x++)
		{
			DrawPixel(x, y, colour);
		}
	}
}

void Platform::DrawSprite(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t frame)
{
	uint8_t w = bitmap[0];
	uint8_t h = bitmap[1];

	bitmap += 2;

	for (int j = 0; j < h; j++)
	{
		for (int i = 0; i < w; i++)
		{
			int blockY = j / 8;
			int blockIndex = (w * blockY + i) * 2;
			uint8_t pixels = bitmap[blockIndex];
			uint8_t maskPixels = bitmap[blockIndex + 1];
			uint8_t bitmask = 1 << (j % 8);

			if (maskPixels & bitmask)
			{
				if (x + i >= 0 && y + j >= 0)
				{
					if (pixels & bitmask)
					{
						DrawPixel(x + i, y + j, 1);
					}
					else
					{
						DrawPixel(x + i, y + j, 0);
					}
				}
			}
		}
	}
}

void Platform::DrawFastVLine(int16_t x, int16_t y, uint8_t h, uint8_t color)
{
	for (uint8_t n = 0; n < h; n++)
	{
		DrawPixel(x, y + n, color);
	}
}

void Platform::DrawFastHLine(int16_t x, int16_t y, uint8_t w, uint8_t color)
{
	for (uint8_t n = 0; n < w; n++)
	{
		DrawPixel(x + n, y, color);
	}
}


void Platform::DrawPixel(int16_t x, int16_t y, uint8_t colour)
{
	if (x < 0 || y < 0 || x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT)
	{
		return;
	}

	uint16_t row_offset;
	uint8_t bit;

	bit = 1 << (y & 7);
	row_offset = (y & 0xF8) * DISPLAY_WIDTH / 8 + x;
	uint8_t data = sBuffer[row_offset] | bit;
	if (!colour) data ^= bit;
	sBuffer[row_offset] = data;
}

uint8_t GetPixel(uint8_t x, uint8_t y)
{
	uint8_t row = y / 8;
	uint8_t bit_position = y % 8;
	return (sBuffer[(row*DISPLAY_WIDTH) + x] & (1 << bit_position)) >> bit_position;
}

uint8_t* Platform::GetScreenBuffer()
{
	return sBuffer;
}

void ResolveScreen(SDL_Surface* surface)
{
	Uint32 black = SDL_MapRGBA(surface->format, 0, 0, 0, 255); 
	Uint32 white = SDL_MapRGBA(surface->format, 255, 255, 255, 255);

	int bpp = surface->format->BytesPerPixel;
	
	for(int y = 0; y < DISPLAY_HEIGHT; y++)
	{
		for(int x = 0; x < DISPLAY_WIDTH; x++)
		{
			Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
			
			*(Uint32 *)p = GetPixel(x, y) ? white : black;
		}
	}
}

void PutPixelImmediate(uint8_t x, uint8_t y, uint8_t colour)
{
	if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT)
	{
		return;
	}

	SDL_Surface* surface = ScreenSurface;

	Uint32 col = colour ? SDL_MapRGBA(surface->format, 255, 255, 255, 255) : SDL_MapRGBA(surface->format, 0, 0, 0, 255);

	int bpp = surface->format->BytesPerPixel;
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

	*(Uint32 *)p = col;
}

void DrawBitmapInternal(const uint8_t* data, uint16_t x, uint16_t y, uint8_t w, uint8_t h)
{
	for (int j = 0; j < h; j++)
	{
		for (int i = 0; i < w; i++)
		{
			int blockX = i / 8;
			int blockY = j / 8;
			int blocksPerWidth = w / 8;
			int blockIndex = blockY * blocksPerWidth + blockX;
			uint8_t pixels = data[blockIndex * 8 + i % 8];
			uint8_t mask = 1 << (j % 8);
			if (x + i >= 0 && y + j >= 0)
			{
				if (pixels & mask)
				{
					Platform::DrawPixel(x + i, y + j, 1);
				}
				else
				{
					Platform::DrawPixel(x + i, y + j, 0);
				}
			}
		}
	}
}

void Platform::DrawBitmap(int16_t x, int16_t y, const uint8_t *bitmap)
{
	DrawBitmapInternal(bitmap + 2, x, y, bitmap[0], bitmap[1]);
}

void Platform::PlaySound(const uint16_t* audioPattern)
{
	Play(audioPattern);
}

bool Platform::IsAudioEnabled()
{
	return isAudioEnabled;
}

void Platform::SetAudioEnabled(bool isEnabled)
{
	isAudioEnabled = isEnabled;
}

uint8_t Platform::GetInput()
{
	uint8_t inputMask = 0;

	const uint8_t* keyStates = SDL_GetKeyboardState(NULL);

	for (unsigned int n = 0; n < KeyMappings.size(); n++)
	{
		if (keyStates[KeyMappings[n].key])
		{
			inputMask |= KeyMappings[n].mask;
		}
	}

	return inputMask;
}

void Platform::DrawRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t colour)
{
	DrawFastHLine(x, y, w, colour);
	DrawFastHLine(x, y + h - 1, w, colour);
	DrawFastVLine(x, y, h, colour);
	DrawFastVLine(x + w - 1, y, h, colour);
}

void Platform::FillRect(int16_t x1, int16_t y1, uint8_t w, uint8_t h, uint8_t colour)
{
	int16_t x2 = x1 + w;
	int16_t y2 = y1 + h;
	if (x1 < 0)
		x1 = 0;
	if (x2 >= DISPLAY_WIDTH)
		x2 = DISPLAY_WIDTH;
	if (y1 < 0)
		y1 = 0;
	if (y2 >= DISPLAY_HEIGHT)
		y2 = DISPLAY_HEIGHT;

	for (int y = y1; y < y2; y++)
	{
		for (int x = x1; x < x2; x++)
		{
			DrawPixel((uint8_t)x, (uint8_t)y, colour);
		}
	}
}

void Platform::DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color)
{
	// bresenham's algorithm - thx wikpedia
	bool steep = abs(y1 - y0) > abs(x1 - x0);
	if (steep) {
		swap(x0, y0);
		swap(x1, y1);
	}

	if (x0 > x1) {
		swap(x0, x1);
		swap(y0, y1);
	}

	int16_t dx, dy;
	dx = x1 - x0;
	dy = abs(y1 - y0);

	int16_t err = dx / 2;
	int8_t ystep;

	if (y0 < y1)
	{
		ystep = 1;
	}
	else
	{
		ystep = -1;
	}

	for (; x0 <= x1; x0++)
	{
		if (steep)
		{
			DrawPixel((uint8_t)y0, (uint8_t)x0, color);
		}
		else
		{
			DrawPixel((uint8_t) x0, (uint8_t)y0, color);
		}

		err -= dy;
		if (err < 0)
		{
			y0 += ystep;
			err += dx;
		}
	}
}

//char testCommString[] = "test\n";
char testCommString[] = "This is some test data to test how the terminal processes and displays text.\nHere is another line of stuff.\nAnd another line.\nMore message to stress test the system and to ensure that auto scrolling is working correctly etc.";
unsigned int testCommPos = 0;
bool useConsoleInput = false;
bool commWaiting = true;

void PlatformComm::SetBaud(uint32_t rate)
{
	testCommPos = 0;
}

bool PlatformComm::IsAvailable()
{
	if (useConsoleInput)
	{
		return _kbhit() != 0;
	}
	return commWaiting && testCommPos < strlen(testCommString);
}

void PlatformComm::Write(uint8_t data)
{
	putchar(data);
}

uint8_t PlatformComm::Read()
{
	if (useConsoleInput)
	{
		return _getch();
	}

	if (testCommPos < strlen(testCommString))
	{
		commWaiting = false;
		return testCommString[testCommPos++];
	}
	return 0;
}

uint8_t PlatformStorage::GetByte(uint16_t address)
{
	if (address < strlen(testCommString))
	{
		return testCommString[address];
	}
	return 0;
}

void PlatformStorage::SetByte(uint16_t address, uint8_t value)
{
	if (address < strlen(testCommString))
	{
		testCommString[address] = value;
	}
}

uint16_t Platform::GetBatteryVoltage()
{
	return 4299 - (rand() % 100);
}

int16_t Platform::GetTemperature()
{
	return 20 + (rand() % 5);
}

bool remoteKeyboardEnabled = false;
void PlatformRemote::SetKeyboardEnabled(bool enabled)
{
	remoteKeyboardEnabled = enabled;
}
bool PlatformRemote::IsKeyboardEnabled()
{
	return remoteKeyboardEnabled;
}

bool remoteMouseEnabled = false;
void PlatformRemote::SetMouseEnabled(bool enabled)
{
	remoteMouseEnabled = enabled;
}
bool PlatformRemote::IsMouseEnabled()
{
	return remoteMouseEnabled;
}


void PlatformRemote::KeyboardWrite(uint8_t data)
{
}

void PlatformRemote::MouseMove(int dirX, int dirY)
{
}

void PlatformRemote::MouseDown()
{
}

void PlatformRemote::MouseUp()
{
}

bool remoteGamepadEnabled = false;
void PlatformRemote::SetGamepadEnabled(bool enabled)
{
	remoteGamepadEnabled = enabled;
}
bool PlatformRemote::IsGamepadEnabled()
{
	return remoteGamepadEnabled;
}

void Platform::Reboot()
{
	// stub
}

void DebugDisplayNow()
{
	ResolveScreen(ScreenSurface);
	SDL_UpdateTexture(ScreenTexture, NULL, ScreenSurface->pixels, ScreenSurface->pitch);
	SDL_Rect src, dest;
	src.x = src.y = dest.x = dest.y = 0;
	src.w = DISPLAY_WIDTH;
	src.h = DISPLAY_HEIGHT;
	dest.w = DISPLAY_WIDTH;
	dest.h = DISPLAY_HEIGHT;
	SDL_RenderCopy(AppRenderer, ScreenTexture, &src, &dest);
	SDL_RenderPresent(AppRenderer);

//	SDL_Delay(1000 / TARGET_FRAMERATE);
	//SDL_Delay(1);
}

int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_CreateWindowAndRenderer(DISPLAY_WIDTH * ZOOM_SCALE, DISPLAY_HEIGHT * ZOOM_SCALE, SDL_WINDOW_RESIZABLE, &AppWindow, &AppRenderer);
	SDL_RenderSetLogicalSize(AppRenderer, DISPLAY_WIDTH, DISPLAY_HEIGHT);

	ScreenSurface = SDL_CreateRGBSurface(0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 32,
		0x000000ff,
		0x0000ff00,
		0x00ff0000,
		0xff000000
	);
	ScreenTexture = SDL_CreateTexture(AppRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, ScreenSurface->w, ScreenSurface->h);

	SDL_SetWindowPosition(AppWindow, 1900 - DISPLAY_WIDTH * 2, 1020 - DISPLAY_HEIGHT);

	SDL_AudioSpec wanted;
	wanted.freq = audioSampleRate;
	wanted.format = AUDIO_U8;
	wanted.channels = 1;
	wanted.samples = 4096;
	wanted.callback = FillAudioBuffer;

	if (SDL_OpenAudio(&wanted, NULL) <0) {
		printf("Error: %s\n", SDL_GetError());
	}
	SDL_PauseAudio(0);
	
	bool running = true;
	int playRate = 1;
	static int testAudio = 0;

	System::Init();

	while (running)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_QUIT:
				running = false;
				break;
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym)
				{
				case SDLK_ESCAPE:
					running = false;
					break;
				case SDLK_F12:
					{
						lodepng::encode(std::string("screenshot.png"), (unsigned char*)(ScreenSurface->pixels), ScreenSurface->w, ScreenSurface->h);
					}
					break;
				case SDLK_F11:
					IsRecording = !IsRecording;
					break;
				}
				break;
			case SDL_KEYUP:
				if (event.key.keysym.sym == SDLK_TAB)
					playRate = 1;
				break;
			}
		}

		SDL_SetRenderDrawColor(AppRenderer, 206, 221, 231, 255);
		SDL_RenderClear(AppRenderer);

		for (int n = 0; n < playRate; n++)
		{
			memset(ScreenSurface->pixels, 0, ScreenSurface->format->BytesPerPixel * ScreenSurface->w * ScreenSurface->h);
			
			System::Tick();
			System::Draw();
			
			ResolveScreen(ScreenSurface);
		}

		commWaiting = true;

		if (IsRecording)
		{
			std::ostringstream filename;
			filename << "Frame";
			filename << std::setfill('0') << std::setw(5) << CurrentRecordingFrame << ".png";

			lodepng::encode(filename.str(), (unsigned char*)(ScreenSurface->pixels), ScreenSurface->w, ScreenSurface->h);
			CurrentRecordingFrame++;
		}

		SDL_UpdateTexture(ScreenTexture, NULL, ScreenSurface->pixels, ScreenSurface->pitch);
		SDL_Rect src, dest;
		src.x = src.y = dest.x = dest.y = 0;
		src.w = DISPLAY_WIDTH;
		src.h = DISPLAY_HEIGHT;
		dest.w = DISPLAY_WIDTH;
		dest.h = DISPLAY_HEIGHT;
		SDL_RenderCopy(AppRenderer, ScreenTexture, &src, &dest);
		SDL_RenderPresent(AppRenderer);

		SDL_Delay(1000 / TARGET_FRAMERATE);
	}

	return 0;
}
