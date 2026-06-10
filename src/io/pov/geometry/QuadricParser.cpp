#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/volume/Quadric.h"
#include "environment/scene/ModelBuilder.h"
#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"
#include "io/pov/texture/TextureParser.h"
#include "io/pov/geometry/QuadricParser.h"


Geometry *
QuadricParser::parseQuadric()
{
    ParserContext ctx;
    return QuadricParser::parseQuadric(ctx);
}

Geometry *
QuadricParser::parseQuadric(ParserContext &ctx)
{
    (void)ctx;
    Quadric *localShape;
    Vector3Dd localVector;
    int constantId;
    Material *localTexture;

    localShape = nullptr;

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::LEFT_ANGLE_TOKEN:
                ctx.tokenStream().ungetToken();
                localShape = ModelBuilder::getQuadricShape();
                PrimitiveParser::parseVector(&(localShape->object2Terms), ctx);
                PrimitiveParser::parseVector(&(localShape->objectMixedTerms), ctx);
                PrimitiveParser::parseVector(&(localShape->objectTerms), ctx);
                (localShape->objectConstant) = PrimitiveParser::parseFloat(ctx);
                localShape->nonZeroSquareTerm =
                    !((localShape->object2Terms.x() == 0.0) &&
                        (localShape->object2Terms.y() == 0.0) &&
                        (localShape->object2Terms.z() == 0.0) &&
                        (localShape->objectMixedTerms.x() == 0.0) &&
                        (localShape->objectMixedTerms.y() == 0.0) &&
                        (localShape->objectMixedTerms.z() == 0.0));
                Exit_Flag = true;
                break;

            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        ParseGlobals::QUADRIC_CONSTANT) {
                        localShape = (Quadric *)GeometryOperations::copy(
                            (SimpleBody *)ctx.constants()[(int)constantId]
                                .constantData);
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::reportUndeclared(ctx);
                }
                Exit_Flag = true;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::LEFT_ANGLE_TOKEN, ctx);
                break;
            }
        }
    }

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = true;
                break;

            case Tokenizer::TRANSLATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::translate(
                    (SimpleBody *)localShape, &localVector);
                break;

            case Tokenizer::ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::rotate(
                    (SimpleBody *)localShape, &localVector);
                break;

            case Tokenizer::SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::scale(
                    (SimpleBody *)localShape, &localVector);
                break;

            case Tokenizer::INVERSE_TOKEN:
                GeometryOperations::invert((SimpleBody *)localShape);
                break;

            case Tokenizer::TEXTURE_TOKEN:
                localTexture = TextureParser::parseTexture(ctx);
                if (localTexture->constantFlag) {
                    localTexture = TextureParser::copyTexture(localTexture);
                }
                TextureParser::prependTextureLayers(localTexture, localShape->Shape_Texture);
                break;

            case Tokenizer::COLOUR_TOKEN:
                localShape->shapeColor = ModelBuilder::getColor();
                PrimitiveParser::parseColor(localShape->shapeColor, ctx);
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    return ((Geometry *)localShape);
}
