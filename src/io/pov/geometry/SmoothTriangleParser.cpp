#include "java/util/PriorityQueue.txx"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"

#include "environment/geometry/element/Triangle.h"
#include "environment/scene/ModelBuilder.h"
#include "environment/geometry/SimpleBody.h"

#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/geometry/SmoothTriangleParser.h"
#include "io/pov/material/TextureParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"

SimpleBody *
SmoothTriangleParser::parseSmoothTriangle()
{
    ParserContext ctx;
    return SmoothTriangleParser::parseSmoothTriangle(ctx);
}

SimpleBody *
SmoothTriangleParser::parseSmoothTriangle(ParserContext &ctx)
{
    (void)ctx;
    SmoothTriangle *localShape;
    SimpleBody *body = nullptr;
    int constantId;
    Vector3Dd localVector;
    PovrayMaterial *localTexture;

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
                localShape =
                    (SmoothTriangle *)ModelBuilder::getSmoothTriangleShape();
                body = ModelBuilder::wrap(localShape);
                PrimitiveParser::parseVector(&localShape->getP1(), ctx);
                PrimitiveParser::parseVector(&localShape->getN1(), ctx);
                localShape->getN1() = localShape->getN1().normalizedFast();
                PrimitiveParser::parseVector(&localShape->getP2(), ctx);
                PrimitiveParser::parseVector(&localShape->getN2(), ctx);
                localShape->getN2() = localShape->getN2().normalizedFast();
                PrimitiveParser::parseVector(&localShape->getP3(), ctx);
                PrimitiveParser::parseVector(&localShape->getN3(), ctx);
                localShape->getN3() = localShape->getN3().normalizedFast();
                if (!Triangle::computeTriangle((Triangle *)localShape)) {
                    {
                        char _logMsg[1024];
                        snprintf(_logMsg, sizeof(_logMsg), "Degenerate triangle on line %d.  Please remove.\n", ctx.token().tokenLineNo);
                        Logger::reportMessage("SmoothTriangleParser", Logger::ERROR, "", _logMsg);
                    }
                    ctx.degenerateTriangles() = true;
                }
                Exit_Flag = true;
                break;

            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        ParseGlobals::SMOOTH_TRIANGLE_CONSTANT) {
                        body = (SimpleBody *)((TransformableElement *)ctx.constants()[(int)constantId]
                                .constantData)->copy();
                        localShape = (SmoothTriangle *)body->getGeometry();
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
