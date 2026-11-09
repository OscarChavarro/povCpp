#ifndef __PHONG_BUMP_SHADER__
#define __PHONG_BUMP_SHADER__
#include "vsdk/toolkit/render/shaders/LightingShader.h"
class PhongBumpShader : public LightingShader { public: PhongBumpShader() : LightingShader(true, false, true) {} };
#endif
