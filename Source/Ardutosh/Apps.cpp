#include "Apps.h"
#include "WindowManager.h"
#include "Font.h"
#include "Platform.h"
#include "System.h"
#include "Generated/Sprites.h"

const char ReadmeContents[] PROGMEM = "Welcome to the Arduboy desktop environment. Written by James Howard for the Arduboy game jam 4: theme 'not a game'.";

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

	if (numRows < numLinesInBuffer)
	{
		maxScroll = numLinesInBuffer - numRows;
	}

	window->VerticalScrollBar(scrollPosition, maxScroll);

	if (eventType == SystemEvent::Repaint)
	{
		Font::DrawStringWindowed(str.SubstringAtLine(scrollPosition, numColumns), window->x + 2, window->y + 11, window->w - 15, window->h - 12, BLACK);
		Font::DrawCaret(BLACK);
	}
	else if (eventType == SystemEvent::Tick)
	{
		if (PlatformComm::IsAvailable())
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

				for(int n = bufferPos; n < bufferSize; n++)
					buffer[n] = '\0';
			}

			bool atBottom = maxScroll == 0 || scrollPosition == maxScroll;
			if (atBottom)
			{
				int newNumLinesInBuffer = str.NumLines(numColumns);
				maxScroll = newNumLinesInBuffer - numRows;
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
		win->x = 1;
		win->y = 8;
		win->w = 94;
		win->h = 54;
		return win;
	}

	return nullptr;
}

void Apps::FinderApp(Window* window, SystemEvent eventType)
{
	const int itemsStartX = window->x + 14;
	const int itemsStartY = window->y + 10;
	constexpr int itemSpacingX = 30;
	constexpr int itemSpacingY = 22;
	int itemX = itemsStartX;
	int itemY = itemsStartY;

	if (window->Item(terminalIcon, FlashString("Terminal"), itemX, itemY))
	{
		if (Window* win = OpenTerminalApp())
		{
			win->OpenWithAnimation(itemX + 5, itemY + 5);
		}
	}

	itemX += itemSpacingX;
	if (window->Item(eepromIcon, FlashString("EEPROM"), itemX, itemY))
	{
		if (Window* win = OpenEEPROMInspectorApp())
		{
			win->OpenWithAnimation(itemX + 5, itemY + 5);
		}
	}

	itemX += itemSpacingX;
	if (window->Item(batteryIcon, FlashString("Battery"), itemX, itemY))
	{
		if (Window* win = OpenBatteryApp())
		{
			win->OpenWithAnimation(itemX + 5, itemY + 5);
		}
	}

	itemX = itemsStartX;
	itemY += itemSpacingY;
	if (window->Item(temperatureIcon, FlashString("Thermal"), itemX, itemY))
	{
		if (Window* win = OpenTemperatureApp())
		{
			win->OpenWithAnimation(itemX + 5, itemY + 5);
		}
	}

	itemX += itemSpacingX;
	if (window->Item(ledIcon, FlashString("LED"), itemX, itemY))
	{
		if (Window* win = OpenLEDApp())
		{
			win->OpenWithAnimation(itemX + 5, itemY + 5);
		}
	}

	itemX += itemSpacingX;
	if (window->Item(documentIcon, FlashString("Readme"), itemX, itemY))
	{
		if (Window* win = OpenTextReader("Readme", ReadmeContents))
		{
			win->OpenWithAnimation(itemX + 5, itemY + 5);
		}
	}
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
		win->y = 12;
		win->w = 100;
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

	window->VerticalScrollBar(scrollLocation, maxScrollLocation);

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
				uint8_t val = PlatformStorage::GetByte(address++);
				Font::DrawHexByte(val, outX, outY, BLACK);
				outX += Font::glyphWidth * 3;
			}

			outY += Font::glyphHeight + 1;
		}

		Platform::DrawFastVLine(window->x + 20, window->y + 9, window->h - 9, BLACK);

//		Font::DrawStringWindowed(xString((const char*)window->data, xString::Type::Flash), window->x + 2, window->y + 11, window->w - 4, window->h - 12, BLACK);
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

	if (eventType == SystemEvent::Repaint)
	{
		int graphX = window->x + 10;
		int graphY = window->y + 10;
		int graphW = window->w - 13;
		int graphH = window->h - 21;
		constexpr uint16_t minVal = 200;
		constexpr uint16_t maxVal = 500;
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
			buffer[bufferSize - 1] = voltage;

			timer = refreshRate;
			System::MarkScreenDirty();
		}
		else timer--;
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

	if (eventType == SystemEvent::Repaint)
	{
		int graphX = window->x + 10;
		int graphY = window->y + 10;
		int graphW = window->w - 13;
		int graphH = window->h - 21;
		constexpr int16_t minVal = 0;
		constexpr int16_t maxVal = 50;
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
			buffer[bufferSize - 1] = Platform::GetTemperature();

			timer = refreshRate;
			System::MarkScreenDirty();
		}
		else timer--;
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

	if (window->Button(FlashString("Off"), window->w / 3 - 10, outY - 8))
	{
		red = green = blue = 0;
	}
	if (window->Button(FlashString("All on"), 2 * window->w / 3 - 15, outY - 8))
	{
		red = green = blue = 255;
	}

	if (eventType == SystemEvent::Tick)
	{
		Platform::SetLED(red, green, blue);
	}
}
