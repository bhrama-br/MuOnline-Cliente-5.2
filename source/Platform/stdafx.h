#pragma once

#ifdef _MSC_VER
#include "../stdafx.h"
#else
#include <GLES3/gl3.h>
#ifndef __EMSCRIPTEN__
#include <EGL/egl.h>
#endif
#include <cstddef>
#include <cstring>
#include <vector>
#endif
