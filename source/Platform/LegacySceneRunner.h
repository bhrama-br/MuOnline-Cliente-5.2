#pragma once

#include "PlatformTypes.h"

namespace Platform
{
    // Windows forwards this seam to the original RenderScene.
    void RenderLegacySceneFrame(NativeWindowHandle renderTarget);
}
