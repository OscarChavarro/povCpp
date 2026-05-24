#include "io/pov/ParserContext.h"
#include "common/LegacyBoolean.h"
#include "environment/material/RendererConfiguration.h"
#include "common/linealAlgebra/Transformation.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "io/image/DumpFormat.h"
#include "io/image/GifFormat.h"
#include "io/image/IffFormat.h"
#include "io/image/TargaFormat.h"
#include "io/pov/Parse.h"
#include "io/pov/SceneFrameParser.h"
#include "io/pov/ast/AstSceneBuilder.h"
#include "io/pov/ast/AstNodes.h"
#include "io/pov/ast/AstSceneParser.h"
#include "environment/scene/SceneFrame.h"

#include "environment/camera/Camera.h"
#include "environment/geometry/elements/Triangle.h"
#include "environment/geometry/surface/InfinitePlane.h"
#include "environment/geometry/surface/parametric/ParametricPatch.h"
#include "environment/geometry/volume/Blob.h"
#include "environment/geometry/volume/Box.h"
#include "environment/geometry/volume/HeightField.h"
#include "environment/geometry/volume/Quadric.h"
#include "environment/geometry/volume/Sphere.h"
#include "environment/geometry/volume/compound/CSG.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "environment/geometry/volume/polynomial/PolynomialShape.h"
#include "environment/light/Light.h"



void
SceneParser::Parse(RenderFrame *framePtr)
{
    ParserContext ctx;
    SceneParser::Parse(framePtr, ctx);
}

void
SceneParser::Parse(RenderFrame *framePtr, ParserContext &ctx)
{
    SimpleBody *object;
    ctx.parsingFrame() = framePtr;

    ctx.degenerateTriangles() = FALSE;
    SceneParser::tokenInit(ctx);
    SceneParser::frameInit(ctx);
    SceneParser::parseFrame(ctx);
    for (object = ctx.parsingFrame()->Objects; object != nullptr;
        object = object->nextObject) {
        ParseHelpers::postProcessObject(object);
    }
    if (ctx.degenerateTriangles()) {
        fprintf(
            stderr, "Degenerate triangles were found and are being ignored.\n");
        /* exit(1); Let's ignore degen tri instead of blowing up. CdW */
    }
}

void
SceneParser::ParseAst(RenderFrame *framePtr)
{
    ParserContext ctx;
    SceneParser::ParseAst(framePtr, ctx);
}

void
SceneParser::ParseAst(RenderFrame *framePtr, ParserContext &ctx)
{
    SimpleBody *object;
    ctx.parsingFrame() = framePtr;

    ctx.degenerateTriangles() = FALSE;
    SceneParser::tokenInit(ctx);
    SceneParser::frameInit(ctx);
    AstScene *scene = AstSceneParser::parseScene(ctx);
    AstSceneBuilder::build(*scene, framePtr, ctx);
    AstNodes::destroyScene(scene);
    for (object = ctx.parsingFrame()->Objects; object != nullptr;
        object = object->nextObject) {
        ParseHelpers::postProcessObject(object);
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
    TextureUtils::defaultTexture() = TextureUtils::getTexture();
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
