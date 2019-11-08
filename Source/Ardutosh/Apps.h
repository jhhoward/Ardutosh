#pragma once

#include "SystemEvent.h"

class Window;
class xString;

class Apps
{
public:
	static Window* OpenTerminalApp();
	static Window* OpenFinderApp();
	static Window* OpenEEPROMInspectorApp();
	static Window* OpenBatteryApp();
	static Window* OpenTemperatureApp();
	static Window* OpenLEDApp();
	static Window* OpenTextReader(const xString& title, const xString& contents);

private:
	static void TerminalApp(Window* window, SystemEvent eventType);
	static void FinderApp(Window* window, SystemEvent eventType);
	static void TextReaderApp(Window* window, SystemEvent eventType);
	static void BatteryApp(Window* window, SystemEvent eventType);
	static void TemperatureApp(Window* window, SystemEvent eventType);
	static void LEDApp(Window* window, SystemEvent eventType);
	static void EEPROMInspectorApp(Window* window, SystemEvent eventType);

	static void SetBaudRateDialog(Window* window, SystemEvent eventType);
};
