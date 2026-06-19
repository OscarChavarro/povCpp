#include "java/util/ArrayList.txx"

#include "environment/material/RendererConfiguration.h"
#include "environment/scene/Scene.h"

namespace {
ColorRgba
blackFogColor()
{
    return ColorRgba(0.0, 0.0, 0.0, 0.0);
}
}

Scene::Scene() :
    screenHeight(0),
    screenWidth(0),
    lightSources(nullptr),
    atmosphereIor(1.0),
    antialiasThreshold(RenderingConfiguration::global().getAntialiasThreshold()),
    fogDistance(0.0),
    fogColor(0.0, 0.0, 0.0, 0.0)
{
}

void
Scene::resetForSceneParse()
{
    viewPoint = Camera();
    lightSources = nullptr;
    Objects.clear();
    atmosphereIor = 1.0;
    antialiasThreshold = RenderingConfiguration::global().getAntialiasThreshold();
    setFog(blackFogColor(), 0.0);
}
