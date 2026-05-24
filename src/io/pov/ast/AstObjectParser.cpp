#include "io/pov/ast/AstObjectParser.h"

#include "io/Tokenizer.h"
#include "io/pov/ParseErrorReporter.h"
#include "io/pov/ParseHelpers.h"
#include "io/pov/ParserContext.h"
#include "io/pov/ast/AstPrimitiveParser.h"

static bool
appendTransformOrFail(ParserContext &ctx, AstTransform *arr, int &count,
    AstTransformKind kind, const AstVector3 &v)
{
    if (!AstNodes::appendTransform(arr, count, kind, v)) {
        ParseErrorReporter::Error("Too many AST transforms", ctx);
        return false;
    }
    return true;
}

static AstNode *parseShapeNodeFromToken(ParserContext &ctx, int tokenId);

static AstNode *
parseShapeNode(ParserContext &ctx)
{
    ctx.tokenStream().getToken();
    AstNode *node = parseShapeNodeFromToken(ctx, ctx.token().tokenId);
    if (node == nullptr) {
        ParseErrorReporter::parseError(SPHERE_TOKEN, ctx);
    }
    return node;
}

static AstNode *
parseShapeNodeFromToken(ParserContext &ctx, int tokenId)
{
    switch (tokenId) {
    case SPHERE_TOKEN:
        return AstObjectParser::parseSphere(ctx);
    case LIGHT_SOURCE_TOKEN:
        return AstObjectParser::parseLightSource(ctx);
    case UNION_TOKEN:
        return AstObjectParser::parseCsg(ctx, AST_CSG_UNION);
    case INTERSECTION_TOKEN:
        return AstObjectParser::parseCsg(ctx, AST_CSG_INTERSECTION);
    case DIFFERENCE_TOKEN:
        return AstObjectParser::parseCsg(ctx, AST_CSG_DIFFERENCE);
    default:
        return nullptr;
    }
}

AstSphereNode *
AstObjectParser::parseSphere(ParserContext &ctx)
{
    AstSphereNode *node = new AstSphereNode();
    node->sourceLine = ctx.token().tokenLineNo + 1;
    node->sourceFile = ctx.token().Filename;

    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);

    int baseParsed = FALSE;
    while (!baseParsed) {
        ctx.tokenStream().getToken();
        switch (ctx.token().tokenId) {
        case LEFT_ANGLE_TOKEN:
            ctx.tokenStream().ungetToken();
            node->center = AstPrimitiveParser::parseVector(ctx);
            node->radius = AstPrimitiveParser::parseFloat(ctx);
            node->hasInlineData = true;
            baseParsed = TRUE;
            break;

        case IDENTIFIER_TOKEN:
            node->hasReference = true;
            node->referenceConstantId = ctx.token().identifierNumber;
            baseParsed = TRUE;
            break;

        default:
            ParseErrorReporter::parseError(LEFT_ANGLE_TOKEN, ctx);
            break;
        }
    }

    int done = FALSE;
    while (!done) {
        ctx.tokenStream().getToken();
        switch (ctx.token().tokenId) {
        case RIGHT_CURLY_TOKEN:
            done = TRUE;
            break;
        case TRANSLATE_TOKEN:
            appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_TRANSLATE, AstPrimitiveParser::parseVector(ctx));
            break;
        case ROTATE_TOKEN:
            appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_ROTATE, AstPrimitiveParser::parseVector(ctx));
            break;
        case SCALE_TOKEN:
            appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_SCALE, AstPrimitiveParser::parseVector(ctx));
            break;
        case INVERSE_TOKEN: {
            AstVector3 z = {0.0, 0.0, 0.0};
            appendTransformOrFail(
                ctx, node->transforms, node->transformCount, AST_INVERSE, z);
            break;
        }
        case COLOUR_TOKEN:
            node->colour = AstPrimitiveParser::parseColour(ctx);
            node->hasColour = true;
            break;
        default:
            ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN, ctx);
            break;
        }
    }

    return node;
}

AstLightSourceNode *
AstObjectParser::parseLightSource(ParserContext &ctx)
{
    AstLightSourceNode *node = new AstLightSourceNode();
    node->sourceLine = ctx.token().tokenLineNo + 1;
    node->sourceFile = ctx.token().Filename;

    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);

    int baseParsed = FALSE;
    while (!baseParsed) {
        ctx.tokenStream().getToken();
        switch (ctx.token().tokenId) {
        case LEFT_ANGLE_TOKEN:
            ctx.tokenStream().ungetToken();
            node->center = AstPrimitiveParser::parseVector(ctx);
            node->colour.r = 1.0;
            node->colour.g = 1.0;
            node->colour.b = 1.0;
            node->colour.a = 0.0;
            ParseHelpers::getExpectedToken(COLOUR_TOKEN, ctx);
            node->colour = AstPrimitiveParser::parseColour(ctx);
            node->hasInlineData = true;
            baseParsed = TRUE;
            break;

        case IDENTIFIER_TOKEN:
            node->hasReference = true;
            node->referenceConstantId = ctx.token().identifierNumber;
            baseParsed = TRUE;
            break;

        default:
            ParseErrorReporter::parseError(LEFT_ANGLE_TOKEN, ctx);
            break;
        }
    }

    int done = FALSE;
    while (!done) {
        ctx.tokenStream().getToken();
        switch (ctx.token().tokenId) {
        case RIGHT_CURLY_TOKEN:
            done = TRUE;
            break;
        case TRANSLATE_TOKEN:
            appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_TRANSLATE, AstPrimitiveParser::parseVector(ctx));
            break;
        case ROTATE_TOKEN:
            appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_ROTATE, AstPrimitiveParser::parseVector(ctx));
            break;
        case SCALE_TOKEN:
            appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_SCALE, AstPrimitiveParser::parseVector(ctx));
            break;
        case POINT_AT_TOKEN:
            node->pointsAt = AstPrimitiveParser::parseVector(ctx);
            break;
        case TIGHTNESS_TOKEN:
            node->tightness = AstPrimitiveParser::parseFloat(ctx);
            break;
        case RADIUS_TOKEN:
            node->radiusDegrees = AstPrimitiveParser::parseFloat(ctx);
            break;
        case FALLOFF_TOKEN:
            node->falloffDegrees = AstPrimitiveParser::parseFloat(ctx);
            break;
        case COLOUR_TOKEN:
            node->colour = AstPrimitiveParser::parseColour(ctx);
            break;
        case SPOTLIGHT_TOKEN:
            node->hasSpotlight = true;
            break;
        default:
            ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN, ctx);
            break;
        }
    }

    return node;
}

AstCsgNode *
AstObjectParser::parseCsg(ParserContext &ctx, AstCsgOpKind op)
{
    AstCsgNode *node = new AstCsgNode();
    node->op = op;
    node->sourceLine = ctx.token().tokenLineNo + 1;
    node->sourceFile = ctx.token().Filename;

    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);

    int done = FALSE;
    while (!done) {
        ctx.tokenStream().getToken();
        switch (ctx.token().tokenId) {
        case IDENTIFIER_TOKEN:
            node->hasReference = true;
            node->referenceConstantId = ctx.token().identifierNumber;
            break;
        case SPHERE_TOKEN:
        case LIGHT_SOURCE_TOKEN:
        case UNION_TOKEN:
        case INTERSECTION_TOKEN:
        case DIFFERENCE_TOKEN: {
            AstNode *child = parseShapeNodeFromToken(ctx, ctx.token().tokenId);
            if (child == nullptr ||
                !AstNodes::appendNode(
                    node->children, node->childCount, MAX_AST_CHILDREN, child)) {
                ParseErrorReporter::Error("Too many CSG children", ctx);
            }
            break;
        }
        case TRANSLATE_TOKEN:
            appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_TRANSLATE, AstPrimitiveParser::parseVector(ctx));
            break;
        case ROTATE_TOKEN:
            appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_ROTATE, AstPrimitiveParser::parseVector(ctx));
            break;
        case SCALE_TOKEN:
            appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_SCALE, AstPrimitiveParser::parseVector(ctx));
            break;
        case INVERSE_TOKEN: {
            AstVector3 z = {0.0, 0.0, 0.0};
            appendTransformOrFail(
                ctx, node->transforms, node->transformCount, AST_INVERSE, z);
            break;
        }
        case RIGHT_CURLY_TOKEN:
            done = TRUE;
            break;
        default:
            ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN, ctx);
            break;
        }
    }

    return node;
}

AstObjectNode *
AstObjectParser::parseObject(ParserContext &ctx)
{
    AstObjectNode *node = new AstObjectNode();
    node->sourceLine = ctx.token().tokenLineNo + 1;
    node->sourceFile = ctx.token().Filename;

    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);

    int baseParsed = FALSE;
    while (!baseParsed) {
        ctx.tokenStream().getToken();
        switch (ctx.token().tokenId) {
        case IDENTIFIER_TOKEN:
            node->hasReference = true;
            node->referenceConstantId = ctx.token().identifierNumber;
            baseParsed = TRUE;
            break;
        default:
            ctx.tokenStream().ungetToken();
            node->shape = parseShapeNode(ctx);
            baseParsed = TRUE;
            break;
        }
    }

    int done = FALSE;
    while (!done) {
        ctx.tokenStream().getToken();
        switch (ctx.token().tokenId) {
        case BOUNDED_TOKEN:
            ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);
            while (TRUE) {
                ctx.tokenStream().getToken();
                if (ctx.token().tokenId == RIGHT_CURLY_TOKEN) {
                    break;
                }
                ctx.tokenStream().ungetToken();
                AstNode *s = parseShapeNode(ctx);
                if (!AstNodes::appendNode(node->boundedBy, node->boundedByCount,
                        MAX_AST_CHILDREN, s)) {
                    ParseErrorReporter::Error("Too many bounded_by shapes", ctx);
                }
            }
            break;
        case CLIPPED_TOKEN:
            ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);
            while (TRUE) {
                ctx.tokenStream().getToken();
                if (ctx.token().tokenId == RIGHT_CURLY_TOKEN) {
                    break;
                }
                ctx.tokenStream().ungetToken();
                AstNode *s = parseShapeNode(ctx);
                if (!AstNodes::appendNode(node->clippedBy, node->clippedByCount,
                        MAX_AST_CHILDREN, s)) {
                    ParseErrorReporter::Error("Too many clipped_by shapes", ctx);
                }
            }
            break;
        case TRANSLATE_TOKEN:
            appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_TRANSLATE, AstPrimitiveParser::parseVector(ctx));
            break;
        case ROTATE_TOKEN:
            appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_ROTATE, AstPrimitiveParser::parseVector(ctx));
            break;
        case SCALE_TOKEN:
            appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_SCALE, AstPrimitiveParser::parseVector(ctx));
            break;
        case INVERSE_TOKEN: {
            AstVector3 z = {0.0, 0.0, 0.0};
            appendTransformOrFail(
                ctx, node->transforms, node->transformCount, AST_INVERSE, z);
            break;
        }
        case NO_SHADOW_TOKEN:
            node->noShadow = true;
            break;
        case RIGHT_CURLY_TOKEN:
            done = TRUE;
            break;
        default:
            ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN, ctx);
            break;
        }
    }

    return node;
}

AstCompositeNode *
AstObjectParser::parseComposite(ParserContext &ctx)
{
    AstCompositeNode *node = new AstCompositeNode();
    node->sourceLine = ctx.token().tokenLineNo + 1;
    node->sourceFile = ctx.token().Filename;

    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);

    while (TRUE) {
        ctx.tokenStream().getToken();
        switch (ctx.token().tokenId) {
        case IDENTIFIER_TOKEN:
            node->hasReference = true;
            node->referenceConstantId = ctx.token().identifierNumber;
            break;
        case OBJECT_TOKEN: {
            AstNode *child = AstObjectParser::parseObject(ctx);
            if (!AstNodes::appendNode(
                    node->children, node->childCount, MAX_AST_CHILDREN, child)) {
                ParseErrorReporter::Error("Too many composite children", ctx);
            }
            break;
        }
        case COMPOSITE_TOKEN: {
            AstNode *child = AstObjectParser::parseComposite(ctx);
            if (!AstNodes::appendNode(
                    node->children, node->childCount, MAX_AST_CHILDREN, child)) {
                ParseErrorReporter::Error("Too many composite children", ctx);
            }
            break;
        }
        case RIGHT_CURLY_TOKEN:
            return node;
        default:
            ctx.tokenStream().ungetToken();
            goto parse_modifiers;
        }
    }

parse_modifiers:
    while (TRUE) {
        ctx.tokenStream().getToken();
        switch (ctx.token().tokenId) {
        case BOUNDED_TOKEN:
            ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);
            while (TRUE) {
                ctx.tokenStream().getToken();
                if (ctx.token().tokenId == RIGHT_CURLY_TOKEN) {
                    break;
                }
                ctx.tokenStream().ungetToken();
                AstNode *s = parseShapeNode(ctx);
                if (!AstNodes::appendNode(node->boundedBy, node->boundedByCount,
                        MAX_AST_CHILDREN, s)) {
                    ParseErrorReporter::Error("Too many bounded_by shapes", ctx);
                }
            }
            break;
        case CLIPPED_TOKEN:
            ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);
            while (TRUE) {
                ctx.tokenStream().getToken();
                if (ctx.token().tokenId == RIGHT_CURLY_TOKEN) {
                    break;
                }
                ctx.tokenStream().ungetToken();
                AstNode *s = parseShapeNode(ctx);
                if (!AstNodes::appendNode(node->clippedBy, node->clippedByCount,
                        MAX_AST_CHILDREN, s)) {
                    ParseErrorReporter::Error("Too many clipped_by shapes", ctx);
                }
            }
            break;
        case TRANSLATE_TOKEN:
            appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_TRANSLATE, AstPrimitiveParser::parseVector(ctx));
            break;
        case ROTATE_TOKEN:
            appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_ROTATE, AstPrimitiveParser::parseVector(ctx));
            break;
        case SCALE_TOKEN:
            appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_SCALE, AstPrimitiveParser::parseVector(ctx));
            break;
        case INVERSE_TOKEN: {
            AstVector3 z = {0.0, 0.0, 0.0};
            appendTransformOrFail(
                ctx, node->transforms, node->transformCount, AST_INVERSE, z);
            break;
        }
        case RIGHT_CURLY_TOKEN:
            return node;
        default:
            ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN, ctx);
            break;
        }
    }
}
