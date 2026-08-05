#include <atomic>
#include "PlatformInput.h"

namespace
{
    class AndroidInputBackend : public Platform::IInputBackend
    {
    public:
        AndroidInputBackend() : m_window(NULL), m_x(0), m_y(0), m_left(false) {}
        bool BindWindow(Platform::NativeWindowHandle window) override { m_window = window; return true; }
        void ReadPointer(Platform::PointerSnapshot& pointer) override
        {
            pointer.x = m_x.load();
            pointer.y = m_y.load();
            pointer.leftButtonDown = m_left.load();
            pointer.rightButtonDown = false;
            pointer.middleButtonDown = false;
        }
        unsigned int GetDoubleClickTimeMilliseconds() const override { return 500; }
        void SetPointer(long x, long y, bool pressed) { m_x = x; m_y = y; m_left = pressed; }
    private:
        Platform::NativeWindowHandle m_window;
        std::atomic<long> m_x;
        std::atomic<long> m_y;
        std::atomic<bool> m_left;
    };

    AndroidInputBackend g_backend;
}

namespace Platform
{
    IInputBackend& GetInputBackend() { return g_backend; }
    void SetInputBackend(IInputBackend*) {}
    void SetAndroidPointerState(long x, long y, bool pressed) { g_backend.SetPointer(x, y, pressed); }
}
