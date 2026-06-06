#include "io/pov/ParserContext.h"
#include "io/pov/lightParser/LightSourceParser.h"
#include "common/logger/Logger.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/light/Light.h"
#include "environment/scene/ModelBuilder.h"
#include "io/pov/ParseErrorReporter.h"
#include "io/pov/ParseGlobals.h"
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

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        int Exit_Flag;
        Exit_Flag = LegacyBoolean::FALSE_VALUE;
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
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant(ctx)) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        ParseGlobals::LIGHT_SOURCE_CONSTANT) {
                        localShape = (Light *)GeometryOperations::copy(
                            (SimpleBody *)ctx.constants()[(int)constantId]
                                .constantData);
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::Undeclared(ctx);
                }
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::LEFT_ANGLE_TOKEN, ctx);
                break;
            }
        }
    }

    {
        int Exit_Flag;
        Exit_Flag = LegacyBoolean::FALSE_VALUE;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
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

    static bool loggedOnce = false;
    if (!loggedOnce && localShape != nullptr && std::getenv("POVCPP_DIAG_MONKEY") != nullptr) {
        loggedOnce = true;
        Logger::info(
            "[DIAG-MONKEY] legacy light center=<%.6f,%.6f,%.6f> pointsAt=<%.6f,%.6f,%.6f> colour=<%.6f,%.6f,%.6f,%.6f> coeff=%.6f radius=%.6f falloff=%.6f type=%d\n",
            localShape->Center.x, localShape->Center.y, localShape->Center.z,
            localShape->pointsAt.x, localShape->pointsAt.y, localShape->pointsAt.z,
            localShape->Shape_Colour != nullptr ? localShape->Shape_Colour->Red : -1.0,
            localShape->Shape_Colour != nullptr ? localShape->Shape_Colour->Green : -1.0,
            localShape->Shape_Colour != nullptr ? localShape->Shape_Colour->Blue : -1.0,
            localShape->Shape_Colour != nullptr ? localShape->Shape_Colour->Alpha : -1.0,
            localShape->Coeff, localShape->Radius, localShape->Falloff, localShape->Type);
    }

    return ((Geometry *)localShape);
}
