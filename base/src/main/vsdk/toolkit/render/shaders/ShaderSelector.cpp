#include "vsdk/toolkit/environment/material/RendererConfiguration.h"
#include "vsdk/toolkit/render/shaders/ConstantShader.h"
#include "vsdk/toolkit/render/shaders/ConstantTextureShader.h"
#include "vsdk/toolkit/render/shaders/CookTorranceShader.h"
#include "vsdk/toolkit/render/shaders/FlatShader.h"
#include "vsdk/toolkit/render/shaders/FlatTexturedShader.h"
#include "vsdk/toolkit/render/shaders/GouraudTextureShader.h"
#include "vsdk/toolkit/render/shaders/PhongBumpShader.h"
#include "vsdk/toolkit/render/shaders/PhongShader.h"
#include "vsdk/toolkit/render/shaders/PhongTextureBumpShader.h"
#include "vsdk/toolkit/render/shaders/PhongTextureShader.h"
#include "vsdk/toolkit/render/shaders/ShaderSelector.h"
Shader* ShaderSelector::select(const RendererConfiguration* qualitySelection)
{
    int shadingType = qualitySelection->getShadingType();
    bool textureEnabled = qualitySelection->isTextureSet();
    bool bumpMapEnabled = qualitySelection->isBumpMapSet();

    if ( shadingType == RendererConfiguration::SHADING_TYPE_NOLIGHT ) {
        if ( textureEnabled ) return new ConstantTextureShader();
        return new ConstantShader();
    }
    if ( shadingType == RendererConfiguration::SHADING_TYPE_FLAT ) {
        if ( textureEnabled ) return new FlatTexturedShader();
        return new FlatShader();
    }
    if ( shadingType == RendererConfiguration::SHADING_TYPE_GOURAUD ) {
        return new GouraudTextureShader(textureEnabled);
    }
    if ( shadingType == RendererConfiguration::SHADING_TYPE_PHONG ) {
        if ( bumpMapEnabled ) {
            if ( textureEnabled ) return new PhongTextureBumpShader();
            return new PhongBumpShader();
        }
        if ( textureEnabled ) return new PhongTextureShader();
        return new PhongShader();
    }
    if ( shadingType == RendererConfiguration::SHADING_TYPE_COOK_TERRANCE ) {
        return new CookTorranceShader(textureEnabled, bumpMapEnabled);
    }
    return new GouraudTextureShader(textureEnabled);
}
