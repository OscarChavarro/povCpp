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
    antialiasThreshold(Scene::DEFAULT_ANTIALIAS_THRESHOLD),
    fogDistance(0.0),
    fogColor(0.0, 0.0, 0.0, 0.0)
{
}

void
Scene::resetForSceneParse(const RenderingConfiguration *config)
{
    viewPoint = Camera();
    lightSources = nullptr;
    Objects.clear();
    atmosphereIor = 1.0;
    antialiasThreshold = config ? config->getAntialiasThreshold() : Scene::DEFAULT_ANTIALIAS_THRESHOLD;
    setFog(blackFogColor(), 0.0);
}
