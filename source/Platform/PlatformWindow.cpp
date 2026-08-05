#include "stdafx.h"
#include "PlatformWindow.h"
#include "Resource.h"

namespace
{
    class Win32Window : public Platform::IWindow
    {
    public:
        Win32Window() : m_window(NULL), m_quitRequested(false) {}

        virtual Platform::NativeWindowHandle Create(const Platform::WindowConfig& config, Platform::WindowProcedure procedure)
        {
            WNDCLASSA windowClass;
            RECT rect;
            DWORD style;

            if (config.className == NULL || config.instance == NULL || procedure == NULL)
                return NULL;

            memset(&windowClass, 0, sizeof(windowClass));
            windowClass.style = CS_OWNDC | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
            windowClass.lpfnWndProc = reinterpret_cast<WNDPROC>(procedure);
            windowClass.hInstance = static_cast<HINSTANCE>(config.instance);
            windowClass.hIcon = LoadIconA(windowClass.hInstance, MAKEINTRESOURCEA(IDI_ICON1));
            windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
            windowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
            windowClass.lpszMenuName = NULL;
            windowClass.lpszClassName = config.className;
            RegisterClassA(&windowClass);

            if (config.windowed)
            {
                rect.left = 0;
                rect.top = 0;
                rect.right = static_cast<LONG>(config.width);
                rect.bottom = static_cast<LONG>(config.height);
                AdjustWindowRect(&rect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_BORDER | WS_CLIPCHILDREN, NULL);

                m_window = CreateWindowA(
                    config.className, config.className,
                    WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_BORDER | WS_CLIPCHILDREN,
                    (GetSystemMetrics(SM_CXSCREEN) - (rect.right - rect.left)) / 2,
                    (GetSystemMetrics(SM_CYSCREEN) - (rect.bottom - rect.top)) / 2,
                    rect.right - rect.left,
                    rect.bottom - rect.top,
                    NULL, NULL, windowClass.hInstance, NULL);
            }
            else
            {
                style = WS_POPUP;
                m_window = CreateWindowExA(WS_EX_TOPMOST | WS_EX_APPWINDOW,
                    config.className, config.className, style, 0, 0,
                    static_cast<int>(config.width), static_cast<int>(config.height),
                    NULL, NULL, windowClass.hInstance, NULL);
            }

            m_quitRequested = (m_window == NULL);
            return m_window;
        }

        virtual bool Show(int command)
        {
            if (m_window == NULL) return false;
            ::ShowWindow(m_window, command);
            ::UpdateWindow(m_window);
            ::SetForegroundWindow(m_window);
            ::SetFocus(m_window);
            return true;
        }

        virtual bool PumpEvents()
        {
            MSG message;
            if (!PeekMessage(&message, NULL, 0, 0, PM_NOREMOVE))
                return false;
            if (!GetMessage(&message, NULL, 0, 0))
            {
                m_quitRequested = true;
                return true;
            }
            TranslateMessage(&message);
            DispatchMessage(&message);
            return true;
        }

        virtual bool QuitRequested() const { return m_quitRequested; }
        virtual Platform::NativeWindowHandle GetNativeHandle() const { return m_window; }

    private:
        HWND m_window;
        bool m_quitRequested;
    };

    Win32Window g_Win32Window;
    Platform::IWindow* g_Window = &g_Win32Window;
}

Platform::IWindow& Platform::GetWindow() { return *g_Window; }
void Platform::SetWindow(IWindow* window) { g_Window = (window != NULL) ? window : &g_Win32Window; }
