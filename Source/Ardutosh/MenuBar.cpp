#include "MenuBar.h"
#include "WindowManager.h"
#include "Font.h"

struct MenuItem
{
	const char* title;
	MenuBarMask mask;
};

const MenuItem menuItems[] PROGMEM =
{
	{ "File" },
		{ "Dump EEPROM", Menu_File_DumpEEPROM },
		{ "Close", Menu_File_Close },

	{ "Edit" },
		{ "Clear", Menu_Edit_Clear },
		{ "Set baud", Menu_Edit_SetBaud },

	{ nullptr}
};

void MenuBar::Draw()
{
	if (WindowManager::GetDesktop())
	{
		Font::DrawString("@ File Edit Special", 10, 1, BLACK);
	}
}

bool MenuBar::HandleEvent(SystemEvent eventType)
{
	if (eventType == SystemEvent::Repaint)
	{
		Draw();
	}
	return false;
}
