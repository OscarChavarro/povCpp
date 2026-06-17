#include "java/util/PriorityQueue.txx"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/numericalAnalysis/polynomial/PolynomialSolver.h"

#include "environment/geometry/volume/polynomial/PolynomialShape.h"
#include "environment/scene/ModelBuilder.h"
#include "environment/geometry/SimpleBody.h"

#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/geometry/PolyParser.h"
#include "io/pov/material/TextureParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"

SimpleBody *
PolyParser::parsePoly(int knownOrder)
{
    ParserContext ctx;
    return PolyParser::parsePoly(knownOrder, ctx);
}

SimpleBody *
PolyParser::parsePoly(int knownOrder, ParserContext &ctx)
{
    (void)ctx;
    PolynomialShape *localShape;
    SimpleBody *body = nullptr;
    Vector3Dd localVector;
    int constantId;
    int order;
    PovrayMaterial *localTexture;

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
                        body = (SimpleBody *)((TransformableElement *)ctx.constants()[(int)constantId]
                                    .constantData)->copy();
                        localShape = (PolynomialShape *)body->getGeometry();
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
                body->translate(&localVector);
                break;

            case Tokenizer::ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                body->rotate(&localVector);
                break;

            case Tokenizer::SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                body->scale(&localVector);
                break;

            case Tokenizer::INVERSE_TOKEN:
                body->invert();
                break;

            case Tokenizer::TEXTURE_TOKEN:
                localTexture = TextureParser::parseTexture(ctx);
                if (localTexture->isConstant()) {
                    localTexture = TextureParser::copyTexture(localTexture);
                }

                TextureParser::prependTextureLayers(localTexture, body->getMaterialRef());
                break;

            case Tokenizer::COLOUR_TOKEN:
                body->setShapeColor(ModelBuilder::getColor());
                PrimitiveParser::parseColor(body->getShapeColor(), ctx);
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    return body;
}
