#pragma once

#include "WindowManager.h"
#include "System.h"
#include "Platform.h"
#include "Defines.h"
#include "Input.h"

void WindowManager::Init()
{
	for (int n = 0; n < maxWindows; n++)
	{
		drawOrder[n] = invalidWindowHandle;
	}
}

void WindowManager::Draw()
{
	for (int n = maxWindows - 1; n >= 0; n--)
	{
		if (drawOrder[n] == invalidWindowHandle)
			continue;
		Window& win = windows[drawOrder[n]];

		bool isWindowAnimating = (System::state.currentState == System::State::OpeningWindowAnimation
			|| System::state.currentState == System::State::ClosingWindowAnimation)
			&& System::state.stateElement.window == win.GetHandle();

		if (!isWindowAnimating)
		{
			win.Draw();
		}
	}
}

void WindowManager::Destroy(Window* window)
{
	int drawPosition = GetDrawPosition(window->GetHandle());
	if (drawPosition != invalidWindowHandle)
	{
		for (int n = drawPosition; n < maxWindows; n++)
		{
			if (n == maxWindows - 1)
			{
				drawOrder[n] = invalidWindowHandle;
			}
			else
			{
				drawOrder[n] = drawOrder[n + 1];
			}
		}
	}

	window->type = WindowType::Closed;
	System::MarkScreenDirty();
}

Window* WindowManager::Create(WindowType type, WindowHandler handler)
{
	for (int n = 0; n < maxWindows; n++)
	{
		if (windows[n].type == WindowType::Closed)
		{
			Window* newWindow = &windows[n];
			newWindow->type = type;
			newWindow->handler = handler;

			for (int i = 0; i < maxWindows; i++)
			{
				if (drawOrder[i] == invalidWindowHandle)
				{
					drawOrder[i] = n;
					break;
				}
			}
			Focus(newWindow);

			newWindow->HandleEvent(SystemEvent::OpenWindow);

			return newWindow;
		}
	}
	return nullptr;
}

Window* WindowManager::GetWindow(int16_t x, int16_t y)
{
	for (int n = 0; n < maxWindows; n++)
	{
		if (drawOrder[n] == invalidWindowHandle)
			break;
		Window& win = windows[drawOrder[n]];
		if (x >= win.x && y >= win.y && x < win.x + win.w && y < win.y + win.h)
		{
			return &win;
		}

		if (win.type == WindowType::Desktop)
			return &win;
	}

	return nullptr;
}

Window* WindowManager::GetWindow(WindowHandle handle)
{
	if (handle < maxWindows && windows[handle].type != WindowType::Closed)
		return &windows[handle];
	return nullptr;
}

void WindowManager::DrawAnimations()
{
	if (System::state.currentState == System::State::OpeningWindowAnimation)
	{
		if (System::state.animationTimer >= numAnimationFrames)
		{
			System::ExitState(System::State::OpeningWindowAnimation);
			System::MarkScreenDirty();
		}
		else
		{
			Window* window = GetWindow(System::state.stateElement.window);
			if (window)
			{
				for (int lerp = 1; lerp <= System::state.animationTimer; lerp++)
				{
					int oneMinusLerp = (numAnimationFrames - lerp);

					int x1 = (window->x * lerp + oneMinusLerp * window->originX) / numAnimationFrames;
					int y1 = (window->y * lerp + oneMinusLerp * window->originY) / numAnimationFrames;
					int w1 = (window->w * lerp) / numAnimationFrames;
					int h1 = (window->h * lerp) / numAnimationFrames;

					Platform::DrawRect(x1, y1, w1, h1, BLACK);
				}
			}
			else
			{
				System::ExitState(System::State::OpeningWindowAnimation);
			}
		}
	}
}

bool WindowManager::HandleEvent(SystemEvent eventType)
{
	if (eventType == SystemEvent::Tick)
	{
		for (int n = 0; n < maxWindows; n++)
		{
			if (windows[n].type != WindowType::Closed)
			{
				windows[n].HandleEvent(eventType);
			}
		}

		DrawAnimations();
	}
	else if (eventType == SystemEvent::Repaint)
	{
		Draw();
		DrawAnimations();
	}
	else if (eventType == SystemEvent::MouseDown)
	{
		Window* window = GetWindow(mouse.x, mouse.y);
		if (window)
		{
			Focus(window);

			window->HandleEvent(eventType);
			return true;
		}
	}
	else
	{
		if (System::state.currentState != System::State::Default)
		{
			if (System::state.stateElement.window != invalidWindowHandle)
			{
				Window* window = &windows[System::state.stateElement.window];
				window->HandleEvent(eventType);
			}
		}
	}

	return false;
}

int WindowManager::GetDrawPosition(WindowHandle handle)
{
	for (int n = 0; n < maxWindows; n++)
	{
		if (drawOrder[n] == invalidWindowHandle)
			break;
		if (drawOrder[n] == handle)
		{
			return n;
		}
	}

	return invalidWindowHandle;
}

void WindowManager::Focus(Window* windowObj)
{
	if (windowObj->type == WindowType::Desktop)
		return;

	WindowHandle window = windowObj->GetHandle();
	int currentPosition = GetDrawPosition(window);

	if (currentPosition == 0)
		return;

	for (int n = currentPosition; n > 0; n--)
	{
		drawOrder[n] = drawOrder[n - 1];
	}

	drawOrder[0] = window;

	System::MarkScreenDirty();
}

Window* WindowManager::FindByData(void* data)
{
	for (uint8_t n = 0; n < maxWindows; n++)
	{
		if (windows[n].type != WindowType::Closed && windows[n].data == data)
		{
			return &windows[n];
		}
	}

	return nullptr;
}

Window* WindowManager::FindByHandler(WindowHandler handler)
{
	for (uint8_t n = 0; n < maxWindows; n++)
	{
		if (windows[n].type != WindowType::Closed && windows[n].handler == handler)
		{
			return &windows[n];
		}
	}

	return nullptr;
}

Window* WindowManager::GetDesktop()
{
	for (uint8_t n = 0; n < maxWindows; n++)
	{
		if (windows[n].type == WindowType::Desktop)
		{
			return &windows[n];
		}
	}

	return nullptr;
}
