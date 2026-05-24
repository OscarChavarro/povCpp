#include "io/pov/ast/AstSceneParser.h"

#include <cstdlib>

#include "io/Tokenizer.h"
#include "environment/scene/SceneFrame.h"
#include "environment/scene/SimpleBodyFactory.h"
#include "io/pov/DeclarationParser.h"
#include "io/pov/ParseErrorReporter.h"
#include "io/pov/ParseHelpers.h"
#include "io/pov/ParserContext.h"
#include "io/pov/RewindableTokenStream.h"
#include "io/pov/ast/AstNodes.h"
#include "io/pov/ast/AstObjectParser.h"
#include "io/pov/ast/AstPrimitiveParser.h"
#include "io/pov/ast/AstParsedSceneProgram.h"
#include "io/pov/geometryParser/ObjectParser.h"
#include "io/pov/mediaParser/DefaultTextureParser.h"

namespace {
void appendCameraOpOrFail(ParserContext &ctx, AstCameraNode *node, AstCameraOpKind kind,
    int refId, const AstVector3 &v)
{
    if (node->opCount >= AstCameraNode::MAX_AST_CAMERA_OPS) {
        ParseErrorReporter::Error("Too many AST camera operations", ctx);
    }
    node->ops[node->opCount].kind = kind;
    node->ops[node->opCount].referenceConstantId = refId;
    node->ops[node->opCount].vectorValue = v;
    node->opCount++;
}
}

AstFogNode *
AstSceneParser::parseFogNode(ParserContext &ctx)
{
    AstFogNode *node = new AstFogNode();
    node->sourceLine = ctx.token().tokenLineNo + 1;
    node->sourceFile = ctx.token().Filename;
    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);
    int done = LegacyBoolean::FALSE_VALUE;
    while (!done) {
        ctx.tokenStream().getToken();
        switch (ctx.token().tokenId) {
        case Tokenizer::COLOUR_TOKEN:
            node->colour = AstPrimitiveParser::parseColour(ctx);
            node->hasColour = true;
            break;
        case Tokenizer::FLOAT_TOKEN:
        case Tokenizer::PLUS_TOKEN:
        case Tokenizer::DASH_TOKEN:
        case Tokenizer::IDENTIFIER_TOKEN:
            ctx.tokenStream().ungetToken();
            node->distance = AstPrimitiveParser::parseFloat(ctx);
            node->hasDistance = true;
            break;
        case Tokenizer::RIGHT_CURLY_TOKEN:
            done = LegacyBoolean::TRUE_VALUE;
            break;
        default:
            ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
            break;
        }
    }
    return node;
}

AstCameraNode *
AstSceneParser::parseCameraNode(ParserContext &ctx)
{
    AstCameraNode *node = new AstCameraNode();
    node->sourceLine = ctx.token().tokenLineNo + 1;
    node->sourceFile = ctx.token().Filename;
    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);
    int done = LegacyBoolean::FALSE_VALUE;
    while (!done) {
        ctx.tokenStream().getToken();
        switch (ctx.token().tokenId) {
        case Tokenizer::IDENTIFIER_TOKEN:
            appendCameraOpOrFail(ctx, node, AST_CAMERA_REF, ctx.token().identifierNumber,
                AstVector3 {0.0, 0.0, 0.0});
            break;
        case Tokenizer::LOCATION_TOKEN:
            appendCameraOpOrFail(
                ctx, node, AST_CAMERA_LOCATION, -1, AstPrimitiveParser::parseVector(ctx));
            break;
        case Tokenizer::DIRECTION_TOKEN:
            appendCameraOpOrFail(
                ctx, node, AST_CAMERA_DIRECTION, -1, AstPrimitiveParser::parseVector(ctx));
            break;
        case Tokenizer::UP_TOKEN:
            appendCameraOpOrFail(
                ctx, node, AST_CAMERA_UP, -1, AstPrimitiveParser::parseVector(ctx));
            break;
        case Tokenizer::RIGHT_TOKEN:
            appendCameraOpOrFail(
                ctx, node, AST_CAMERA_RIGHT, -1, AstPrimitiveParser::parseVector(ctx));
            break;
        case Tokenizer::SKY_TOKEN:
            appendCameraOpOrFail(
                ctx, node, AST_CAMERA_SKY, -1, AstPrimitiveParser::parseVector(ctx));
            break;
        case Tokenizer::LOOK_AT_TOKEN:
            appendCameraOpOrFail(
                ctx, node, AST_CAMERA_LOOK_AT, -1, AstPrimitiveParser::parseVector(ctx));
            break;
        case Tokenizer::TRANSLATE_TOKEN:
            appendCameraOpOrFail(
                ctx, node, AST_CAMERA_TRANSLATE, -1, AstPrimitiveParser::parseVector(ctx));
            break;
        case Tokenizer::ROTATE_TOKEN:
            appendCameraOpOrFail(
                ctx, node, AST_CAMERA_ROTATE, -1, AstPrimitiveParser::parseVector(ctx));
            break;
        case Tokenizer::SCALE_TOKEN:
            appendCameraOpOrFail(
                ctx, node, AST_CAMERA_SCALE, -1, AstPrimitiveParser::parseVector(ctx));
            break;
        case Tokenizer::RIGHT_CURLY_TOKEN:
            done = LegacyBoolean::TRUE_VALUE;
            break;
        default:
            ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
            break;
        }
    }
    return node;
}

AstMaxTraceLevelNode *
AstSceneParser::parseMaxTraceLevelNode(ParserContext &ctx)
{
    AstMaxTraceLevelNode *node = new AstMaxTraceLevelNode();
    node->sourceLine = ctx.token().tokenLineNo + 1;
    node->sourceFile = ctx.token().Filename;
    node->value = AstPrimitiveParser::parseFloat(ctx);
    return node;
}

bool
AstSceneParser::isAstDeclareToken(int tokenId)
{
    return tokenId == Tokenizer::VIEW_POINT_TOKEN;
}

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
            const char *enableAstDeclare = std::getenv("POVCPP_AST_DECLARE_VIEWPOINT");
            if (enableAstDeclare == nullptr || enableAstDeclare[0] != '1') {
                DeclarationParser::parseDeclare(ctx);
                break;
            }

            ITokenStream *originalStream = &ctx.tokenStream();
            RewindableTokenStream rewindableStream(originalStream);
            ctx.setTokenStream(&rewindableStream);
            try {
                const int marker = ctx.tokenStream().mark();

                bool parseWithAst = false;
                bool parsed = false;
                try {
                    ParseHelpers::getExpectedToken(Tokenizer::IDENTIFIER_TOKEN, ctx);
                    ParseHelpers::getExpectedToken(Tokenizer::EQUALS_TOKEN, ctx);
                    ctx.tokenStream().getToken();
                    parseWithAst = isAstDeclareToken(ctx.token().tokenId);
                    ctx.tokenStream().rewind(marker);

                    if (parseWithAst) {
                        AstDeclareNode *decl = AstSceneParser::parseDeclareNode(ctx);
                        if (!AstNodes::appendNode(scene->nodes, scene->nodeCount,
                                AstLimits::MAX_AST_SCENE_NODES, (AstNode *)decl)) {
                            ParseErrorReporter::Error("Too many AST scene nodes", ctx);
                        }
                        parsed = true;
                    }
                } catch (const ParseErrorReporter::ParseException &) {
                    ctx.tokenStream().rewind(marker);
                }

                if (!parsed) {
                    ctx.tokenStream().rewind(marker);
                    DeclarationParser::parseDeclare(ctx);
                }

                ctx.setTokenStream(originalStream);
            } catch (...) {
                ctx.setTokenStream(originalStream);
                throw;
            }
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
        case Tokenizer::DEFAULT_TOKEN:
            DefaultTextureParser::parseDefault(&program->legacyFrame, ctx);
            break;
        case Tokenizer::MAX_TRACE_LEVEL_TOKEN:
            if (!AstNodes::appendNode(scene->nodes, scene->nodeCount,
                    AstLimits::MAX_AST_SCENE_NODES, parseMaxTraceLevelNode(ctx))) {
                ParseErrorReporter::Error("Too many AST scene nodes", ctx);
            }
            break;
        case Tokenizer::VIEW_POINT_TOKEN: {
            AstCameraNode *camera = parseCameraNode(ctx);
            if (!AstNodes::appendNode(
                    scene->nodes, scene->nodeCount, AstLimits::MAX_AST_SCENE_NODES, camera)) {
                ParseErrorReporter::Error("Too many AST scene nodes", ctx);
            }
            program->hasCamera = true;
            break;
        }
        case Tokenizer::FOG_TOKEN: {
            AstFogNode *fog = parseFogNode(ctx);
            if (!AstNodes::appendNode(
                    scene->nodes, scene->nodeCount, AstLimits::MAX_AST_SCENE_NODES, fog)) {
                ParseErrorReporter::Error("Too many AST scene nodes", ctx);
            }
            program->hasFog = true;
            break;
        }
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
