#pragma once

#include "PlatformTypes.h"

namespace Platform
{
    struct RenderContextConfig
    {
        RenderContextConfig()
            : window(NULL), colorBits(0), depthBits(0), doubleBuffered(false), enableShaderBackend(false) {}
        NativeWindowHandle window;
        unsigned int colorBits;
        unsigned int depthBits;
        bool doubleBuffered;
        bool enableShaderBackend;
    };

    class IRenderContext
    {
    public:
        virtual ~IRenderContext() {}

        virtual bool Create(const RenderContextConfig& config) = 0;
        virtual bool Destroy() = 0;
        virtual bool MakeCurrent() = 0;
        virtual bool Present() = 0;
        virtual bool IsValid() const = 0;
        virtual void* GetNativeDeviceContext() const = 0;
        virtual void* GetNativeRenderContext() const = 0;
        virtual unsigned long GetLastSystemError() const = 0;
    };

    IRenderContext& GetRenderContext();
    void SetRenderContext(IRenderContext* context);
}
