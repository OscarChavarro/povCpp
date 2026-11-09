#ifndef __LIGHTING_SHADER__
#define __LIGHTING_SHADER__

#include "vsdk/toolkit/render/shaders/Shader.h"
class LightingShader : public Shader {
private:
    bool specularEnabled;
    bool textureEnabled;
    bool bumpMapEnabled;

public:
    LightingShader(bool specularEnabled, bool textureEnabled, bool bumpMapEnabled);
    virtual ~LightingShader() {}

    virtual LocalShadingResult shadeLocal(
        RayHit* info,
        double viewX,
        double viewY,
        double viewZ,
        java::ArrayList<Light*>& lights,
        java::ArrayList<SimpleBody*>& objects,
        SimpleMaterial* material,
        TraceWorkspace* workspace) override;
};

#endif
