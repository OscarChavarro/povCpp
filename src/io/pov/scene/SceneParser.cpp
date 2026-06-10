#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/material/RendererConfiguration.h"
#include "environment/scene/SceneFrame.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/scene/SceneFrameParser.h"
#include "io/pov/scene/SceneParser.h"
#include "solidTexture/MaterialUtils.h"

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
    MaterialUtils::instance().setDefaultTexture(TextureUtils::instance().getTexture());
    ctx.parsingFrame()->viewPoint.initializeDefaults();
    ctx.parsingFrame()->Light_Sources = nullptr;
    ctx.parsingFrame()->Objects = nullptr;
    ctx.parsingFrame()->atmosphereIor = 1.0;
    ctx.parsingFrame()->antialiasThreshold = RenderingConfiguration::global().antialiasThreshold;
    ctx.parsingFrame()->fogDistance = 0.0;
    ctx.parsingFrame()->fogColor.setR(0.0); ctx.parsingFrame()->fogColor.setG(0.0); ctx.parsingFrame()->fogColor.setB(0.0); ctx.parsingFrame()->fogColor.setA(0);
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
