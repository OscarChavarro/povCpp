#include "io/pov/ParserContext.h"
#include <cstdlib>
#include <string>
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
#include "io/pov/antlr/AntlrSceneRuntimePipeline.h"
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

namespace {
void postProcessPhase(ParserContext &ctx)
{
    for (SimpleBody *object = ctx.parsingFrame()->Objects; object != nullptr;
        object = object->nextObject) {
        ParseHelpers::postProcessObject(object);
    }
}

struct AntlrRoutingStats {
    long parseCalls = 0;
    long antlrAttempts = 0;
    long antlrSuccess = 0;
    long antlrFailures = 0;
};

AntlrRoutingStats &
antlrRoutingStats()
{
    static AntlrRoutingStats *stats = new AntlrRoutingStats();
    return *stats;
}

void printAntlrRoutingSummary()
{
    AntlrRoutingStats &stats = antlrRoutingStats();
    fprintf(stderr,
        "ANTLR routing summary: parse_calls=%ld antlr_attempts=%ld antlr_success=%ld antlr_failures=%ld\n",
        stats.parseCalls, stats.antlrAttempts, stats.antlrSuccess, stats.antlrFailures);
}

void ensureAntlrRoutingSummaryHook()
{
    static bool enabled = true;
    if (!enabled) {
        return;
    }

    static bool installed = false;
    if (!installed) {
        installed = true;
        std::atexit(printAntlrRoutingSummary);
    }
}
}



void
SceneParser::Parse(RenderFrame *framePtr)
{
    ParserContext ctx;
    SceneParser::Parse(framePtr, ctx);
}

void
SceneParser::Parse(RenderFrame *framePtr, ParserContext &ctx)
{
    ensureAntlrRoutingSummaryHook();
    AntlrRoutingStats &stats = antlrRoutingStats();
    ++stats.parseCalls;

#ifdef POV_WITH_ANTLR_RUNTIME
    ++stats.antlrAttempts;
    ctx.parsingFrame() = framePtr;
    ctx.degenerateTriangles() = LegacyBoolean::FALSE_VALUE;
    SceneParser::tokenInit(ctx);
    SceneParser::frameInit(ctx);

    std::string antlrError;
    try {
        if (AntlrSceneRuntimePipeline::parseAndApply(framePtr, antlrError)) {
            ++stats.antlrSuccess;
            postProcessPhase(ctx);
            if (ctx.degenerateTriangles()) {
                fprintf(stderr, "Degenerate triangles were found and are being ignored.\n");
            }
            return;
        }
    } catch (const ParseErrorReporter::ParseException &e) {
        antlrError = e.what();
    } catch (const std::exception &e) {
        antlrError = e.what();
    } catch (...) {
        antlrError = "Unknown ANTLR runtime pipeline error";
    }
    ++stats.antlrFailures;
    if (antlrError.empty()) {
        antlrError = "ANTLR pipeline failed";
    }
    ParseErrorReporter::Error(antlrError.c_str(), ctx);
#else
    ctx.parsingFrame() = framePtr;
    ctx.degenerateTriangles() = LegacyBoolean::FALSE_VALUE;
    SceneParser::tokenInit(ctx);
    SceneParser::frameInit(ctx);
    SceneParser::parseFrame(ctx);
    postProcessPhase(ctx);
    if (ctx.degenerateTriangles()) {
        fprintf(stderr, "Degenerate triangles were found and are being ignored.\n");
    }
#endif
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
