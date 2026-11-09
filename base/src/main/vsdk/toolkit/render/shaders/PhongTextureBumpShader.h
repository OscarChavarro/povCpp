#ifndef __PHONG_TEXTURE_BUMP_SHADER__
#define __PHONG_TEXTURE_BUMP_SHADER__
#include "vsdk/toolkit/render/shaders/LightingShader.h"
class PhongTextureBumpShader : public LightingShader { public: PhongTextureBumpShader() : LightingShader(true, true, true) {} };
#endif
