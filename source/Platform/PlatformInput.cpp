#include "stdafx.h"
#include "PlatformInput.h"

namespace
{
    class Win32InputBackend : public Platform::IInputBackend
    {
    public:
        Win32InputBackend() : m_window(NULL) {}

        virtual bool BindWindow(Platform::NativeWindowHandle window)
        {
            m_window = static_cast<HWND>(window);
            return m_window != NULL;
        }

        virtual void ReadPointer(Platform::PointerSnapshot& pointer)
        {
            POINT cursor;
            cursor.x = 0;
            cursor.y = 0;

            if (::GetCursorPos(&cursor) && m_window != NULL)
            {
                ::ScreenToClient(m_window, &cursor);
            }

            pointer.x = cursor.x;
            pointer.y = cursor.y;
            pointer.leftButtonDown = (::GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
            pointer.rightButtonDown = (::GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
            pointer.middleButtonDown = (::GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
        }

        virtual unsigned int GetDoubleClickTimeMilliseconds() const
        {
            return static_cast<unsigned int>(::GetDoubleClickTime());
        }

    private:
        HWND m_window;
    };

    Win32InputBackend g_Win32InputBackend;
    Platform::IInputBackend* g_InputBackend = &g_Win32InputBackend;
}

Platform::IInputBackend& Platform::GetInputBackend()
{
    return *g_InputBackend;
}

void Platform::SetInputBackend(IInputBackend* backend)
{
    g_InputBackend = (backend != NULL) ? backend : &g_Win32InputBackend;
}
