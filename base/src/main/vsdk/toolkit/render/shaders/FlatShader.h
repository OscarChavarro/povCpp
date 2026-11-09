#ifndef __FLAT_SHADER__
#define __FLAT_SHADER__
#include "vsdk/toolkit/render/shaders/LightingShader.h"
class FlatShader : public LightingShader { public: FlatShader() : LightingShader(true, false, false) {} };
#endif
