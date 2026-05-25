#include "io/pov/ast/AstSceneParser.h"

#include <cstdlib>
#include <cstring>

#include "io/Tokenizer.h"
#include "common/color/Color.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/scene/SceneFrame.h"
#include "environment/scene/ModelBuilder.h"
#include "io/pov/ParseErrorReporter.h"
#include "io/pov/ParseHelpers.h"
#include "io/pov/ParserContext.h"
#include "io/pov/ParseGlobals.h"
#include "io/pov/Parse.h"
#include "io/pov/PrimitiveParser.h"
#include "io/pov/ast/AstNodes.h"
#include "io/pov/ast/AstObjectParser.h"
#include "io/pov/ast/AstPrimitiveParser.h"
#include "io/pov/ast/AstParsedSceneProgram.h"

namespace {
void captureTextureTokens(ParserContext &ctx, AstTextureChainNode *textureChain)
{
    textureChain->captureToken(ctx.token());
    ctx.tokenStream().getToken();
    if (ctx.token().tokenId != Tokenizer::LEFT_CURLY_TOKEN) {
        ParseErrorReporter::parseError(Tokenizer::LEFT_CURLY_TOKEN, ctx);
    }
    textureChain->captureToken(ctx.token());
    int depth = 1;
    while (depth > 0) {
        ctx.tokenStream().getToken();
        textureChain->captureToken(ctx.token());
        if (ctx.token().tokenId == Tokenizer::LEFT_CURLY_TOKEN) {
            depth++;
        } else if (ctx.token().tokenId == Tokenizer::RIGHT_CURLY_TOKEN) {
            depth--;
        } else if (ctx.token().tokenId == Tokenizer::END_OF_FILE_TOKEN) {
            ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
        }
    }
}

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

AstDefaultTextureNode *
AstSceneParser::parseDefaultTextureNode(ParserContext &ctx)
{
    AstDefaultTextureNode *node = new AstDefaultTextureNode();
    node->sourceLine = ctx.token().tokenLineNo + 1;
    node->sourceFile = ctx.token().Filename;

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);
    int done = LegacyBoolean::FALSE_VALUE;
    while (!done) {
        ctx.tokenStream().getToken();
        switch (ctx.token().tokenId) {
        case Tokenizer::TEXTURE_TOKEN:
            if (node->textureChain == nullptr) {
                node->textureChain = new AstTextureChainNode();
            }
            captureTextureTokens(ctx, node->textureChain);
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

AstNode *
AstSceneParser::parseRootNodeForToken(ParserContext &ctx, int tokenId)
{
    switch (tokenId) {
    case Tokenizer::SPHERE_TOKEN:
        return AstObjectParser::parseSphere(ctx);
    case Tokenizer::PLANE_TOKEN:
        return AstObjectParser::parsePlane(ctx);
    case Tokenizer::BOX_TOKEN:
        return AstObjectParser::parseBox(ctx);
    case Tokenizer::QUADRIC_TOKEN:
        return AstObjectParser::parseQuadric(ctx);
    case Tokenizer::BLOB_TOKEN:
        return AstObjectParser::parseBlob(ctx);
    case Tokenizer::TRIANGLE_TOKEN:
        return AstObjectParser::parseTriangle(ctx);
    case Tokenizer::SMOOTH_TRIANGLE_TOKEN:
        return AstObjectParser::parseSmoothTriangle(ctx);
    case Tokenizer::CUBIC_TOKEN:
        return AstObjectParser::parsePoly(ctx, 3);
    case Tokenizer::QUARTIC_TOKEN:
        return AstObjectParser::parsePoly(ctx, 4);
    case Tokenizer::POLY_TOKEN:
        return AstObjectParser::parsePoly(ctx, 0);
    case Tokenizer::HEIGHT_FIELD_TOKEN:
        return AstObjectParser::parseHeightField(ctx);
    case Tokenizer::BICUBIC_PATCH_TOKEN:
        return AstObjectParser::parseBicubicPatch(ctx);
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
    ParseHelpers::getExpectedToken(Tokenizer::IDENTIFIER_TOKEN, ctx);
    const TokenStruct identifierToken = ctx.token();
    ParseHelpers::getExpectedToken(Tokenizer::EQUALS_TOKEN, ctx);
    ctx.tokenStream().getToken();
    const int valueTokenId = ctx.token().tokenId;

    Constant *constantPtr =
        ctx.symbols().upsertByIdentifierNumber(identifierToken.identifierNumber);
    if (constantPtr == nullptr) {
        ParseErrorReporter::Error("Too many constants \"declared\"", ctx);
    }
    constantPtr->identifierNumber = identifierToken.identifierNumber;

    if (valueTokenId == Tokenizer::COLOUR_TOKEN ||
        valueTokenId == Tokenizer::LEFT_ANGLE_TOKEN ||
        valueTokenId == Tokenizer::DASH_TOKEN ||
        valueTokenId == Tokenizer::PLUS_TOKEN ||
        valueTokenId == Tokenizer::FLOAT_TOKEN) {
        if (valueTokenId == Tokenizer::COLOUR_TOKEN) {
            constantPtr->constantData = (void *)ModelBuilder::getColour();
            constantPtr->constantType = ParseGlobals::COLOUR_CONSTANT;
            PrimitiveParser::parseColour((RGBAColor *)constantPtr->constantData, ctx);
        } else if (valueTokenId == Tokenizer::LEFT_ANGLE_TOKEN) {
            ctx.tokenStream().ungetToken();
            constantPtr->constantData = (void *)ModelBuilder::getVector();
            constantPtr->constantType = ParseGlobals::VECTOR_CONSTANT;
            PrimitiveParser::parseVector((Vector3Dd *)constantPtr->constantData, ctx);
        } else {
            ctx.tokenStream().ungetToken();
            constantPtr->constantData = (void *)ModelBuilder::getFloat();
            constantPtr->constantType = ParseGlobals::FLOAT_CONSTANT;
            *((double *)constantPtr->constantData) = PrimitiveParser::parseFloat(ctx);
        }
        AstDeclareNode *node = new AstDeclareNode();
        node->sourceLine = identifierToken.tokenLineNo + 1;
        node->sourceFile = identifierToken.Filename;
        node->identifierNumber = identifierToken.identifierNumber;
        AstConstantValueNode *value = new AstConstantValueNode();
        value->sourceLine = identifierToken.tokenLineNo + 1;
        value->sourceFile = identifierToken.Filename;
        value->constantType = constantPtr->constantType;
        value->constantData = constantPtr->constantData;
        node->value = value;
        return node;
    }

    if (valueTokenId == Tokenizer::TEXTURE_TOKEN) {
        AstDeclareNode *node = new AstDeclareNode();
        node->sourceLine = identifierToken.tokenLineNo + 1;
        node->sourceFile = identifierToken.Filename;
        node->identifierNumber = identifierToken.identifierNumber;
        AstTextureChainNode *textureNode = new AstTextureChainNode();
        textureNode->sourceLine = identifierToken.tokenLineNo + 1;
        textureNode->sourceFile = identifierToken.Filename;

        constantPtr->constantData = nullptr;
        constantPtr->constantType = ParseGlobals::TEXTURE_CONSTANT;
        ctx.tokenStream().ungetToken();
        while (LegacyBoolean::TRUE_VALUE) {
            ctx.tokenStream().getToken();
            if (ctx.token().tokenId != Tokenizer::TEXTURE_TOKEN) {
                ctx.tokenStream().ungetToken();
                break;
            }
            captureTextureTokens(ctx, textureNode);
        }
        node->value = textureNode;
        return node;
    }

    AstDeclareNode *node = new AstDeclareNode();
    node->sourceLine = identifierToken.tokenLineNo + 1;
    node->sourceFile = identifierToken.Filename;
    node->identifierNumber = identifierToken.identifierNumber;

    AstNode *value = nullptr;
    if (valueTokenId == Tokenizer::VIEW_POINT_TOKEN) {
        value = parseCameraNode(ctx);
    } else {
        value = AstSceneParser::parseRootNodeForToken(ctx, valueTokenId);
        if (value == nullptr) {
            ParseErrorReporter::parseError(Tokenizer::SPHERE_TOKEN, ctx);
        }
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
            AstDeclareNode *decl = parseDeclareNode(ctx);
            if (decl != nullptr &&
                !AstNodes::appendNode(scene->nodes, scene->nodeCount,
                    AstLimits::MAX_AST_SCENE_NODES, (AstNode *)decl)) {
                ParseErrorReporter::Error("Too many AST scene nodes", ctx);
            }
            break;
        }
        case Tokenizer::SPHERE_TOKEN:
        case Tokenizer::PLANE_TOKEN:
        case Tokenizer::BOX_TOKEN:
        case Tokenizer::QUADRIC_TOKEN:
    case Tokenizer::BLOB_TOKEN:
    case Tokenizer::TRIANGLE_TOKEN:
    case Tokenizer::SMOOTH_TRIANGLE_TOKEN:
    case Tokenizer::CUBIC_TOKEN:
    case Tokenizer::QUARTIC_TOKEN:
    case Tokenizer::POLY_TOKEN:
    case Tokenizer::HEIGHT_FIELD_TOKEN:
    case Tokenizer::BICUBIC_PATCH_TOKEN:
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
            AstNode *n = parseRootNodeForToken(ctx, ctx.token().tokenId);
            if (n == nullptr || !AstNodes::appendNode(scene->nodes, scene->nodeCount,
                    AstLimits::MAX_AST_SCENE_NODES, n)) {
                ParseErrorReporter::Error("Too many AST scene nodes", ctx);
            }
            break;
        }
        case Tokenizer::COMPOSITE_TOKEN: {
            AstNode *n = parseRootNodeForToken(ctx, ctx.token().tokenId);
            if (n == nullptr || !AstNodes::appendNode(scene->nodes, scene->nodeCount,
                    AstLimits::MAX_AST_SCENE_NODES, n)) {
                ParseErrorReporter::Error("Too many AST scene nodes", ctx);
            }
            break;
        }
        case Tokenizer::DEFAULT_TOKEN:
            if (!AstNodes::appendNode(scene->nodes, scene->nodeCount,
                    AstLimits::MAX_AST_SCENE_NODES, parseDefaultTextureNode(ctx))) {
                ParseErrorReporter::Error("Too many AST scene nodes", ctx);
            }
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
            break;
        }
        case Tokenizer::FOG_TOKEN: {
            AstFogNode *fog = parseFogNode(ctx);
            if (!AstNodes::appendNode(
                    scene->nodes, scene->nodeCount, AstLimits::MAX_AST_SCENE_NODES, fog)) {
                ParseErrorReporter::Error("Too many AST scene nodes", ctx);
            }
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
