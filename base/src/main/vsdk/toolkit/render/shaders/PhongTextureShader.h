#ifndef __PHONG_TEXTURE_SHADER__
#define __PHONG_TEXTURE_SHADER__
#include "vsdk/toolkit/render/shaders/LightingShader.h"
class PhongTextureShader : public LightingShader { public: PhongTextureShader() : LightingShader(true, true, false) {} };
#endif
