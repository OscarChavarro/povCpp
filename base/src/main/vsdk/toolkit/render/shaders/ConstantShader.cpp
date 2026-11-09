#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/material/SimpleMaterial.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/render/shaders/ConstantShader.h"
Shader::LocalShadingResult ConstantShader::shadeLocal(RayHit* info,double,double,double,java::ArrayList<Light*>&,java::ArrayList<SimpleBody*>&,SimpleMaterial* material,TraceWorkspace*)
{ return LocalShadingResult(info->normal, ColorRgb(material->getDiffuseReference())); }
