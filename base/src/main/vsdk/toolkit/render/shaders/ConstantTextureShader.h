#ifndef __CONSTANT_TEXTURE_SHADER__
#define __CONSTANT_TEXTURE_SHADER__
#include "vsdk/toolkit/render/shaders/Shader.h"
class ConstantTextureShader : public Shader {
public:
    virtual LocalShadingResult shadeLocal(RayHit* info,double,double,double,java::ArrayList<Light*>&,java::ArrayList<SimpleBody*>&,SimpleMaterial* material,TraceWorkspace*) override;
};
#endif
