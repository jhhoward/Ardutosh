#include "Keyboard.h"
#include "Platform.h"
#include "Font.h"
#include "System.h"
#include "Generated/Sprites.h"
// \a = shift
// \v = sym/abc
// \b = backspace
// \r = return

const char keyboardLayout[] PROGMEM =
	"qwertyuiop\n"
	"asdfghjkl\n"
	"\azxcvbnm\b\n"
	"\v,     .\r";

const char keyboardLayoutUpper[] PROGMEM =
	"QWERTYUIOP\n"
	"ASDFGHJKL\n"
	"\aZXCVBNM\b\n"
	"\v,     .\r";

const char keyboardLayoutSym[] PROGMEM =
	"1234567890\n"
	"!\"#$%^&*()\n"
	"[];'#-+=_\b\n"
	"\v\\|    /\r";

const char* Keyboard::currentLayout = keyboardLayout;
uint8_t Keyboard::selectedKeyX;
uint8_t Keyboard::selectedKeyY;
bool Keyboard::keyboardVisible;
char Keyboard::lastKeyPressed;

int Keyboard::CountKeysOnRow(const char* c)
{
	int keys = 0;
	while (pgm_read_byte(c))
	{
		if (pgm_read_byte(c) == '\n')
			break;
		keys++;
		c++;
	}
	return keys;
}

int Keyboard::GetSpacebarWidth(const char* c)
{
	int count = 0;

	while (pgm_read_byte(c) == ' ')
	{
		count++;
		c++;
	}
	return count;
}

void Keyboard::DrawSelectionBox(int x, int y, int w, int h)
{
	static int animation = 0;
	animation++;
	int movement = (animation & 4) ? 0 : 1;

	for (int n = movement; n < w; n += 2)
	{
		Platform::DrawPixel(x + n, y, BLACK);
		Platform::DrawPixel(x + w - n, y + h, BLACK);
	}
	for(int n = movement; n < h; n += 2)
	{
		Platform::DrawPixel(x + w, y + n, BLACK);
		Platform::DrawPixel(x, y + h - n, BLACK);
	}
}

void Keyboard::Update()
{
	static uint8_t lastInput = Platform::GetInput();
	uint8_t input = Platform::GetInput();

	if ((input & INPUT_A) && !(lastInput & INPUT_A))
	{
		// Toggle keyboard
		keyboardVisible = !keyboardVisible;

		if (keyboardVisible)
		{
			currentLayout = keyboardLayout;
		}
		System::MarkScreenDirty();
	}

	if (keyboardVisible)
	{
		constexpr int columns = 10;
		constexpr int rows = 4;
		constexpr int keySpacing = 8;
		constexpr int paddingX = 2;
		constexpr int paddingY = 1;
		constexpr int keyboardWidth = columns * keySpacing + paddingX * 2;
		constexpr int keyboardHeight = rows * keySpacing + paddingY * 2;

		constexpr int keyboardX = DISPLAY_WIDTH / 2 - keyboardWidth / 2;
		constexpr int keyboardY = DISPLAY_HEIGHT - keyboardHeight;

		Platform::FillRect(keyboardX - paddingX, keyboardY - paddingY, keyboardWidth, keyboardHeight, WHITE);
		Platform::DrawRect(keyboardX - paddingX, keyboardY - paddingY, keyboardWidth, keyboardHeight, BLACK);

		int keysOnRow = CountKeysOnRow(currentLayout);
		int outX = keyboardX + (columns - keysOnRow) * keySpacing / 2;
		int outY = keyboardY;
		uint8_t keyX = 0, keyY = 0;

		char selectedChar = 0;
		uint8_t prevSelectedX = selectedKeyX;
		uint8_t prevSelectedY = selectedKeyY;

		if ((input & INPUT_LEFT) && !(lastInput & INPUT_LEFT) && selectedKeyX > 0)
		{
			selectedKeyX--;
		}
		if ((input & INPUT_RIGHT) && !(lastInput & INPUT_RIGHT))
		{
			selectedKeyX++;
		}
		if ((input & INPUT_UP) && !(lastInput & INPUT_UP) && selectedKeyY > 0)
		{
			selectedKeyY--;
		}
		if ((input & INPUT_DOWN) && !(lastInput & INPUT_DOWN) && selectedKeyY < rows - 1)
		{
			selectedKeyY++;
		}

		for (const char* ptr = currentLayout; pgm_read_byte(ptr); ptr++)
		{
			const char c = pgm_read_byte(ptr);
			if (selectedKeyY == keyY && selectedKeyX >= keysOnRow)
			{
				selectedKeyX = keysOnRow - 1;
			}
			switch (c)
			{
			case ' ':
			{
				int spacebarWidth = GetSpacebarWidth(ptr);

				if (selectedKeyY == keyY && selectedKeyX >= keyX && selectedKeyX < keyX + spacebarWidth)
				{
					// A bit hacky
					if (selectedKeyY != prevSelectedY || prevSelectedX < keyX || prevSelectedX >= keyX + spacebarWidth)
					{
						selectedKeyX = keyX + spacebarWidth / 2;
					}
					else if (selectedKeyX < prevSelectedX)
					{
						selectedKeyX = keyX - 1;
					}
					else if (selectedKeyX > prevSelectedX)
					{
						selectedKeyX = keyX + spacebarWidth;
					}

					DrawSelectionBox(outX, outY, spacebarWidth * keySpacing, keySpacing);
					selectedChar = ' ';
				}
				else
				{
					Platform::DrawRect(outX, outY, spacebarWidth * keySpacing, keySpacing, BLACK);
				}
				ptr += spacebarWidth - 1;
				keyX += spacebarWidth;
				outX += spacebarWidth * keySpacing;
			}
			continue;
			case '\n':
				keysOnRow = CountKeysOnRow(ptr + 1);
				outX = keyboardX + (columns - keysOnRow) * keySpacing / 2;
				outY += keySpacing;
				keyY++;
				keyX = 0;
				continue;
			case '\r':
				Platform::DrawSprite(outX, outY, returnIcon, 0);
				break;
			case '\a':
				Platform::DrawSprite(outX, outY, shiftIcon, 0);
				break;
			case '\b':
				Platform::DrawSprite(outX, outY, backspaceIcon, 0);
				break;
			case '\v':
				if (currentLayout == keyboardLayoutSym)
					Platform::DrawSprite(outX, outY, alphaIcon, 0);
				else
					Platform::DrawSprite(outX, outY, numbersIcon, 0);
				break;
			default:
				//Platform::FillRect(outX, outY, keySpacing, keySpacing, WHITE);
				Font::DrawChar(c, outX + paddingX, outY + paddingY, BLACK);
				break;
			}

			if (selectedKeyX == keyX && selectedKeyY == keyY)
			{
				DrawSelectionBox(outX, outY, keySpacing, keySpacing);
				selectedChar = c;
			}

			outX += keySpacing;
			keyX++;
		}

		if ((input & INPUT_B) && !(lastInput & INPUT_B))
		{
			switch (selectedChar)
			{
			case '\a':
				if (currentLayout == keyboardLayoutUpper)
					currentLayout = keyboardLayout;
				else
					currentLayout = keyboardLayoutUpper;
				break;
			case '\v':
				if (currentLayout == keyboardLayoutSym)
					currentLayout = keyboardLayout;
				else
					currentLayout = keyboardLayoutSym;
				break;
			case '\r':
				lastKeyPressed = '\n';
				System::HandleEvent(SystemEvent::KeyPressed);
				break;
			default:
				lastKeyPressed = selectedChar;
				System::HandleEvent(SystemEvent::KeyPressed);
				break;
			}
		}
	}

	lastInput = input;
}
