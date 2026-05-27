#include "io/pov/ParserContext.h"
#include <cstdlib>
#include <map>
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
#include "io/pov/ast/AstSceneBuilder.h"
#include "io/pov/ast/AstNodes.h"
#include "io/pov/ast/AstParsedSceneProgram.h"
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

namespace {
#ifndef POVCPP_DEFAULT_USE_ANTLR
#define POVCPP_DEFAULT_USE_ANTLR 1
#endif
#ifndef POVCPP_DEFAULT_ANTLR_STRICT
#define POVCPP_DEFAULT_ANTLR_STRICT 1
#endif
#ifndef POVCPP_DEFAULT_ANTLR_ROUTING_STATS
#define POVCPP_DEFAULT_ANTLR_ROUTING_STATS 1
#endif

AstParsedSceneProgram *parseAstPhase(ParserContext &ctx)
{
    return AstSceneParser::parseProgram(ctx);
}

void buildScenePhase(
    const AstParsedSceneProgram &program, RenderFrame *framePtr, ParserContext &ctx)
{
    AstSceneBuilder::build(*program.scene, framePtr, ctx);
}

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
    long astFallbacks = 0;
    std::map<std::string, long> fallbackByReason;
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
        "ANTLR routing summary: parse_calls=%ld antlr_attempts=%ld antlr_success=%ld ast_fallbacks=%ld\n",
        stats.parseCalls, stats.antlrAttempts, stats.antlrSuccess, stats.astFallbacks);
    for (const auto &kv : stats.fallbackByReason) {
        fprintf(stderr, "  fallback_reason[%s]=%ld\n", kv.first.c_str(), kv.second);
    }
}

void ensureAntlrRoutingSummaryHook()
{
    static bool enabled = false;
    static bool enabledInitialized = false;
    if (!enabledInitialized) {
        enabledInitialized = true;
        const char *env = std::getenv("POVCPP_ANTLR_ROUTING_STATS");
        enabled = (env != nullptr) ? (env[0] == '1') : (POVCPP_DEFAULT_ANTLR_ROUTING_STATS == 1);
    }
    if (!enabled) {
        return;
    }

    static bool installed = false;
    if (!installed) {
        installed = true;
        std::atexit(printAntlrRoutingSummary);
    }
}

#ifdef POV_WITH_ANTLR_RUNTIME
std::string classifyAntlrFallbackReason(const std::string &error)
{
    if (error.empty()) {
        return "antlr_returned_false_without_error";
    }
    if (error.find("syntax error") != std::string::npos) {
        return "antlr_syntax_error";
    }
    if (error.find("Unknown ANTLR") != std::string::npos) {
        return "antlr_semantic_unknown_reference";
    }
    if (error.find("requires inline") != std::string::npos) {
        return "antlr_semantic_inline_requirement";
    }
    return "antlr_runtime_or_other_error";
}
#endif
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

    bool useAntlr = (POVCPP_DEFAULT_USE_ANTLR == 1);
    const char *useAntlrEnv = std::getenv("POVCPP_USE_ANTLR");
    if (useAntlrEnv != nullptr) {
        useAntlr = (useAntlrEnv[0] == '1');
    }

    if (useAntlr) {
#ifdef POV_WITH_ANTLR_RUNTIME
        bool antlrStrict = (POVCPP_DEFAULT_ANTLR_STRICT == 1);
        const char *antlrStrictEnv = std::getenv("POVCPP_ANTLR_STRICT");
        if (antlrStrictEnv != nullptr) {
            antlrStrict = (antlrStrictEnv[0] == '1');
        }

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

        if (antlrStrict) {
            if (antlrError.empty()) {
                antlrError = "ANTLR pipeline failed in strict mode";
            }
            ParseErrorReporter::Error(antlrError.c_str(), ctx);
        }

        ++stats.astFallbacks;
        ++stats.fallbackByReason[classifyAntlrFallbackReason(antlrError)];
        if (!antlrError.empty()) {
            fprintf(stderr, "ANTLR pipeline fallback to AST: %s\n", antlrError.c_str());
        } else {
            fprintf(stderr, "ANTLR pipeline fallback to AST\n");
        }
#else
        ++stats.astFallbacks;
        ++stats.fallbackByReason["antlr_runtime_not_compiled"];
#endif
    } else {
        ++stats.astFallbacks;
        ++stats.fallbackByReason["antlr_disabled_by_env"];
    }
    SceneParser::ParseAst(framePtr, ctx);
}

void
SceneParser::ParseAst(RenderFrame *framePtr)
{
    ParserContext astCtx;
    SceneParser::ParseAst(framePtr, astCtx);
}

void
SceneParser::ParseAst(RenderFrame *framePtr, ParserContext &ctx)
{
    ctx.parsingFrame() = framePtr;

    ctx.degenerateTriangles() = LegacyBoolean::FALSE_VALUE;
    SceneParser::tokenInit(ctx);
    SceneParser::frameInit(ctx);
    AstParsedSceneProgram *program = nullptr;
    try {
        program = parseAstPhase(ctx);
        buildScenePhase(*program, framePtr, ctx);
        postProcessPhase(ctx);
    } catch (const ParseErrorReporter::ParseException &) {
        if (program != nullptr) {
            AstNodes::destroyScene(program->scene);
            delete program;
        }
        throw;
    } catch (const std::exception &e) {
        if (program != nullptr) {
            AstNodes::destroyScene(program->scene);
            delete program;
        }
        ParseErrorReporter::Error(e.what(), ctx);
    } catch (...) {
        if (program != nullptr) {
            AstNodes::destroyScene(program->scene);
            delete program;
        }
        ParseErrorReporter::Error("Unknown parser error", ctx);
    }
    AstNodes::destroyScene(program->scene);
    delete program;
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
