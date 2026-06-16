#include "java/util/PriorityQueue.txx"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/numericalAnalysis/polynomial/PolynomialSolver.h"

#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/volume/polynomial/PolynomialShape.h"
#include "environment/scene/ModelBuilder.h"
#include "environment/scene/TranslatedBody.h"

#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/geometry/PolyParser.h"
#include "io/pov/material/TextureParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"

TranslatedBody *
PolyParser::parsePoly(int knownOrder)
{
    ParserContext ctx;
    return PolyParser::parsePoly(knownOrder, ctx);
}

TranslatedBody *
PolyParser::parsePoly(int knownOrder, ParserContext &ctx)
{
    (void)ctx;
    PolynomialShape *localShape;
    TranslatedBody *body = nullptr;
    Vector3Dd localVector;
    int constantId;
    int order;
    Material *localTexture;

    if (knownOrder > 0) {
        localShape = ModelBuilder::getPolyShape(
            knownOrder, PolynomialShape::termCountsByOrder());
        body = ModelBuilder::wrap(localShape);
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
                if (order < 2 || order > PolynomialSolver::MAX_ORDER) {
                    ParseErrorReporter::reportError("order of Poly is out of range", ctx);
                }
                localShape = ModelBuilder::getPolyShape(
                    order, PolynomialShape::termCountsByOrder());
                body = ModelBuilder::wrap(localShape);
                break;

            case Tokenizer::LEFT_ANGLE_TOKEN:
                ctx.tokenStream().ungetToken();
                if (localShape == nullptr) {
                    Logger::reportMessage("PolyParser", Logger::WARNING, "", "Need the order of the Poly");
                }
                PrimitiveParser::parseCoeffs(
                    localShape->order, &(localShape->Coeffs[0]));
                Exit_Flag = true;
                break;

            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        ParseGlobals::POLY_CONSTANT) {
                        body = (TranslatedBody *)GeometryOperations::copy(
                                (TransformableElement *)ctx.constants()[(int)constantId]
                                    .constantData);
                        localShape = (PolynomialShape *)body->geometry;
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
                localShape->setSturmFlag(1);
                break;

            case Tokenizer::TRANSLATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::translate(
                    body, &localVector);
                break;

            case Tokenizer::ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::rotate(
                    body, &localVector);
                break;

            case Tokenizer::SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::scale(
                    body, &localVector);
                break;

            case Tokenizer::INVERSE_TOKEN:
                GeometryOperations::invert(body);
                break;

            case Tokenizer::TEXTURE_TOKEN:
                localTexture = TextureParser::parseTexture(ctx);
                if (localTexture->isConstant()) {
                    localTexture = TextureParser::copyTexture(localTexture);
                }

                TextureParser::prependTextureLayers(localTexture, body->material);
                break;

            case Tokenizer::COLOUR_TOKEN:
                body->setShapeColor(ModelBuilder::getColor());
                PrimitiveParser::parseColor(body->shapeColor, ctx);
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    return body;
}
