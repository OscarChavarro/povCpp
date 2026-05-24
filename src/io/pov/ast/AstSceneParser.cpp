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
#include "io/pov/geometryParser/ObjectParser.h"
#include "io/pov/mediaParser/TextureParser.h"
#include "media/TextureUtils.h"

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
            node->texture = TextureParser::parseTexture(ctx);
            if (node->texture != nullptr) {
                node->texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                TextureUtils::defaultTexture() = node->texture;
                TextureUtils::defaultTexture()->constantFlag = LegacyBoolean::TRUE_VALUE;
            }
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
        return nullptr;
    }

    if (valueTokenId == Tokenizer::TEXTURE_TOKEN) {
        Texture *localTexture = nullptr;
        Texture *tempTexture = nullptr;
        constantPtr->constantData = (void *)localTexture;
        constantPtr->constantType = ParseGlobals::TEXTURE_CONSTANT;
        ctx.tokenStream().ungetToken();
        while (LegacyBoolean::TRUE_VALUE) {
            ctx.tokenStream().getToken();
            if (ctx.token().tokenId != Tokenizer::TEXTURE_TOKEN) {
                ctx.tokenStream().ungetToken();
                break;
            }
            localTexture = TextureUtils::defaultTexture();
            localTexture = TextureParser::parseTexture(ctx);
            if (localTexture->constantFlag) {
                localTexture = TextureParser::copyTexture(localTexture);
            }
            localTexture->constantFlag = LegacyBoolean::TRUE_VALUE;
            for (tempTexture = localTexture; tempTexture->Next_Texture != nullptr;
                 tempTexture = tempTexture->Next_Texture) {
            }
            tempTexture->Next_Texture = (Texture *)constantPtr->constantData;
            constantPtr->constantData = (void *)localTexture;
        }
        return nullptr;
    }

    if (valueTokenId == Tokenizer::OBJECT_TOKEN) {
        constantPtr->constantData = (void *)ObjectParser::parseObject(ctx);
        constantPtr->constantType = ParseGlobals::OBJECT_CONSTANT;
        return nullptr;
    }
    if (valueTokenId == Tokenizer::SPHERE_TOKEN) {
        constantPtr->constantData = (void *)SphereParser::parseSphere(ctx);
        constantPtr->constantType = ParseGlobals::SPHERE_CONSTANT;
        return nullptr;
    }
    if (valueTokenId == Tokenizer::PLANE_TOKEN) {
        constantPtr->constantData = (void *)PlaneParser::parsePlane(ctx);
        constantPtr->constantType = ParseGlobals::PLANE_CONSTANT;
        return nullptr;
    }
    if (valueTokenId == Tokenizer::TRIANGLE_TOKEN) {
        constantPtr->constantData = (void *)TriangleParser::parseTriangle(ctx);
        constantPtr->constantType = ParseGlobals::TRIANGLE_CONSTANT;
        return nullptr;
    }
    if (valueTokenId == Tokenizer::SMOOTH_TRIANGLE_TOKEN) {
        constantPtr->constantData = (void *)SmoothTriangleParser::parseSmoothTriangle(ctx);
        constantPtr->constantType = ParseGlobals::SMOOTH_TRIANGLE_CONSTANT;
        return nullptr;
    }
    if (valueTokenId == Tokenizer::QUADRIC_TOKEN) {
        constantPtr->constantData = (void *)QuadricParser::parseQuadric(ctx);
        constantPtr->constantType = ParseGlobals::QUADRIC_CONSTANT;
        return nullptr;
    }
    if (valueTokenId == Tokenizer::CUBIC_TOKEN) {
        constantPtr->constantData = (void *)PolyParser::parsePoly(3, ctx);
        constantPtr->constantType = ParseGlobals::POLY_CONSTANT;
        return nullptr;
    }
    if (valueTokenId == Tokenizer::QUARTIC_TOKEN) {
        constantPtr->constantData = (void *)PolyParser::parsePoly(4, ctx);
        constantPtr->constantType = ParseGlobals::POLY_CONSTANT;
        return nullptr;
    }
    if (valueTokenId == Tokenizer::POLY_TOKEN) {
        constantPtr->constantData = (void *)PolyParser::parsePoly(0, ctx);
        constantPtr->constantType = ParseGlobals::POLY_CONSTANT;
        return nullptr;
    }
    if (valueTokenId == Tokenizer::HEIGHT_FIELD_TOKEN) {
        constantPtr->constantData = (void *)HeightFieldParser::parseHeightField(ctx);
        constantPtr->constantType = ParseGlobals::HEIGHT_FIELD_CONSTANT;
        return nullptr;
    }
    if (valueTokenId == Tokenizer::BOX_TOKEN) {
        constantPtr->constantData = (void *)BoxParser::parseBox(ctx);
        constantPtr->constantType = ParseGlobals::BOX_CONSTANT;
        return nullptr;
    }
    if (valueTokenId == Tokenizer::BLOB_TOKEN) {
        constantPtr->constantData = (void *)BlobParser::parseBlob(ctx);
        constantPtr->constantType = ParseGlobals::BLOB_CONSTANT;
        return nullptr;
    }
    if (valueTokenId == Tokenizer::BICUBIC_PATCH_TOKEN) {
        constantPtr->constantData = (void *)BicubicPatchParser::parseBicubicPatch(ctx);
        constantPtr->constantType = ParseGlobals::BICUBIC_PATCH_CONSTANT;
        return nullptr;
    }
    if (valueTokenId == Tokenizer::INTERSECTION_TOKEN) {
        constantPtr->constantData = (void *)ObjectParser::parseCsg(GeometryOperations::CSG_INTERSECTION_TYPE, ctx);
        constantPtr->constantType = ParseGlobals::CSG_INTERSECTION_CONSTANT;
        return nullptr;
    }
    if (valueTokenId == Tokenizer::UNION_TOKEN) {
        constantPtr->constantData = (void *)ObjectParser::parseCsg(GeometryOperations::CSG_UNION_TYPE, ctx);
        constantPtr->constantType = ParseGlobals::CSG_UNION_CONSTANT;
        return nullptr;
    }
    if (valueTokenId == Tokenizer::DIFFERENCE_TOKEN) {
        constantPtr->constantData = (void *)ObjectParser::parseCsg(GeometryOperations::CSG_DIFFERENCE_TYPE, ctx);
        constantPtr->constantType = ParseGlobals::CSG_DIFFERENCE_CONSTANT;
        return nullptr;
    }
    if (valueTokenId == Tokenizer::COMPOSITE_TOKEN) {
        constantPtr->constantData = (void *)ObjectParser::parseComposite(ctx);
        constantPtr->constantType = ParseGlobals::COMPOSITE_CONSTANT;
        return nullptr;
    }
    if (valueTokenId == Tokenizer::LIGHT_SOURCE_TOKEN) {
        constantPtr->constantData = (void *)LightSourceParser::parseLightSource(ctx);
        constantPtr->constantType = ParseGlobals::LIGHT_SOURCE_CONSTANT;
        return nullptr;
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
