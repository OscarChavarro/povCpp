#ifndef __GOURAUD_TEXTURE_SHADER__
#define __GOURAUD_TEXTURE_SHADER__
#include "vsdk/toolkit/render/shaders/LightingShader.h"
class GouraudTextureShader : public LightingShader { public: explicit GouraudTextureShader(bool textureEnabled) : LightingShader(true, textureEnabled, false) {} };
#endif
