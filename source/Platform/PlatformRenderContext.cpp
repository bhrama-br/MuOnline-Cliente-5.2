#include "stdafx.h"
#include "PlatformRenderContext.h"
#include "LegacyRenderAdapter.h"

namespace
{
    class Win32RenderContext : public Platform::IRenderContext
    {
    public:
        Win32RenderContext()
            : m_window(NULL), m_deviceContext(NULL), m_renderContext(NULL), m_lastSystemError(0)
        {
        }

        virtual bool Create(const Platform::RenderContextConfig& config)
        {
            PIXELFORMATDESCRIPTOR pixelFormatDescriptor;
            UINT pixelFormat;

            if (config.window == NULL || IsValid())
                return false;

            m_window = static_cast<HWND>(config.window);
            m_deviceContext = ::GetDC(m_window);
            if (m_deviceContext == NULL)
                return SaveLastError();

            memset(&pixelFormatDescriptor, 0, sizeof(pixelFormatDescriptor));
            pixelFormatDescriptor.nSize = sizeof(pixelFormatDescriptor);
            pixelFormatDescriptor.nVersion = 1;
            pixelFormatDescriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
            if (config.doubleBuffered)
                pixelFormatDescriptor.dwFlags |= PFD_DOUBLEBUFFER;
            pixelFormatDescriptor.iPixelType = PFD_TYPE_RGBA;
            pixelFormatDescriptor.cColorBits = static_cast<BYTE>(config.colorBits);
            pixelFormatDescriptor.cDepthBits = static_cast<BYTE>(config.depthBits);

            pixelFormat = ::ChoosePixelFormat(m_deviceContext, &pixelFormatDescriptor);
            if (pixelFormat == 0)
                return SaveLastError();

            if (!::SetPixelFormat(m_deviceContext, pixelFormat, &pixelFormatDescriptor))
                return SaveLastError();

            m_renderContext = ::wglCreateContext(m_deviceContext);
            if (m_renderContext == NULL)
                return SaveLastError();

            if (!MakeCurrent())
                return false;
            if (config.enableShaderBackend)
                Platform::EnableGlslLegacyBackend(true);
            return true;
        }

        virtual bool Destroy()
        {
            bool success = true;

            if (m_renderContext != NULL)
            {
				// O adaptador GLSL mantem VAO/VBO/programa do contexto atual.
				// Esquece esses nomes enquanto o contexto ainda esta valido para
				// que uma recriacao nao reutilize objetos que pertenciam ao
				// contexto destruido.
				Platform::InvalidateLegacyRenderResources();
                if (!::wglMakeCurrent(NULL, NULL))
                    success = SaveLastError();
                if (!::wglDeleteContext(m_renderContext))
                    success = SaveLastError();
                m_renderContext = NULL;
            }

            if (m_deviceContext != NULL)
            {
                if (!::ReleaseDC(m_window, m_deviceContext))
                    success = SaveLastError();
                m_deviceContext = NULL;
            }

            m_window = NULL;
            return success;
        }

        virtual bool MakeCurrent()
        {
            if (m_deviceContext == NULL || m_renderContext == NULL)
                return false;
            if (!::wglMakeCurrent(m_deviceContext, m_renderContext))
                return SaveLastError();
            return true;
        }

        virtual bool Present()
        {
            if (m_deviceContext == NULL || !::SwapBuffers(m_deviceContext))
                return SaveLastError();
            return true;
        }

        virtual bool IsValid() const
        {
            return m_deviceContext != NULL && m_renderContext != NULL;
        }

        virtual void* GetNativeDeviceContext() const
        {
            return m_deviceContext;
        }

        virtual void* GetNativeRenderContext() const
        {
            return m_renderContext;
        }

        virtual unsigned long GetLastSystemError() const
        {
            return m_lastSystemError;
        }

    private:
        bool SaveLastError()
        {
            m_lastSystemError = static_cast<unsigned long>(::GetLastError());
            return false;
        }

        HWND m_window;
        HDC m_deviceContext;
        HGLRC m_renderContext;
        unsigned long m_lastSystemError;
    };

    Win32RenderContext g_Win32RenderContext;
    Platform::IRenderContext* g_RenderContext = &g_Win32RenderContext;
}

Platform::IRenderContext& Platform::GetRenderContext()
{
    return *g_RenderContext;
}

void Platform::SetRenderContext(IRenderContext* context)
{
    g_RenderContext = (context != NULL) ? context : &g_Win32RenderContext;
}
