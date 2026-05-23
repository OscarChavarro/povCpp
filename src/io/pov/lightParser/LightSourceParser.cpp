#include "io/pov/ParserContext.h"
#include "io/pov/lightParser/LightSourceParser.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/light/Light.h"
#include "io/pov/Parse.h"
#include "io/pov/ParseHelpers.h"
#include "io/pov/PrimitiveParser.h"
#include "io/pov/SceneConfigParser.h"


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
    CONSTANT constantId;

    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (ctx.token().tokenId) {
            case LEFT_ANGLE_TOKEN:
                Tokenizer::ungetToken();
                localShape = ModelBuilder::getLightSourceShape();
                PrimitiveParser::parseVector(&(localShape->Center), ctx);
                localShape->Shape_Colour = ModelBuilder::getColour();
                Color::makeColor(localShape->Shape_Colour, 1.0, 1.0, 1.0);
                localShape->Shape_Colour->Alpha = 0.0;
                ParseHelpers::getExpectedToken(COLOUR_TOKEN, ctx);
                PrimitiveParser::parseColour(localShape->Shape_Colour, ctx);
                Exit_Flag = TRUE;
                break;

            case IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant(ctx)) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        LIGHT_SOURCE_CONSTANT) {
                        localShape = (Light *)GeometryOperations::copy(
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

            /* Point that the spot is pointed at */
            case POINT_AT_TOKEN:
                PrimitiveParser::parseVector(&(localShape->pointsAt), ctx);
                break;

            case TIGHTNESS_TOKEN:
                localShape->Coeff = PrimitiveParser::parseFloat(ctx);
                break;

            case RADIUS_TOKEN:
                localShape->Radius =
                    cos(PrimitiveParser::parseFloat(ctx) * M_PI / 180.0);
                break;

            case COLOUR_TOKEN:
                PrimitiveParser::parseColour(localShape->Shape_Colour, ctx);
                break;

            case FALLOFF_TOKEN:
                localShape->Falloff =
                    cos(PrimitiveParser::parseFloat(ctx) * M_PI / 180.0);
                break;

            case SPOTLIGHT_TOKEN:
                localShape->Type = SPOT_LIGHT_TYPE;
                break;

            default:
                ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    return ((Geometry *)localShape);
}
