#include "stdafx.h"
#include "LegacySceneRunner.h"
#include "ZzzScene.h"

void Platform::RenderLegacySceneFrame(NativeWindowHandle renderTarget)
{
    RenderScene(static_cast<HDC>(renderTarget));
}
