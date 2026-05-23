#include "io/pov/ParserContext.h"
#include "io/pov/geometryParser/PolyParser.h"
#include "common/logger/Logger.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/volume/polynomial/PolynomialShape.h"
#include "environment/scene/ObjectUtils.h"
#include "io/pov/Parse.h"
#include "io/pov/ParseHelpers.h"
#include "io/pov/PrimitiveParser.h"
#include "io/pov/SceneConfigParser.h"
#include "io/pov/mediaParser/TextureParser.h"


Geometry *
PolyParser::parsePoly(int knownOrder)
{
    ParserContext ctx;
    return PolyParser::parsePoly(knownOrder, ctx);
}

Geometry *
PolyParser::parsePoly(int knownOrder, ParserContext &ctx)
{
    (void)ctx;
    PolynomialShape *localShape;
    Vector3Dd localVector;
    CONSTANT constantId;
    int order;
    Texture *localTexture;

    if (knownOrder > 0) {
        localShape = SceneFactory::getPolyShape(knownOrder, ctx.termCounts());
    } else {
        localShape = nullptr;
    }

    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (ctx.token().tokenId) {
            case DASH_TOKEN:
            case PLUS_TOKEN:
            case FLOAT_TOKEN:
                Tokenizer::ungetToken();
                if (localShape != nullptr) {
                    ParseErrorReporter::Error(
                        "The order of a polynomial may not be specified twice", ctx);
                }
                order = (int)PrimitiveParser::parseFloat(ctx);
                if (order < 2 || order > MAX_ORDER) {
                    ParseErrorReporter::Error("Order of Poly is out of range", ctx);
                }
                localShape = SceneFactory::getPolyShape(order, ctx.termCounts());
                break;

            case LEFT_ANGLE_TOKEN:
                Tokenizer::ungetToken();
                if (localShape == nullptr) {
                    Logger::info("Need the order of the Poly");
                }
                PrimitiveParser::parseCoeffs(
                    localShape->Order, &(localShape->Coeffs[0]));
                Exit_Flag = TRUE;
                break;

            case IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant(ctx)) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        POLY_CONSTANT) {
                        localShape =
                            (PolynomialShape *)GeometryOperations::copy(
                                (SimpleBody *)ctx.constants()[(int)constantId]
                                    .constantData);
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::Undeclared(ctx);
                }
                Exit_Flag = TRUE;
                break;

            default:
                ParseErrorReporter::parseError(LEFT_ANGLE_TOKEN, ctx);
                break;
            }
        }
    }

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (ctx.token().tokenId) {
            case RIGHT_CURLY_TOKEN:
                Exit_Flag = TRUE;
                break;

            case STURM_TOKEN:
                localShape->sturmFlag = 1;
                break;

            case TRANSLATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::translate(
                    (SimpleBody *)localShape, &localVector);
                break;

            case ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::rotate(
                    (SimpleBody *)localShape, &localVector);
                break;

            case SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::scale(
                    (SimpleBody *)localShape, &localVector);
                break;

            case INVERSE_TOKEN:
                GeometryOperations::invert((SimpleBody *)localShape);
                break;

            case TEXTURE_TOKEN:
                localTexture = TextureParser::parseTexture(ctx);
                if (localTexture->constantFlag) {
                    localTexture = TextureParser::copyTexture(localTexture);
                }

                ObjectUtils::link((SimpleBody *)localTexture,
                    (SimpleBody **)&localTexture->Next_Texture,
                    (SimpleBody **)&localShape->Shape_Texture);
                break;

            case COLOUR_TOKEN:
                localShape->Shape_Colour = SceneFactory::getColour();
                PrimitiveParser::parseColour(localShape->Shape_Colour, ctx);
                break;

            default:
                ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    return ((Geometry *)localShape);
}
