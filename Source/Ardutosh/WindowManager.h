#pragma once

#include "Window.h"

class WindowManager
{
public:
	static void Init();
	static void Draw();

	static bool HandleEvent(SystemEvent eventType);

	static Window* GetWindow(WindowHandle handle);
	static Window* Create(WindowType type, WindowHandler handler);
	static void Destroy(Window* window);
	static Window* GetWindow(int16_t x, int16_t y);
	static void Focus(Window* window);

	static uint16_t GetMenuBarItemMask();

	static Window* FindByData(void* data);
	static Window* FindByHandler(WindowHandler handler);
	static Window* GetDesktop();

	static bool ShowingDialog();

	static constexpr int maxWindows = 8;
	static Window windows[maxWindows];
	static WindowHandle drawOrder[maxWindows];

	static constexpr WindowHandle invalidWindowHandle = 0xff;
	static constexpr int numAnimationFrames = 8;

	static uint8_t currentElementIndex;

private:
	friend class Window;

	static void DrawAnimations();
	static int GetDrawPosition(WindowHandle window);

	static uint16_t menuBarItemMask;

};

