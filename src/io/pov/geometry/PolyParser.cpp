#include "java/util/PriorityQueue.txx"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/numericalAnalysis/polynomial/PolynomialSolver.h"

#include "environment/geometry/volume/polynomial/PolynomialShape.h"
#include "environment/scene/SceneBuilder.h"
#include "environment/geometry/GeometryBuilder.h"
#include "environment/geometry/SimpleBody.h"

#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/geometry/PolyParser.h"
#include "environment/material/povray/PovRayMaterialConstancy.h"
#include "io/pov/material/TextureParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"

SimpleBody *
PolyParser::rebuildBodyWithGeometry(SimpleBody *body, Geometry *geometry)
{
    Material *clonedMaterial = (body->getMaterial() != nullptr) ?
        body->getMaterial()->copy() : nullptr;
    ColorRgba *clonedShapeColor = (body->getShapeColor() != nullptr) ?
        new ColorRgba(*body->getShapeColor()) : nullptr;
    SimpleBody *newBody = new SimpleBody(geometry, clonedMaterial, clonedShapeColor);
    newBody->getTransform() = body->getTransform();
    newBody->getTransformInverse() = body->getTransformInverse();
    delete body;
    return newBody;
}

void
PolyParser::parseCoeffs(int order, double *givenCoeffs, ParserContext &ctx)
{
    int i;

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
            case Tokenizer::LEFT_ANGLE_TOKEN:
                for (i = 0; i < PolynomialShape::termCountsByOrder()[order]; i++) {
                    givenCoeffs[i] = PrimitiveParser::parseFloat(ctx);
                }
                ParseHelpers::getExpectedToken(Tokenizer::RIGHT_ANGLE_TOKEN, ctx);
                Exit_Flag = true;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::LEFT_ANGLE_TOKEN, ctx);
                break;
            }
        }
    }
}

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
    PovRayMaterial *localTexture;

    if (knownOrder > 0) {
        localShape = GeometryBuilder::getPolyShape(
            knownOrder, PolynomialShape::termCountsByOrder());
        body = SceneBuilder::wrap(localShape);
    } else {
        localShape = nullptr;
    }

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
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
                localShape = GeometryBuilder::getPolyShape(
                    order, PolynomialShape::termCountsByOrder());
                body = SceneBuilder::wrap(localShape);
                break;

            case Tokenizer::LEFT_ANGLE_TOKEN:
                ctx.tokenStream().ungetToken();
                if (localShape == nullptr) {
                    Logger::reportMessage("PolyParser", Logger::WARNING, "", "Need the order of the Poly");
                }
                parseCoeffs(localShape->getOrder(), &(localShape->getCoeffs()[0]), ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].getConstantType() ==
                        ParseGlobals::POLY_CONSTANT) {
                        // For cubic/quartic (knownOrder > 0), `body` was already
                        // built above before this token was seen (the fixed order
                        // is known up front, unlike generic `poly`) - this
                        // identifier reference replaces it entirely, so the
                        // pre-built one must be freed first. Brand new, never
                        // shared, so safe to delete unconditionally.
                        delete body;
                        body = new SimpleBody(
                                *(SimpleBody *)ctx.constants()[(int)constantId]
                                    .getConstantData());
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
            switch (ctx.token().getTokenId()) {
            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = true;
                break;

            case Tokenizer::STURM_TOKEN:
                if (localShape->getSturmFlag() == 0) {
                    body = rebuildBodyWithGeometry(
                        body, localShape->copyWithSturmFlag(1));
                    localShape = (PolynomialShape *)body->getGeometry();
                }
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
                if (PovRayMaterialConstancy::isConstant(localTexture)) {
                    localTexture = TextureParser::copyTexture(localTexture);
                }

                body->prependMaterialLayers(localTexture);
                break;

            case Tokenizer::COLOUR_TOKEN:
                PrimitiveParser::parseColor(body->ensureShapeColor(), ctx);
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    return body;
}
