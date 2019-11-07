#pragma once

#include "SystemEvent.h"
#include <stdint.h>

typedef uint16_t MenuBarMask;

enum
{
	Menu_File_Close = 2,
	Menu_File_DumpEEPROM = 4,

	Menu_Edit_Clear = 32,
	Menu_Edit_SetBaud = 64,
};

class MenuBar
{
public:
	static bool HandleEvent(SystemEvent eventType);

private:
	static void Draw();
};
