#ifndef __FLAT_TEXTURED_SHADER__
#define __FLAT_TEXTURED_SHADER__
#include "vsdk/toolkit/render/shaders/LightingShader.h"
class FlatTexturedShader : public LightingShader { public: FlatTexturedShader() : LightingShader(true, true, false) {} };
#endif
