#ifndef __PHONG_SHADER__
#define __PHONG_SHADER__
#include "vsdk/toolkit/render/shaders/LightingShader.h"
class PhongShader : public LightingShader { public: PhongShader() : LightingShader(true, false, false) {} };
#endif
