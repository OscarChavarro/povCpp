#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/material/SimpleMaterial.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/render/shaders/ConstantTextureShader.h"
#include "vsdk/toolkit/render/shaders/CpuTextureSamplingConfig.h"
Shader::LocalShadingResult ConstantTextureShader::shadeLocal(RayHit* info,double,double,double,java::ArrayList<Light*>&,java::ArrayList<SimpleBody*>&,SimpleMaterial* material,TraceWorkspace*)
{
    ColorRgb diffuse = material->getDiffuseReference();
    double r=diffuse.r(),g=diffuse.g(),b=diffuse.b();
    if ( info->texture != 0 ) {
        ColorRgb tc = CpuTextureSamplingConfig::sample(info->texture, info->u, 1-info->v);
        r*=tc.r(); g*=tc.g(); b*=tc.b();
    }
    return LocalShadingResult(info->normal, ColorRgb(r,g,b));
}
