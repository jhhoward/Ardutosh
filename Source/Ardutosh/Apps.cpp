#include "Apps.h"
#include "WindowManager.h"
#include "Font.h"
#include "Platform.h"
#include "System.h"
#include "MenuBar.h"
#include "VirtualKeyboard.h"
#include "Input.h"
#include "Generated/Sprites.h"

const char ReadmeContents[] PROGMEM = "Welcome to the Arduboy desktop environment!\nWritten by James Howard for the Arduboy game jam 4 where the theme was 'not a game'.\nThe graphical style is based on the classic Macintosh interface.";

#define USE_GAMEPAD_REMOTE 0

struct GridView
{
	GridView(Window* inWindow, int inStartX = 14, int inStartY = 10, int inSpacingX = 30, int inSpacingY = 22) 
		: x(inStartX), y(inStartY), window(inWindow), startX(inStartX), startY(inStartY), spacingX(inSpacingX), spacingY(inSpacingY)
	{
	}

	int x, y;

	void Next()
	{
		x += spacingX;
		if (x >= window->w)
		{
			x = startX;
			y += spacingY;
		}
	}

private:
	Window* window;
	int startX, startY;
	int spacingX, spacingY;
};

Window* Apps::OpenTerminalApp()
{
	Window* win = WindowManager::FindByHandler(TerminalApp);

	if (win)
	{
		WindowManager::Focus(win);
		return nullptr;
	}

	win = WindowManager::Create(WindowType::FullWindow, TerminalApp);
	if (win)
	{
		win->title = FlashString("Terminal");
		win->x = 0;
		win->y = 8;
		win->w = 128;
		win->h = 54;
		return win;
		//win->OpenWithAnimation(itemsX + 5, itemsY + 5);
	}

	return nullptr;
}

void Apps::SetBaudRateDialog(Window* window, SystemEvent eventType)
{
	constexpr int dialogWidth = 96;
	constexpr int dialogHeight = 48;
	constexpr int numOptions = 4;
	constexpr int selectionSpacing = 7;
	constexpr int defaultBaudRateSelection = 1;
	static const int bauds[numOptions] = { 4800, 9600, 14400, 19200 };
	static uint8_t selection = defaultBaudRateSelection;

	window->w = dialogWidth;
	window->h = dialogHeight;
	window->x = DISPLAY_WIDTH / 2 - dialogWidth / 2;
	window->y = MenuBar::height;

	window->Label(FlashString("Baud rate:"), 6, 6);

	int selectionX = 6;
	int selectionY = 14;
	for (int n = 0; n < numOptions; n++)
	{
		window->RadioButton(selectionX, selectionY, 32, n, selection);
		window->Label(bauds[n], selectionX + 12, selectionY);
		selectionY += selectionSpacing;
	}

	if (window->Button(FlashString("Default"), window->w - 36, window->h - 27))
	{
		selection = defaultBaudRateSelection;
		PlatformComm::SetBaud(bauds[selection]);
	}
	if (window->Button(FlashString("  OK  "), window->w - 36, window->h - 16))
	{
		PlatformComm::SetBaud(bauds[selection]);
		WindowManager::Destroy(window);
	}
}

void Apps::TerminalApp(Window* window, SystemEvent eventType)
{
	constexpr int maxVisibleLines = 6;
	constexpr int bufferSize = 256;
	static char buffer[bufferSize];
	static int bufferPos = 0; 
	static uint16_t scrollPosition = 0;
	xString str(buffer);
	int textAreaWidth = window->w - 15;
	int textAreaHeight = window->h - 12;
	int numColumns = textAreaWidth / Font::glyphWidth;
	int numRows = textAreaHeight / (Font::glyphHeight + 1);
	int numLinesInBuffer = str.NumLines(numColumns);
	uint16_t maxScroll = 0;

	if (window->MenuItem(Menu_Edit_Clear))
	{
		for (int n = 0; n < bufferSize; n++)
			buffer[n] = '\0';
		bufferPos = 0;
	}
	if (window->MenuItem(Menu_File_DumpEEPROM))
	{
		for (uint16_t address = 0; address < 1024; address++)
		{
			PlatformComm::Write(PlatformStorage::GetByte(address));
		}
	}
	if (window->MenuItem(Menu_Edit_SetBaud))
	{
		WindowManager::Create(WindowType::DialogBox, SetBaudRateDialog);
	}

	if (numRows < numLinesInBuffer)
	{
		maxScroll = numLinesInBuffer - numRows;
	}

	window->VerticalScrollBar(scrollPosition, maxScroll);

	if (eventType == SystemEvent::KeyPressed)
	{
		PlatformComm::Write(VirtualKeyboard::GetLastKeyPressed());
	}
	else if (eventType == SystemEvent::Repaint)
	{
		Font::DrawStringWindowed(str.SubstringAtLine(scrollPosition, numColumns), window->x + 2, window->y + 11, window->w - 15, window->h - 12, BLACK);
		if (window->IsFocused())
		{
			VirtualKeyboard::SetCursorScreenLocation(Font::GetCursorX(), Font::GetCursorY());
		}
		Font::DrawCaret(BLACK);
	}
	else if (eventType == SystemEvent::Tick)
	{
		bool receivedData = false;

		while (PlatformComm::IsAvailable())
		{
			char input = PlatformComm::Read();

			if (input == '\b' || input == 0x7f)
			{
				// Backspace
				if (bufferPos > 0)
				{
					buffer[--bufferPos] = '\0';
				}
			}
			else if (input == '\0')
			{
				// Null character
				buffer[bufferPos++] = ' ';
			}
			else if (bufferPos < bufferSize - 2)
			{
				buffer[bufferPos++] = input;

				/*
				For debugging uncomment this for HEX code output
				int upper = ((uint8_t)(input)) >> 4;
				int lower = ((uint8_t)(input)) & 0xf;
				if (upper < 0xa)
					buffer[bufferPos++] = '0' + upper;
				else
					buffer[bufferPos++] = 'A' + upper - 0xa;

				if (lower < 0xa)
					buffer[bufferPos++] = '0' + lower;
				else
					buffer[bufferPos++] = 'A' + lower - 0xa;
					*/
			}
			else
			{
				int cutoff = str.GetLineEndIndex(0, numColumns);
				bufferPos -= cutoff;
				for (int n = 0; n <= bufferPos; n++)
				{
					buffer[n] = buffer[n + cutoff];
				}
				buffer[bufferPos++] = input;

				for (int n = bufferPos; n < bufferSize; n++)
					buffer[n] = '\0';
			}

			receivedData = true;
		}

		if (receivedData)
		{
			// auto scroll if we are at the bottom
			bool atBottom = maxScroll == 0 || scrollPosition == maxScroll;
			if (atBottom)
			{
				numLinesInBuffer = str.NumLines(numColumns);
				if (numRows < numLinesInBuffer)
				{
					maxScroll = numLinesInBuffer - numRows;
				}
				scrollPosition = maxScroll;
			}
			System::MarkScreenDirty();
		}
	}
}

Window* Apps::OpenFinderApp()
{
	Window* win = WindowManager::FindByHandler(FinderApp);

	if (win)
	{
		WindowManager::Focus(win);
		return nullptr;
	}

	win = WindowManager::Create(WindowType::FullWindow, FinderApp);
	if (win)
	{
		win->title = FlashString("Arduboy");
		win->x = 0;
		win->y = 8;
		win->w = 128;
		win->h = 54;
		return win;
	}

	return nullptr;
}

typedef Window*(*FinderItemHandler)();

struct FinderItem
{
	const uint8_t* icon;
	const char label[9];
	FinderItemHandler handler;

	const uint8_t* GetIcon() const { return (uint8_t*)pgm_read_ptr(&icon); }
	FinderItemHandler GetHandler() const { return (FinderItemHandler) pgm_read_ptr(&handler);  }
};

const static FinderItem finderItems[] PROGMEM =
{
	{ terminalIcon,		"Terminal",		Apps::OpenTerminalApp },
	{ eepromIcon,		"EEPROM",		Apps::OpenEEPROMInspectorApp },
	{ batteryIcon,		"Battery",		Apps::OpenBatteryApp },
	{ temperatureIcon,	"Thermal",		Apps::OpenTemperatureApp },
	{ ledIcon,			"LED",			Apps::OpenLEDApp },
	{ remoteIcon,		"Remote",		Apps::OpenRemoteApp },
	{ documentIcon,		"Readme",		Apps::OpenReadme },
};

void Apps::FinderApp(Window* window, SystemEvent eventType)
{
	GridView grid(window);

	for(const FinderItem& item : finderItems)
	{
		if (window->Item(item.GetIcon(), xString(item.label, xString::Type::Flash), grid.x, grid.y))
		{
			if (Window* win = item.GetHandler()())
			{
				win->OpenWithAnimation(grid.x + 5, grid.y + 5);
			}
		}
		grid.Next();
	}
}

Window* Apps::OpenReadme()
{
	return OpenTextReader(FlashString("Readme"), ReadmeContents);
}

Window* Apps::OpenTextReader(const xString& title, const xString& contents)
{
	Window* win = WindowManager::FindByData((void*)contents.GetData());

	if (win)
	{
		WindowManager::Focus(win);
		return nullptr;
	}

	win = WindowManager::Create(WindowType::FullWindow, TextReaderApp);
	if (win)
	{
		win->title = title;
		win->data = (void*) contents.GetData();
		win->x = 12;
		win->y = 9;
		win->w = 86;
		win->h = 54;
		return win;
	}

	return nullptr;
}

void Apps::TextReaderApp(Window* window, SystemEvent eventType)
{
	xString str((const char*)window->data, xString::Type::Flash);
	static uint16_t currentScroll = 0;
	int textAreaWidth = window->w - 12;
	int textAreaHeight = window->h - 12;
	int numRows = textAreaHeight / (Font::glyphHeight + 1);
	int numColumns = textAreaWidth / Font::glyphWidth;
	int numLinesInText = str.NumLines(numColumns);
	uint16_t maxScroll = 0;

	if (numRows > maxScroll)
	{
		maxScroll = numLinesInText - numRows;
	}

	window->VerticalScrollBar(currentScroll, maxScroll);
	if (eventType == SystemEvent::Repaint)
	{
		Font::DrawStringWindowed(str.SubstringAtLine(currentScroll, numColumns), window->x + 2, window->y + 11, textAreaWidth, textAreaHeight, BLACK);
	}
}

Window* Apps::OpenEEPROMInspectorApp()
{
	Window* win = WindowManager::FindByHandler(EEPROMInspectorApp);

	if (win)
	{
		WindowManager::Focus(win);
		return nullptr;
	}

	win = WindowManager::Create(WindowType::FullWindow, EEPROMInspectorApp);
	if (win)
	{
		win->title = FlashString("EEPROM");
		win->x = 0;
		win->y = 8;
		win->w = 128;
		win->h = 54;
		return win;
		//win->OpenWithAnimation(itemsX + 5, itemsY + 5);
	}

	return nullptr;
}

void Apps::EEPROMInspectorApp(Window* window, SystemEvent eventType)
{
	constexpr int visibleLines = 6;
	constexpr int columnsPerLine = 8;
	static uint16_t scrollLocation = 0;
	constexpr uint16_t maxAddress = 1024;
	constexpr uint16_t maxScrollLocation = (maxAddress - visibleLines * columnsPerLine) / columnsPerLine;
	int outY = window->y + 10;
	static uint16_t cursorLocation = -1;
	bool cursorOverLowNibble = (cursorLocation & 1) != 0;

	window->VerticalScrollBar(scrollLocation, maxScrollLocation);

	if (window->IsFocused())
	{
		VirtualKeyboard::SetHexInputLayout();
	}

	if (eventType == SystemEvent::Repaint)
	{
		uint16_t address = scrollLocation * columnsPerLine;

		for (int n = 0; n < visibleLines; n++)
		{
			int outX = window->x + 2;
			Font::DrawHexInt(address, outX, outY, BLACK);
			outX += Font::glyphWidth * 5;

			for (int i = 0; i < 8; i++)
			{
				bool cursorOverAddress = (cursorLocation >> 1) == address;

				uint8_t val = PlatformStorage::GetByte(address++);

				Font::SetCursor(outX, outY);

				if (cursorOverAddress)
				{
					if (cursorOverLowNibble)
					{
						Platform::FillRect(outX + Font::glyphWidth, outY - 1, Font::glyphWidth, Font::glyphHeight + 1, BLACK);
					}
					else
					{
						Platform::FillRect(outX - 1, outY - 1, Font::glyphWidth, Font::glyphHeight + 1, BLACK);
					}

					VirtualKeyboard::SetCursorScreenLocation(outX, outY);
				}
				Font::DrawHexNibble(val >> 4, cursorOverAddress && !cursorOverLowNibble ? WHITE : BLACK);
				Font::DrawHexNibble(val, cursorOverAddress && cursorOverLowNibble ? WHITE : BLACK);


//				Font::DrawHexByte(val, outX, outY, BLACK);			


				outX += Font::glyphWidth * 3;
			}

			outY += Font::glyphHeight + 1;
		}

		Platform::DrawFastVLine(window->x + 20, window->y + 9, window->h - 9, BLACK);

//		Font::DrawStringWindowed(xString((const char*)window->data, xString::Type::Flash), window->x + 2, window->y + 11, window->w - 4, window->h - 12, BLACK);
	}
	if (eventType == SystemEvent::MouseDown)
	{
		int clickedX = mouse.x - window->x;
		int clickedY = mouse.y - window->y;
		constexpr int columnLeft = Font::glyphWidth * 5;
		constexpr int columnTop = 10;
		constexpr int columnSpacing = Font::glyphWidth * 3;
		constexpr int rowSpacing = Font::glyphHeight + 1;

		if (clickedX > columnLeft && clickedX < columnLeft + columnsPerLine * columnSpacing && clickedY >= columnTop && clickedY < columnTop + rowSpacing * visibleLines)
		{
			int column = (clickedX - columnLeft) / columnSpacing;
			int row = (clickedY - columnTop) / rowSpacing;
			
			int selectedColumnX = column * columnSpacing + columnLeft;
			bool selectedLowerNibble = clickedX > selectedColumnX + columnSpacing / 2;

			uint16_t address = scrollLocation * columnsPerLine + (row * columnsPerLine) + column;
			cursorLocation = (address * 2) + (selectedLowerNibble ? 1 : 0);
			System::MarkScreenDirty();
		}
	}
	else if (eventType == SystemEvent::KeyPressed)
	{
		char c = VirtualKeyboard::GetLastKeyPressed();
		bool validInput = false;
		uint8_t inputValue = 0;
		if (c >= '0' && c <= '9')
		{
			inputValue = c - '0';
			validInput = true;
		}
		else if (c >= 'a' && c <= 'f')
		{
			inputValue = c - 'a' + 0xa;
			validInput = true;
		}
		else if (c >= 'A' && c <= 'F')
		{
			inputValue = c - 'A' + 0xa;
			validInput = true;
		}

		if (validInput)
		{
			uint8_t newValue = PlatformStorage::GetByte(cursorLocation >> 1);

			if (cursorOverLowNibble)
			{
				newValue = (newValue & 0xf0) | inputValue;
			}
			else
			{
				newValue = (newValue & 0xf) | (inputValue << 4);
			}

			PlatformStorage::SetByte(cursorLocation >> 1, newValue);

			if (cursorLocation < maxAddress * 2)
			{
				cursorLocation++;

				while ((cursorLocation >> 1) >= (scrollLocation + visibleLines) * columnsPerLine)
				{
					scrollLocation++;
				}
				while ((cursorLocation >> 1) < scrollLocation * columnsPerLine)
				{
					scrollLocation--;
				}
			}
			System::MarkScreenDirty();
		}
	}
}


Window* Apps::OpenBatteryApp()
{
	Window* win = WindowManager::FindByHandler(BatteryApp);

	if (win)
	{
		WindowManager::Focus(win);
		return nullptr;
	}

	win = WindowManager::Create(WindowType::FullWindow, BatteryApp);
	if (win)
	{
		win->title = FlashString("Battery");
		win->x = 0;
		win->y = 8;
		win->w = 85;
		win->h = 54;
		return win;
	}

	return nullptr;
}

void Apps::BatteryApp(Window* window, SystemEvent eventType)
{
	constexpr int refreshRate = 60;
	static uint8_t timer = 0;
	constexpr int bufferSize = 16;
	static uint16_t buffer[bufferSize];
	constexpr uint16_t minVal = 200;
	constexpr uint16_t maxVal = 500;

	if (eventType == SystemEvent::Repaint)
	{
		int graphX = window->x + 10;
		int graphY = window->y + 10;
		int graphW = window->w - 13;
		int graphH = window->h - 21;
		constexpr uint16_t range = maxVal - minVal;

		Platform::DrawRect(graphX, graphY, graphW + 1, graphH + 1, BLACK);
		for (int n = 0; n < bufferSize - 1; n++)
		{
			int outX = graphX + (graphW * n) / (bufferSize - 1);
			int outY = graphY + graphH - ((buffer[n] / 10 - minVal) * graphH) / range;
			int nextX = graphX + (graphW * (n + 1)) / (bufferSize - 1);
			int nextY = graphY + graphH - ((buffer[n + 1] / 10 - minVal) * graphH) / range;

			if (buffer[n] == 0)
			{
				outY = graphY + graphH;
			}
			if (buffer[n + 1] == 0)
			{
				nextY = graphY + graphH;
			}
			Platform::DrawLine(outX, outY, nextX, nextY, BLACK);
		}

		constexpr int numLabels = 4;
		for (int n = 0; n < numLabels; n++)
		{
			int outY = graphY + graphH - (n * graphH) / (numLabels - 1);
			int labelValue = minVal + ((n * range) / (numLabels - 1));
			Font::DrawInt(labelValue / 100, window->x + 2, outY, BLACK);
		}

		uint16_t voltage = buffer[bufferSize - 1];
		int labelX = window->x + window->w / 2 - Font::glyphWidth * 6;
		int labelY = window->y + window->h - Font::glyphHeight - 2;
		Font::DrawString(FlashString("Voltage: "), labelX, labelY, BLACK);
		Font::DrawInt(voltage / 1000, BLACK);
		Font::DrawChar('.', BLACK);
		Font::DrawInt(voltage % 1000, BLACK);
		Font::DrawChar('V', BLACK);
	}
	else if (eventType == SystemEvent::Tick)
	{
		if (timer == 0)
		{
			uint16_t voltage = Platform::GetBatteryVoltage();
			for (int n = 0; n < bufferSize - 1; n++)
			{
				buffer[n] = buffer[n + 1];
			}
			if (voltage < minVal * 10)
				voltage = minVal * 10;
			if (voltage > maxVal * 10)
				voltage = maxVal * 10;
			buffer[bufferSize - 1] = voltage;

			timer = refreshRate;
			System::MarkScreenDirty();
		}
		else timer--;
	}

	if (window->MenuItem(Menu_Edit_Clear))
	{
		for (int n = 0; n < bufferSize; n++)
			buffer[n] = 0;
	}
}

Window* Apps::OpenTemperatureApp()
{
	Window* win = WindowManager::FindByHandler(TemperatureApp);

	if (win)
	{
		WindowManager::Focus(win);
		return nullptr;
	}

	win = WindowManager::Create(WindowType::FullWindow, TemperatureApp);
	if (win)
	{
		win->title = FlashString("Temperature");
		win->x = 0;
		win->y = 8;
		win->w = 85;
		win->h = 54;
		return win;
	}

	return nullptr;
}

void Apps::TemperatureApp(Window* window, SystemEvent eventType)
{
	constexpr int refreshRate = 60;
	static uint8_t timer = 0;
	constexpr int bufferSize = 16;
	static int16_t buffer[bufferSize];
	constexpr int16_t minVal = 0;
	constexpr int16_t maxVal = 50;

	if (eventType == SystemEvent::Repaint)
	{
		int graphX = window->x + 10;
		int graphY = window->y + 10;
		int graphW = window->w - 13;
		int graphH = window->h - 21;
		constexpr int16_t range = maxVal - minVal;

		Platform::DrawRect(graphX, graphY, graphW + 1, graphH + 1, BLACK);
		for (int n = 0; n < bufferSize - 1; n++)
		{
			int outX = graphX + (graphW * n) / (bufferSize - 1);
			int outY = graphY + graphH - ((buffer[n] - minVal) * graphH) / range;
			int nextX = graphX + (graphW * (n + 1)) / (bufferSize - 1);
			int nextY = graphY + graphH - ((buffer[n + 1] - minVal) * graphH) / range;

			if (buffer[n] == 0)
			{
				outY = graphY + graphH;
			}
			if (buffer[n + 1] == 0)
			{
				nextY = graphY + graphH;
			}
			Platform::DrawLine(outX, outY, nextX, nextY, BLACK);
		}

		constexpr int numLabels = 4;
		for (int n = 0; n < numLabels; n++)
		{
			int outY = graphY + graphH - (n * graphH) / (numLabels - 1);
			int labelValue = minVal + ((n * range) / (numLabels - 1));
			Font::DrawInt(labelValue, window->x + 2, outY, BLACK);
		}

		int labelX = window->x + window->w / 2 - Font::glyphWidth * 6;
		int labelY = window->y + window->h - Font::glyphHeight - 2;
		Font::DrawString(FlashString("Temperature: "), labelX, labelY, BLACK);
		Font::DrawInt(buffer[bufferSize - 1], BLACK);
		Font::DrawChar('C', BLACK);
	}
	else if (eventType == SystemEvent::Tick)
	{
		if (timer == 0)
		{
			for (int n = 0; n < bufferSize - 1; n++)
			{
				buffer[n] = buffer[n + 1];
			}
			int16_t sample = Platform::GetTemperature();
			if (sample < minVal)
				sample = minVal;
			if (sample > maxVal)
				sample = maxVal;
			buffer[bufferSize - 1] = sample;

			timer = refreshRate;
			System::MarkScreenDirty();
		}
		else timer--;
	}

	if (window->MenuItem(Menu_Edit_Clear))
	{
		for (int n = 0; n < bufferSize; n++)
			buffer[n] = 0;
	}
}

Window* Apps::OpenLEDApp()
{
	Window* win = WindowManager::FindByHandler(LEDApp);

	if (win)
	{
		WindowManager::Focus(win);
		return nullptr;
	}

	win = WindowManager::Create(WindowType::FullWindow, LEDApp);
	if (win)
	{
		win->title = FlashString("LED");
		win->x = 8;
		win->y = 10;
		win->w = 74;
		win->h = 52;
		return win;
	}

	return nullptr;
}

void Apps::LEDApp(Window* window, SystemEvent eventType)
{
	constexpr int sliderX = 26;
	constexpr int sliderSpacing = 9;
	int sliderWidth = window->w - sliderX - 5;
	static uint8_t red, green, blue;

	int outY = 12;
	window->Label(FlashString("Red"), 2, outY);
	window->Slider(sliderX, outY, sliderWidth, red);
	outY += sliderSpacing;

	window->Label(FlashString("Green"), 2, outY);
	window->Slider(sliderX, outY, sliderWidth, green);
	outY += sliderSpacing;

	window->Label(FlashString("Blue"), 2, outY);
	window->Slider(sliderX, outY, sliderWidth, blue);
	outY += sliderSpacing;

	if (window->Button(FlashString("Off"), window->w / 3 - 10, outY))
	{
		red = green = blue = 0;
	}
	if (window->Button(FlashString("All on"), 2 * window->w / 3 - 15, outY))
	{
		red = green = blue = 255;
	}

	if (eventType == SystemEvent::Tick)
	{
		Platform::SetLED(red, green, blue);
	}
}

Window* Apps::OpenRemoteApp()
{
	Window* win = WindowManager::FindByHandler(RemoteApp);

	if (win)
	{
		WindowManager::Focus(win);
		return nullptr;
	}

	win = WindowManager::Create(WindowType::FullWindow, RemoteApp);
	if (win)
	{
		win->title = FlashString("Remote");
		win->w = 100;

#if USE_GAMEPAD_REMOTE
		win->h = 46;
#else
		win->h = 44;
#endif		
		win->x = DISPLAY_WIDTH / 2 - win->w / 2;
		win->y = MenuBar::height;
		return win;
	}

	return nullptr;
}

void Apps::RemoteApp(Window* window, SystemEvent eventType)
{
	if (PlatformRemote::IsKeyboardEnabled())
	{
		window->Label(FlashString("Remote keyboard enabled"), 5, 12);
		if (!VirtualKeyboard::IsVisible())
		{
			PlatformRemote::SetKeyboardEnabled(false);
		}
	}
	else if (PlatformRemote::IsMouseEnabled())
	{
		window->Label(FlashString("Remote mouse enabled"), 5, 12);
		window->Label(FlashString("Press A to cancel"), 5, 19);
	}
#if USE_GAMEPAD_REMOTE
	else if (PlatformRemote::IsGamepadEnabled())
	{
		window->Label(FlashString("Remote mouse enabled"), 5, 12);
		window->Label(FlashString("Hold A+B to cancel"), 5, 19);
	}
#endif
	else
	{
		if (window->Button(FlashString("Enable remote keyboard"), 3, 10))
		{
			window->x = DISPLAY_WIDTH / 2 - window->w / 2;
			window->y = MenuBar::height;
			PlatformRemote::SetKeyboardEnabled(true);
			VirtualKeyboard::Show();
		}
		if (window->Button(FlashString("Enable remote mouse"), 3, 22))
		{
			PlatformRemote::SetMouseEnabled(true);
		}
		window->Label(FlashString("Mouse speed"), 3, 34);
		window->Slider(50, 35, window->w - 53, mouse.remoteMouseSpeed, 1, 10);
#if USE_GAMEPAD_REMOTE
		if (window->Button(FlashString("Enable remote gamepad"), 3, 34))
		{
			PlatformRemote::SetGamepadEnabled(true);
		}
#endif
	}

	if (eventType == SystemEvent::CloseWindow)
	{
		PlatformRemote::SetKeyboardEnabled(false);
		PlatformRemote::SetMouseEnabled(false);
		PlatformRemote::SetGamepadEnabled(false);
	}
	else if (eventType == SystemEvent::KeyPressed)
	{
		if (PlatformRemote::IsKeyboardEnabled())
		{
			PlatformRemote::KeyboardWrite(VirtualKeyboard::GetLastKeyPressed());
		}
	}
	else if (eventType == SystemEvent::Tick)
	{
#if USE_GAMEPAD_REMOTE
		if (PlatformRemote::IsGamepadEnabled())
		{
			static uint8_t timer = 0;
			constexpr uint8_t timeout = 128;
			uint8_t input = Platform::GetInput();
			if ((input & (INPUT_A | INPUT_B)) == (INPUT_A | INPUT_B))
			{
				timer++;
				if (timer == timeout)
				{
					PlatformRemote::SetGamepadEnabled(false);
					System::MarkScreenDirty();
				}
			}
			else
			{
				timer = 0;
			}
		}
#endif
	}
}
