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
    SmoothTriangle *localShape;
    SimpleBody *body = nullptr;
    int constantId;
    Vector3Dd localVector;
    Vector3Dd p1;
    Vector3Dd p2;
    Vector3Dd p3;
    Vector3Dd n1;
    Vector3Dd n2;
    Vector3Dd n3;
    PovrayMaterial *localTexture;

    localShape = nullptr;

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
            case Tokenizer::LEFT_ANGLE_TOKEN:
                ctx.tokenStream().ungetToken();
                PrimitiveParser::parseVector(&p1, ctx);
                PrimitiveParser::parseVector(&n1, ctx);
                PrimitiveParser::parseVector(&p2, ctx);
                PrimitiveParser::parseVector(&n2, ctx);
                PrimitiveParser::parseVector(&p3, ctx);
                PrimitiveParser::parseVector(&n3, ctx);
                localShape = new SmoothTriangle(p1, n1, p2, n2, p3, n3);
                body = ModelBuilder::wrap(localShape);
                if (localShape->isDegenerate()) {
                    {
                        char _logMsg[1024];
                        snprintf(_logMsg, sizeof(_logMsg), "Degenerate triangle on line %d.  Please remove.\n", ctx.token().getTokenLineNumber());
                        Logger::reportMessage("SmoothTriangleParser", Logger::ERROR, "", _logMsg);
                    }
                    ctx.degenerateTriangles() = true;
                }
                Exit_Flag = true;
                break;

            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].getConstantType() ==
                        ParseGlobals::SMOOTH_TRIANGLE_CONSTANT) {
                        body = new SimpleBody(
                                *(SimpleBody *)ctx.constants()[(int)constantId]
                                    .getConstantData());
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
            switch (ctx.token().getTokenId()) {
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

                TextureParser::prependTextureLayers(localTexture, body);
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
