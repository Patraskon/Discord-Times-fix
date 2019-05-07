#define CINTERFACE

#include <ddraw.h>

#include "Common/Hook.h"
#include "DDraw/ActivateAppHandler.h"
#include "Gdi/Gdi.h"
#include "Win32/DisplayMode.h"
#include "Win32/FontSmoothing.h"

namespace
{
	WNDPROC g_origDdWndProc = nullptr;
	bool g_windowed = false;

	void activateApp()
	{
		Gdi::enableEmulation();
	}

	void deactivateApp()
	{
		Gdi::disableEmulation();
	}

	LRESULT CALLBACK ddWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		static bool isDisplayChangeNotificationEnabled = true;
		static int redrawCount = 0;

		switch (uMsg)
		{
			//Workaround for invisible menu on Load/Save/Delete in Tiberian Sun
			case WM_PARENTNOTIFY:
			{
				if (LOWORD(wParam) == WM_DESTROY)
					redrawCount = 2;

				break;
			}
			case WM_PAINT:
			{
				if (redrawCount > 0)
				{
					redrawCount--;
					RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
				}
				break;
			}

			case WM_SIZE:
			case WM_MOVE:
			{
				RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
				break;
			}

			case WM_ACTIVATE:
			{	
				RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);

				if (g_windowed)
				{
					wParam = WA_ACTIVE;
				}
				else
				{
					if (wParam == WA_INACTIVE)
						ShowWindow(hwnd, SW_MINIMIZE);
				}
				break;
			}

			case WM_ACTIVATEAPP:
			{
				if (g_windowed)
				{
					wParam = TRUE;
					lParam = 0;
				}
				else
				{
					isDisplayChangeNotificationEnabled = false;
					if (TRUE == wParam)
					{
						activateApp();
					}
					else
					{
						deactivateApp();
					}
					LRESULT result = g_origDdWndProc(hwnd, uMsg, wParam, lParam);
					isDisplayChangeNotificationEnabled = true;
					return result;
				}
			}

			case WM_DISPLAYCHANGE:
			{
				// Fix for alt-tabbing in Commandos 2
				if (!g_windowed && !isDisplayChangeNotificationEnabled)
				{
					return 0;
				}
				break;
			}

		}

		return g_origDdWndProc(hwnd, uMsg, wParam, lParam);
	}
}

namespace DDraw
{
	namespace ActivateAppHandler
	{
		void setCooperativeLevel(HWND hwnd, DWORD flags)
		{
			static bool isDdWndProcHooked = false;
			g_windowed = !(flags & DDSCL_FULLSCREEN);
			if (!isDdWndProcHooked)
			{
				g_origDdWndProc = reinterpret_cast<WNDPROC>(GetWindowLongPtr(hwnd, GWLP_WNDPROC));
				Compat::hookFunction(reinterpret_cast<void*&>(g_origDdWndProc), ddWndProc);
				isDdWndProcHooked = true;
			}
		}
	}
}
