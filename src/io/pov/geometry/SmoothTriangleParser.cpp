#include "io/pov/geometry/SmoothTriangleParser.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/elements/Triangle.h"
#include "environment/scene/ModelBuilder.h"
#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/material/TextureParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

Geometry *
SmoothTriangleParser::parseSmoothTriangle()
{
    ParserContext ctx;
    return SmoothTriangleParser::parseSmoothTriangle(ctx);
}

Geometry *
SmoothTriangleParser::parseSmoothTriangle(ParserContext &ctx)
{
    (void)ctx;
    SmoothTriangle *localShape;
    int constantId;
    Vector3Dd localVector;
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
                localShape =
                    (SmoothTriangle *)ModelBuilder::getSmoothTriangleShape();
                PrimitiveParser::parseVector(&localShape->P1, ctx);
                PrimitiveParser::parseVector(&localShape->N1, ctx);
                localShape->N1 = localShape->N1.normalizedFast();
                PrimitiveParser::parseVector(&localShape->P2, ctx);
                PrimitiveParser::parseVector(&localShape->N2, ctx);
                localShape->N2 = localShape->N2.normalizedFast();
                PrimitiveParser::parseVector(&localShape->P3, ctx);
                PrimitiveParser::parseVector(&localShape->N3, ctx);
                localShape->N3 = localShape->N3.normalizedFast();
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
                        localShape = (SmoothTriangle *)GeometryOperations::copy(
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

                TextureParser::prependTextureLayers(localTexture, localShape->material);
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
#include "java/util/PriorityQueue.txx"
