#pragma once

#include "PlatformTypes.h"

namespace Platform
{
    typedef void* WindowProcedure;

    struct WindowConfig
    {
        const char* className;
        NativeWindowHandle instance;
        unsigned int width;
        unsigned int height;
        bool windowed;
    };

    class IWindow
    {
    public:
        virtual ~IWindow() {}
        virtual NativeWindowHandle Create(const WindowConfig& config, WindowProcedure procedure) = 0;
        virtual bool Show(int command) = 0;
        virtual bool PumpEvents() = 0;
        virtual bool QuitRequested() const = 0;
        virtual NativeWindowHandle GetNativeHandle() const = 0;
    };

    IWindow& GetWindow();
    void SetWindow(IWindow* window);
}
