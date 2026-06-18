#include "java/util/ArrayList.txx"

#include "environment/material/RendererConfiguration.h"
#include "environment/scene/Scene.h"

namespace {
ColorRgba
blackFogColor()
{
    ColorRgba color;
    color.setR(0.0);
    color.setG(0.0);
    color.setB(0.0);
    color.setA(0.0);
    return color;
}
}

Scene::Scene() :
    screenHeight(0),
    screenWidth(0),
    lightSources(nullptr),
    atmosphereIor(1.0),
    antialiasThreshold(RenderingConfiguration::global().getAntialiasThreshold()),
    fogDistance(0.0)
{
    fogColor = blackFogColor();
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
