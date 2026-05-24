#include "io/pov/ast/AstSceneParser.h"

#include "io/Tokenizer.h"
#include "environment/scene/SceneFrame.h"
#include "environment/scene/SimpleBodyFactory.h"
#include "io/pov/DeclarationParser.h"
#include "io/pov/FogParser.h"
#include "io/pov/ParseErrorReporter.h"
#include "io/pov/ParseHelpers.h"
#include "io/pov/ParserContext.h"
#include "io/pov/RenderSettingsParser.h"
#include "io/pov/ast/AstNodes.h"
#include "io/pov/ast/AstObjectParser.h"
#include "io/pov/ast/AstParsedSceneProgram.h"
#include "io/pov/cameraParser/CameraParser.h"
#include "io/pov/geometryParser/ObjectParser.h"
#include "io/pov/mediaParser/DefaultTextureParser.h"

AstNode *
AstSceneParser::parseRootNodeForToken(ParserContext &ctx, int tokenId)
{
    switch (tokenId) {
    case Tokenizer::SPHERE_TOKEN:
        return AstObjectParser::parseSphere(ctx);
    case Tokenizer::LIGHT_SOURCE_TOKEN:
        return AstObjectParser::parseLightSource(ctx);
    case Tokenizer::UNION_TOKEN:
        return AstObjectParser::parseCsg(ctx, AST_CSG_UNION);
    case Tokenizer::INTERSECTION_TOKEN:
        return AstObjectParser::parseCsg(ctx, AST_CSG_INTERSECTION);
    case Tokenizer::DIFFERENCE_TOKEN:
        return AstObjectParser::parseCsg(ctx, AST_CSG_DIFFERENCE);
    case Tokenizer::OBJECT_TOKEN:
        return AstObjectParser::parseObject(ctx);
    case Tokenizer::COMPOSITE_TOKEN:
        return AstObjectParser::parseComposite(ctx);
    default:
        return nullptr;
    }
}

AstDeclareNode *
AstSceneParser::parseDeclareNode(ParserContext &ctx)
{
    AstDeclareNode *node = new AstDeclareNode();
    node->sourceLine = ctx.token().tokenLineNo + 1;
    node->sourceFile = ctx.token().Filename;

    ParseHelpers::getExpectedToken(Tokenizer::IDENTIFIER_TOKEN, ctx);
    node->identifierNumber = ctx.token().identifierNumber;
    ParseHelpers::getExpectedToken(Tokenizer::EQUALS_TOKEN, ctx);
    ctx.tokenStream().getToken();
    AstNode *value = AstSceneParser::parseRootNodeForToken(ctx, ctx.token().tokenId);
    if (value == nullptr) {
        ParseErrorReporter::parseError(Tokenizer::SPHERE_TOKEN, ctx);
    }
    node->value = value;
    return node;
}

AstParsedSceneProgram *
AstSceneParser::parseProgram(ParserContext &ctx)
{
    AstParsedSceneProgram *program = new AstParsedSceneProgram();
    AstScene *scene = new AstScene();
    program->scene = scene;

    int done = LegacyBoolean::FALSE_VALUE;
    while (!done) {
        ctx.tokenStream().getToken();
        switch (ctx.token().tokenId) {
        case Tokenizer::DECLARE_TOKEN: {
            // Keep full DECLARE compatibility by reusing the legacy declaration parser.
            // AST nodes can still reference declared constants through identifier numbers.
            DeclarationParser::parseDeclare(ctx);
            break;
        }
        case Tokenizer::SPHERE_TOKEN:
        case Tokenizer::LIGHT_SOURCE_TOKEN:
        case Tokenizer::UNION_TOKEN:
        case Tokenizer::INTERSECTION_TOKEN:
        case Tokenizer::DIFFERENCE_TOKEN: {
            AstNode *n =
                AstSceneParser::parseRootNodeForToken(ctx, ctx.token().tokenId);
            if (n == nullptr) {
                ParseErrorReporter::parseError(Tokenizer::SPHERE_TOKEN, ctx);
            }
            if (!AstNodes::appendNode(
                    scene->nodes, scene->nodeCount, AstLimits::MAX_AST_SCENE_NODES, n)) {
                ParseErrorReporter::Error("Too many AST scene nodes", ctx);
            }
            break;
        }
        case Tokenizer::OBJECT_TOKEN: {
            SimpleBody *localObject = ObjectParser::parseObject(ctx);
            SimpleBodyFactory::link(
                localObject, &(localObject->nextObject), (SimpleBody **)&(program->legacyFrame.Objects));
            break;
        }
        case Tokenizer::COMPOSITE_TOKEN: {
            SimpleBody *localObject = ObjectParser::parseComposite(ctx);
            SimpleBodyFactory::link(
                localObject, &(localObject->nextObject), (SimpleBody **)&(program->legacyFrame.Objects));
            break;
        }
        case Tokenizer::FOG_TOKEN:
            FogParser::parseFog(&program->legacyFrame, ctx);
            program->hasFog = true;
            break;
        case Tokenizer::DEFAULT_TOKEN:
            DefaultTextureParser::parseDefault(&program->legacyFrame, ctx);
            break;
        case Tokenizer::MAX_TRACE_LEVEL_TOKEN:
            RenderSettingsParser::parseMaxTraceLevel(ctx);
            break;
        case Tokenizer::VIEW_POINT_TOKEN:
            CameraParser::parseCamera(&(program->legacyFrame.viewPoint), ctx);
            program->hasCamera = true;
            break;
        case Tokenizer::END_OF_FILE_TOKEN:
            done = LegacyBoolean::TRUE_VALUE;
            break;
        default:
            ParseErrorReporter::parseError(Tokenizer::OBJECT_TOKEN, ctx);
            break;
        }
    }

    return program;
}
