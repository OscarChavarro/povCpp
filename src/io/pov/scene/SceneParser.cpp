#include <cstdio>

#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"
#include "environment/material/MaterialUtils.h"
#include "environment/material/RendererConfiguration.h"
#include "environment/scene/Scene.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/scene/SceneBodyParser.h"
#include "io/pov/scene/ScenePostProcessor.h"
#include "io/pov/scene/SceneParser.h"

void
SceneParser::postProcessPhase(Scene *framePtr)
{
    const java::ArrayList<BoundedGeometry*> &objects =
        framePtr->getObjects();
    Light *lightHead = nullptr;
    for (long int i = objects.size() - 1; i >= 0; i--) {
        ScenePostProcessor::linkLights(objects[i], lightHead);
    }
    framePtr->setLightSources(lightHead);
}



void
SceneParser::parse(Scene *framePtr)
{
    ParserContext ctx;
    SceneParser::parse(framePtr, ctx);
}

void
SceneParser::parse(Scene *framePtr, ParserContext &ctx)
{
    Scene parsedFrame = *framePtr;
    ctx.degenerateTriangles() = false;
    SceneParser::tokenInit(ctx);
    SceneParser::frameInit(&parsedFrame, ctx);
    SceneParser::parseFrame(&parsedFrame, ctx);
    postProcessPhase(&parsedFrame);
    *framePtr = parsedFrame;
    if (ctx.degenerateTriangles()) {
        fprintf(stderr, "Degenerate triangles were found and are being ignored.\n");
    }
}

void
SceneParser::tokenInit()
{
    ParserContext ctx;
    SceneParser::tokenInit(ctx);
}

void
SceneParser::tokenInit(ParserContext &ctx)
{
    ctx.resetTokenStreamHistory();
    ctx.symbols().clear();
}

// Set up the fields in the frame to default values
void
SceneParser::frameInit()
{
    ParserContext ctx;
    Scene frame;
    SceneParser::frameInit(&frame, ctx);
}

void
SceneParser::frameInit(Scene *framePtr, ParserContext &ctx)
{
    ctx.setDefaultTexture(MaterialUtils::getTexture());
    framePtr->resetForSceneParse(ctx.getReportingConfig());
}

void
SceneParser::parseFrame()
{
    ParserContext ctx;
    Scene frame;
    SceneParser::parseFrame(&frame, ctx);
}

void
SceneParser::parseFrame(Scene *framePtr, ParserContext &ctx)
{
    SceneBodyParser::parseFrame(framePtr, ctx);
}
