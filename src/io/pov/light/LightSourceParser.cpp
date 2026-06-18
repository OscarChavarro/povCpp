#include "java/lang/Math.h"
#include "java/util/PriorityQueue.txx"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

#include "environment/light/Light.h"
#include "environment/scene/ModelBuilder.h"

#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/light/LightSourceParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"


Light *
LightSourceParser::parseLightSource()
{
    ParserContext ctx;
    return LightSourceParser::parseLightSource(ctx);
}

Light *
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
                PrimitiveParser::parseVector(&localShape->getCenter(), ctx);
                localShape->setShapeColor(ModelBuilder::getColor());
                localShape->getShapeColor()->setR(1.0); localShape->getShapeColor()->setG(1.0); localShape->getShapeColor()->setB(1.0); localShape->getShapeColor()->setA(0.0);
                ParseHelpers::getExpectedToken(Tokenizer::COLOUR_TOKEN, ctx);
                PrimitiveParser::parseColor(localShape->getShapeColor(), ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].getConstantType() ==
                        ParseGlobals::LIGHT_SOURCE_CONSTANT) {
                        localShape = static_cast<Light *>(
                            static_cast<Light *>(
                                ctx.constants()[(int)constantId].getConstantData())
                                ->copy());
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
                localShape->translate(&localVector);
                break;

            case Tokenizer::ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                localShape->rotate(&localVector);
                break;

            case Tokenizer::SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                localShape->scale(&localVector);
                break;

            // Point that the spot is pointed at
            case Tokenizer::POINT_AT_TOKEN:
                PrimitiveParser::parseVector(&localShape->getPointsAt(), ctx);
                break;

            case Tokenizer::TIGHTNESS_TOKEN:
                localShape->setCoefficient(PrimitiveParser::parseFloat(ctx));
                break;

            case Tokenizer::RADIUS_TOKEN:
                localShape->setRadius(
                    java::Math::cos(PrimitiveParser::parseFloat(ctx) * java::Math::PI / 180.0));
                break;

            case Tokenizer::COLOUR_TOKEN:
                PrimitiveParser::parseColor(localShape->getShapeColor(), ctx);
                break;

            case Tokenizer::FALLOFF_TOKEN:
                localShape->setFalloff(
                    java::Math::cos(PrimitiveParser::parseFloat(ctx) * java::Math::PI / 180.0));
                break;

            case Tokenizer::SPOTLIGHT_TOKEN:
                localShape = ModelBuilder::promoteToSpotLight(localShape);
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    return localShape;
}
