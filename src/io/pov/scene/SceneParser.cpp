#include <cstdio>

#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"
#include "environment/material/MaterialUtils.h"
#include "environment/material/RendererConfiguration.h"
#include "environment/scene/SceneFrame.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/scene/SceneFrameParser.h"
#include "io/pov/scene/SceneParser.h"

void
SceneParser::postProcessPhase(ParserContext &ctx)
{
    java::ArrayList<BoundedGeometry*> &objects = ctx.parsingFrame()->getObjects();
    Light *lightHead = nullptr;
    for (long int i = objects.size() - 1; i >= 0; i--) {
        ParseHelpers::postProcessObject(objects[i], lightHead);
    }
    ctx.parsingFrame()->setLightSources(lightHead);
}



void
SceneParser::parse(RenderFrame *framePtr)
{
    ParserContext ctx;
    SceneParser::parse(framePtr, ctx);
}

void
SceneParser::parse(RenderFrame *framePtr, ParserContext &ctx)
{
    RenderFrame parsedFrame = *framePtr;
    ctx.parsingFrame() = &parsedFrame;
    ctx.degenerateTriangles() = false;
    SceneParser::tokenInit(ctx);
    SceneParser::frameInit(ctx);
    SceneParser::parseFrame(ctx);
    postProcessPhase(ctx);
    *framePtr = parsedFrame;
    ctx.parsingFrame() = framePtr;
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
    SceneParser::frameInit(ctx);
}

void
SceneParser::frameInit(ParserContext &ctx)
{
    MaterialUtils::instance().setDefaultTexture(MaterialUtils::instance().getTexture());
    ctx.parsingFrame()->resetForSceneParse();
}

void
SceneParser::parseFrame()
{
    ParserContext ctx;
    SceneParser::parseFrame(ctx);
}

void
SceneParser::parseFrame(ParserContext &ctx)
{
    SceneFrameParser::parseFrame(ctx.parsingFrame(), ctx);
}
