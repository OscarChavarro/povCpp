#include "io/pov/context/ParserContext.h"
#include "io/pov/light/LightSourceParser.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/light/Light.h"
#include "environment/scene/ModelBuilder.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/context/ParseGlobals.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"


Geometry *
LightSourceParser::parseLightSource()
{
    ParserContext ctx;
    return LightSourceParser::parseLightSource(ctx);
}

Geometry *
LightSourceParser::parseLightSource(ParserContext &ctx)
{
    (void)ctx;
    Light *localShape = nullptr;
    Vector3Dd localVector;
    int constantId;

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::LEFT_ANGLE_TOKEN:
                ctx.tokenStream().ungetToken();
                localShape = ModelBuilder::getLightSourceShape();
                PrimitiveParser::parseVector(&(localShape->Center), ctx);
                localShape->Shape_Colour = ModelBuilder::getColour();
                Color::makeColor(localShape->Shape_Colour, 1.0, 1.0, 1.0);
                localShape->Shape_Colour->Alpha = 0.0;
                ParseHelpers::getExpectedToken(Tokenizer::COLOUR_TOKEN, ctx);
                PrimitiveParser::parseColour(localShape->Shape_Colour, ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        ParseGlobals::LIGHT_SOURCE_CONSTANT) {
                        localShape = (Light *)GeometryOperations::copy(
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

            /* Point that the spot is pointed at */
            case Tokenizer::POINT_AT_TOKEN:
                PrimitiveParser::parseVector(&(localShape->pointsAt), ctx);
                break;

            case Tokenizer::TIGHTNESS_TOKEN:
                localShape->Coeff = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::RADIUS_TOKEN:
                localShape->Radius =
                    cos(PrimitiveParser::parseFloat(ctx) * M_PI / 180.0);
                break;

            case Tokenizer::COLOUR_TOKEN:
                PrimitiveParser::parseColour(localShape->Shape_Colour, ctx);
                break;

            case Tokenizer::FALLOFF_TOKEN:
                localShape->Falloff =
                    cos(PrimitiveParser::parseFloat(ctx) * M_PI / 180.0);
                break;

            case Tokenizer::SPOTLIGHT_TOKEN:
                localShape->Type = GeometryOperations::SPOT_LIGHT_TYPE;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    return ((Geometry *)localShape);
}
