#ifndef __COOK_TORRANCE_SHADER__
#define __COOK_TORRANCE_SHADER__

#include "vsdk/toolkit/render/shaders/Shader.h"
class CookTorranceShader : public Shader {
private:
    bool textureEnabled;
    bool bumpMapEnabled;
public:
    CookTorranceShader(bool textureEnabled, bool bumpMapEnabled);
    virtual LocalShadingResult shadeLocal(RayHit* info,double viewX,double viewY,double viewZ,java::ArrayList<Light*>& lights,java::ArrayList<SimpleBody*>& objects,SimpleMaterial* material,TraceWorkspace* workspace) override;
};

#endif
