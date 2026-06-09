#include "io/pov/context/ParserContext.h"
#include "environment/material/RendererConfiguration.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/scene/SceneParser.h"
#include "io/pov/scene/SceneFrameParser.h"
#include "environment/scene/SceneFrame.h"

void
SceneParser::postProcessPhase(ParserContext &ctx)
{
    for (SimpleBody *object = ctx.parsingFrame()->Objects; object != nullptr;
        object = object->nextObject) {
        ParseHelpers::postProcessObject(object);
    }
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
    ctx.parsingFrame() = framePtr;
    ctx.degenerateTriangles() = false;
    SceneParser::tokenInit(ctx);
    SceneParser::frameInit(ctx);
    SceneParser::parseFrame(ctx);
    postProcessPhase(ctx);
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

/* Set up the fields in the frame to default values. */
void
SceneParser::frameInit()
{
    ParserContext ctx;
    SceneParser::frameInit(ctx);
}

void
SceneParser::frameInit(ParserContext &ctx)
{
    textureUtils::instance().defaultTexture() = textureUtils::instance().getTexture();
    ctx.parsingFrame()->viewPoint.initializeDefaults();
    ctx.parsingFrame()->Light_Sources = nullptr;
    ctx.parsingFrame()->Objects = nullptr;
    ctx.parsingFrame()->atmosphereIor = 1.0;
    ctx.parsingFrame()->antialiasThreshold = RenderingConfiguration::global().antialiasThreshold;
    ctx.parsingFrame()->fogDistance = 0.0;
    Color::makeColor(&(ctx.parsingFrame()->fogColour), 0.0, 0.0, 0.0);
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
