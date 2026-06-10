#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "processing/polynomial/PolynomialConstants.h"
#include "processing/polynomial/PolynomialTermCounts.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/volume/polynomial/PolynomialShape.h"
#include "environment/scene/ModelBuilder.h"
#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"
#include "io/pov/texture/TextureParser.h"
#include "io/pov/geometry/PolyParser.h"

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
    int constantId;
    int order;
    Material *localTexture;

    if (knownOrder > 0) {
        localShape = ModelBuilder::getPolyShape(knownOrder, PolynomialTermCounts::table());
    } else {
        localShape = nullptr;
    }

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::DASH_TOKEN:
            case Tokenizer::PLUS_TOKEN:
            case Tokenizer::FLOAT_TOKEN:
                ctx.tokenStream().ungetToken();
                if (localShape != nullptr) {
                    ParseErrorReporter::reportError(
                        "The order of a polynomial may not be specified twice", ctx);
                }
                order = (int)PrimitiveParser::parseFloat(ctx);
                if (order < 2 || order > PolynomialConstants::MAX_ORDER) {
                    ParseErrorReporter::reportError("Order of Poly is out of range", ctx);
                }
                localShape = ModelBuilder::getPolyShape(order, PolynomialTermCounts::table());
                break;

            case Tokenizer::LEFT_ANGLE_TOKEN:
                ctx.tokenStream().ungetToken();
                if (localShape == nullptr) {
                    Logger::reportMessage("PolyParser", Logger::WARNING, "", "Need the order of the Poly");
                }
                PrimitiveParser::parseCoeffs(
                    localShape->Order, &(localShape->Coeffs[0]));
                Exit_Flag = true;
                break;

            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        ParseGlobals::POLY_CONSTANT) {
                        localShape =
                            (PolynomialShape *)GeometryOperations::copy(
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

            case Tokenizer::STURM_TOKEN:
                localShape->sturmFlag = 1;
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
