#include "io/pov/ast/AstObjectParser.h"

#include "io/Tokenizer.h"
#include "io/pov/ParseErrorReporter.h"
#include "io/pov/ParseHelpers.h"
#include "io/pov/ParserContext.h"
#include "io/pov/ast/AstPrimitiveParser.h"

bool
AstObjectParser::appendTransformOrFail(ParserContext &ctx, AstTransform *arr,
    int &count,
    AstTransformKind kind, const AstVector3 &v)
{
    if (!AstNodes::appendTransform(arr, count, kind, v)) {
        ParseErrorReporter::Error("Too many AST transforms", ctx);
        return false;
    }
    return true;
}



AstNode *AstObjectParser::parseShapeNode(ParserContext &ctx)
{
    ctx.tokenStream().getToken();
    AstNode *node = AstObjectParser::parseShapeNodeFromToken(ctx, ctx.token().tokenId);
    if (node == nullptr) {
        ParseErrorReporter::parseError(Tokenizer::SPHERE_TOKEN, ctx);
    }
    return node;
}

AstNode *AstObjectParser::parseShapeNodeFromToken(ParserContext &ctx, int tokenId)
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

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    int baseParsed = LegacyBoolean::FALSE_VALUE;
    while (!baseParsed) {
        ctx.tokenStream().getToken();
        switch (ctx.token().tokenId) {
        case Tokenizer::LEFT_ANGLE_TOKEN:
            ctx.tokenStream().ungetToken();
            node->center = AstPrimitiveParser::parseVector(ctx);
            node->radius = AstPrimitiveParser::parseFloat(ctx);
            node->hasInlineData = true;
            baseParsed = LegacyBoolean::TRUE_VALUE;
            break;

        case Tokenizer::IDENTIFIER_TOKEN:
            node->hasReference = true;
            node->referenceConstantId = ctx.token().identifierNumber;
            baseParsed = LegacyBoolean::TRUE_VALUE;
            break;

        default:
            ParseErrorReporter::parseError(Tokenizer::LEFT_ANGLE_TOKEN, ctx);
            break;
        }
    }

    int done = LegacyBoolean::FALSE_VALUE;
    while (!done) {
        ctx.tokenStream().getToken();
        switch (ctx.token().tokenId) {
        case Tokenizer::RIGHT_CURLY_TOKEN:
            done = LegacyBoolean::TRUE_VALUE;
            break;
        case Tokenizer::TRANSLATE_TOKEN:
            AstObjectParser::appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_TRANSLATE, AstPrimitiveParser::parseVector(ctx));
            break;
        case Tokenizer::ROTATE_TOKEN:
            AstObjectParser::appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_ROTATE, AstPrimitiveParser::parseVector(ctx));
            break;
        case Tokenizer::SCALE_TOKEN:
            AstObjectParser::appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_SCALE, AstPrimitiveParser::parseVector(ctx));
            break;
        case Tokenizer::INVERSE_TOKEN: {
            AstVector3 z = {0.0, 0.0, 0.0};
            AstObjectParser::appendTransformOrFail(
                ctx, node->transforms, node->transformCount, AST_INVERSE, z);
            break;
        }
        case Tokenizer::COLOUR_TOKEN:
            node->colour = AstPrimitiveParser::parseColour(ctx);
            node->hasColour = true;
            break;
        default:
            ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
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

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    int baseParsed = LegacyBoolean::FALSE_VALUE;
    while (!baseParsed) {
        ctx.tokenStream().getToken();
        switch (ctx.token().tokenId) {
        case Tokenizer::LEFT_ANGLE_TOKEN:
            ctx.tokenStream().ungetToken();
            node->center = AstPrimitiveParser::parseVector(ctx);
            node->colour.r = 1.0;
            node->colour.g = 1.0;
            node->colour.b = 1.0;
            node->colour.a = 0.0;
            ParseHelpers::getExpectedToken(Tokenizer::COLOUR_TOKEN, ctx);
            node->colour = AstPrimitiveParser::parseColour(ctx);
            node->hasInlineData = true;
            baseParsed = LegacyBoolean::TRUE_VALUE;
            break;

        case Tokenizer::IDENTIFIER_TOKEN:
            node->hasReference = true;
            node->referenceConstantId = ctx.token().identifierNumber;
            baseParsed = LegacyBoolean::TRUE_VALUE;
            break;

        default:
            ParseErrorReporter::parseError(Tokenizer::LEFT_ANGLE_TOKEN, ctx);
            break;
        }
    }

    int done = LegacyBoolean::FALSE_VALUE;
    while (!done) {
        ctx.tokenStream().getToken();
        switch (ctx.token().tokenId) {
        case Tokenizer::RIGHT_CURLY_TOKEN:
            done = LegacyBoolean::TRUE_VALUE;
            break;
        case Tokenizer::TRANSLATE_TOKEN:
            AstObjectParser::appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_TRANSLATE, AstPrimitiveParser::parseVector(ctx));
            break;
        case Tokenizer::ROTATE_TOKEN:
            AstObjectParser::appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_ROTATE, AstPrimitiveParser::parseVector(ctx));
            break;
        case Tokenizer::SCALE_TOKEN:
            AstObjectParser::appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_SCALE, AstPrimitiveParser::parseVector(ctx));
            break;
        case Tokenizer::POINT_AT_TOKEN:
            node->pointsAt = AstPrimitiveParser::parseVector(ctx);
            break;
        case Tokenizer::TIGHTNESS_TOKEN:
            node->tightness = AstPrimitiveParser::parseFloat(ctx);
            break;
        case Tokenizer::RADIUS_TOKEN:
            node->radiusDegrees = AstPrimitiveParser::parseFloat(ctx);
            break;
        case Tokenizer::FALLOFF_TOKEN:
            node->falloffDegrees = AstPrimitiveParser::parseFloat(ctx);
            break;
        case Tokenizer::COLOUR_TOKEN:
            node->colour = AstPrimitiveParser::parseColour(ctx);
            break;
        case Tokenizer::SPOTLIGHT_TOKEN:
            node->hasSpotlight = true;
            break;
        default:
            ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
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

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    int done = LegacyBoolean::FALSE_VALUE;
    while (!done) {
        ctx.tokenStream().getToken();
        switch (ctx.token().tokenId) {
        case Tokenizer::IDENTIFIER_TOKEN:
            node->hasReference = true;
            node->referenceConstantId = ctx.token().identifierNumber;
            break;
        case Tokenizer::SPHERE_TOKEN:
        case Tokenizer::LIGHT_SOURCE_TOKEN:
        case Tokenizer::UNION_TOKEN:
        case Tokenizer::INTERSECTION_TOKEN:
        case Tokenizer::DIFFERENCE_TOKEN: {
            AstNode *child = AstObjectParser::parseShapeNodeFromToken(ctx, ctx.token().tokenId);
            if (child == nullptr ||
                !AstNodes::appendNode(
                    node->children, node->childCount, AstLimits::MAX_AST_CHILDREN, child)) {
                ParseErrorReporter::Error("Too many CSG children", ctx);
            }
            break;
        }
        case Tokenizer::TRANSLATE_TOKEN:
            AstObjectParser::appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_TRANSLATE, AstPrimitiveParser::parseVector(ctx));
            break;
        case Tokenizer::ROTATE_TOKEN:
            AstObjectParser::appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_ROTATE, AstPrimitiveParser::parseVector(ctx));
            break;
        case Tokenizer::SCALE_TOKEN:
            AstObjectParser::appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_SCALE, AstPrimitiveParser::parseVector(ctx));
            break;
        case Tokenizer::INVERSE_TOKEN: {
            AstVector3 z = {0.0, 0.0, 0.0};
            AstObjectParser::appendTransformOrFail(
                ctx, node->transforms, node->transformCount, AST_INVERSE, z);
            break;
        }
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

AstObjectNode *
AstObjectParser::parseObject(ParserContext &ctx)
{
    AstObjectNode *node = new AstObjectNode();
    node->sourceLine = ctx.token().tokenLineNo + 1;
    node->sourceFile = ctx.token().Filename;

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    int baseParsed = LegacyBoolean::FALSE_VALUE;
    while (!baseParsed) {
        ctx.tokenStream().getToken();
        switch (ctx.token().tokenId) {
        case Tokenizer::IDENTIFIER_TOKEN:
            node->hasReference = true;
            node->referenceConstantId = ctx.token().identifierNumber;
            baseParsed = LegacyBoolean::TRUE_VALUE;
            break;
        default:
            ctx.tokenStream().ungetToken();
            node->shape = AstObjectParser::parseShapeNode(ctx);
            baseParsed = LegacyBoolean::TRUE_VALUE;
            break;
        }
    }

    int done = LegacyBoolean::FALSE_VALUE;
    while (!done) {
        ctx.tokenStream().getToken();
        switch (ctx.token().tokenId) {
        case Tokenizer::BOUNDED_TOKEN:
            ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);
            while (LegacyBoolean::TRUE_VALUE) {
                ctx.tokenStream().getToken();
                if (ctx.token().tokenId == Tokenizer::RIGHT_CURLY_TOKEN) {
                    break;
                }
                ctx.tokenStream().ungetToken();
                AstNode *s = AstObjectParser::parseShapeNode(ctx);
                if (!AstNodes::appendNode(node->boundedBy, node->boundedByCount,
                        AstLimits::MAX_AST_CHILDREN, s)) {
                    ParseErrorReporter::Error("Too many bounded_by shapes", ctx);
                }
            }
            break;
        case Tokenizer::CLIPPED_TOKEN:
            ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);
            while (LegacyBoolean::TRUE_VALUE) {
                ctx.tokenStream().getToken();
                if (ctx.token().tokenId == Tokenizer::RIGHT_CURLY_TOKEN) {
                    break;
                }
                ctx.tokenStream().ungetToken();
                AstNode *s = AstObjectParser::parseShapeNode(ctx);
                if (!AstNodes::appendNode(node->clippedBy, node->clippedByCount,
                        AstLimits::MAX_AST_CHILDREN, s)) {
                    ParseErrorReporter::Error("Too many clipped_by shapes", ctx);
                }
            }
            break;
        case Tokenizer::TRANSLATE_TOKEN:
            AstObjectParser::appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_TRANSLATE, AstPrimitiveParser::parseVector(ctx));
            break;
        case Tokenizer::ROTATE_TOKEN:
            AstObjectParser::appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_ROTATE, AstPrimitiveParser::parseVector(ctx));
            break;
        case Tokenizer::SCALE_TOKEN:
            AstObjectParser::appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_SCALE, AstPrimitiveParser::parseVector(ctx));
            break;
        case Tokenizer::INVERSE_TOKEN: {
            AstVector3 z = {0.0, 0.0, 0.0};
            AstObjectParser::appendTransformOrFail(
                ctx, node->transforms, node->transformCount, AST_INVERSE, z);
            break;
        }
        case Tokenizer::NO_SHADOW_TOKEN:
            node->noShadow = true;
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

AstCompositeNode *
AstObjectParser::parseComposite(ParserContext &ctx)
{
    AstCompositeNode *node = new AstCompositeNode();
    node->sourceLine = ctx.token().tokenLineNo + 1;
    node->sourceFile = ctx.token().Filename;

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    while (LegacyBoolean::TRUE_VALUE) {
        ctx.tokenStream().getToken();
        switch (ctx.token().tokenId) {
        case Tokenizer::IDENTIFIER_TOKEN:
            node->hasReference = true;
            node->referenceConstantId = ctx.token().identifierNumber;
            break;
        case Tokenizer::OBJECT_TOKEN: {
            AstNode *child = AstObjectParser::parseObject(ctx);
            if (!AstNodes::appendNode(
                    node->children, node->childCount, AstLimits::MAX_AST_CHILDREN, child)) {
                ParseErrorReporter::Error("Too many composite children", ctx);
            }
            break;
        }
        case Tokenizer::COMPOSITE_TOKEN: {
            AstNode *child = AstObjectParser::parseComposite(ctx);
            if (!AstNodes::appendNode(
                    node->children, node->childCount, AstLimits::MAX_AST_CHILDREN, child)) {
                ParseErrorReporter::Error("Too many composite children", ctx);
            }
            break;
        }
        case Tokenizer::RIGHT_CURLY_TOKEN:
            return node;
        default:
            ctx.tokenStream().ungetToken();
            goto parse_modifiers;
        }
    }

parse_modifiers:
    while (LegacyBoolean::TRUE_VALUE) {
        ctx.tokenStream().getToken();
        switch (ctx.token().tokenId) {
        case Tokenizer::BOUNDED_TOKEN:
            ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);
            while (LegacyBoolean::TRUE_VALUE) {
                ctx.tokenStream().getToken();
                if (ctx.token().tokenId == Tokenizer::RIGHT_CURLY_TOKEN) {
                    break;
                }
                ctx.tokenStream().ungetToken();
                AstNode *s = AstObjectParser::parseShapeNode(ctx);
                if (!AstNodes::appendNode(node->boundedBy, node->boundedByCount,
                        AstLimits::MAX_AST_CHILDREN, s)) {
                    ParseErrorReporter::Error("Too many bounded_by shapes", ctx);
                }
            }
            break;
        case Tokenizer::CLIPPED_TOKEN:
            ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);
            while (LegacyBoolean::TRUE_VALUE) {
                ctx.tokenStream().getToken();
                if (ctx.token().tokenId == Tokenizer::RIGHT_CURLY_TOKEN) {
                    break;
                }
                ctx.tokenStream().ungetToken();
                AstNode *s = AstObjectParser::parseShapeNode(ctx);
                if (!AstNodes::appendNode(node->clippedBy, node->clippedByCount,
                        AstLimits::MAX_AST_CHILDREN, s)) {
                    ParseErrorReporter::Error("Too many clipped_by shapes", ctx);
                }
            }
            break;
        case Tokenizer::TRANSLATE_TOKEN:
            AstObjectParser::appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_TRANSLATE, AstPrimitiveParser::parseVector(ctx));
            break;
        case Tokenizer::ROTATE_TOKEN:
            AstObjectParser::appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_ROTATE, AstPrimitiveParser::parseVector(ctx));
            break;
        case Tokenizer::SCALE_TOKEN:
            AstObjectParser::appendTransformOrFail(ctx, node->transforms, node->transformCount,
                AST_SCALE, AstPrimitiveParser::parseVector(ctx));
            break;
        case Tokenizer::INVERSE_TOKEN: {
            AstVector3 z = {0.0, 0.0, 0.0};
            AstObjectParser::appendTransformOrFail(
                ctx, node->transforms, node->transformCount, AST_INVERSE, z);
            break;
        }
        case Tokenizer::RIGHT_CURLY_TOKEN:
            return node;
        default:
            ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
            break;
        }
    }
}
