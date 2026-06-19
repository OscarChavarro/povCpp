#include "java/lang/Math.h"
#include "java/util/PriorityQueue.txx"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

#include "environment/light/Light.h"
#include "environment/light/PointLight.h"
#include "environment/light/SpotLight.h"
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
    Vector3Dd localVector;
    Vector3Dd center(0.0, 0.0, 0.0);
    Vector3Dd pointsAt(0.0, 0.0, 1.0);
    bool inverted = false;
    double coefficient = 10.0;
    double radius = 0.35;
    double falloff = 0.35;
    bool spotlight = false;
    ColorRgba *localColor = nullptr;
    int constantId;

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
            case Tokenizer::LEFT_ANGLE_TOKEN:
                ctx.tokenStream().ungetToken();
                center = Vector3Dd(0.0, 0.0, 0.0);
                pointsAt = Vector3Dd(0.0, 0.0, 1.0);
                inverted = false;
                coefficient = 10.0;
                radius = 0.35;
                falloff = 0.35;
                spotlight = false;
                localColor = ModelBuilder::getColor();
                localColor->setR(1.0); localColor->setG(1.0); localColor->setB(1.0); localColor->setA(0.0);
                PrimitiveParser::parseVector(&center, ctx);
                ParseHelpers::getExpectedToken(Tokenizer::COLOUR_TOKEN, ctx);
                PrimitiveParser::parseColor(localColor, ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].getConstantType() ==
                        ParseGlobals::LIGHT_SOURCE_CONSTANT) {
                        Light * const constantLight = static_cast<Light *>(
                            ctx.constants()[(int)constantId].getConstantData());
                        center = constantLight->getCenter();
                        pointsAt = constantLight->getPointsAt();
                        inverted = constantLight->isInverted();
                        coefficient = constantLight->getCoefficient();
                        radius = constantLight->getRadius();
                        falloff = constantLight->getFalloff();
                        localColor = constantLight->getShapeColor();
                        spotlight = dynamic_cast<SpotLight *>(constantLight) != nullptr;
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
                center = center.add(localVector);
                pointsAt = pointsAt.add(localVector);
                break;

            case Tokenizer::ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                {
                    Matrix4x4d transformation;
                    Matrix4x4d transformationInverse;
                    transformation.axisRotationRodrigues(&transformationInverse, &localVector);
                    center = transformation.transpose().multiply(center);
                    pointsAt = transformation.transpose().multiply(pointsAt);
                }
                break;

            case Tokenizer::SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                {
                    Matrix4x4d transformation;
                    transformation = Matrix4x4d().scale(
                        localVector.x(), localVector.y(), localVector.z());
                    center = transformation.transpose().multiply(center);
                    pointsAt = transformation.transpose().multiply(pointsAt);
                }
                break;

            // Point that the spot is pointed at
            case Tokenizer::POINT_AT_TOKEN:
                PrimitiveParser::parseVector(&pointsAt, ctx);
                break;

            case Tokenizer::TIGHTNESS_TOKEN:
                coefficient = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::RADIUS_TOKEN:
                radius = java::Math::cos(
                    PrimitiveParser::parseFloat(ctx) * java::Math::PI / 180.0);
                break;

            case Tokenizer::COLOUR_TOKEN:
                PrimitiveParser::parseColor(localColor, ctx);
                break;

            case Tokenizer::FALLOFF_TOKEN:
                falloff = java::Math::cos(
                    PrimitiveParser::parseFloat(ctx) * java::Math::PI / 180.0);
                break;

            case Tokenizer::SPOTLIGHT_TOKEN:
                spotlight = true;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    Light *localShape = spotlight ?
        static_cast<Light *>(new SpotLight(
            center, pointsAt, inverted, coefficient, radius, falloff)) :
        static_cast<Light *>(new PointLight(
            center, pointsAt, inverted, coefficient, radius, falloff));
    if (localShape != nullptr) {
        localShape->setShapeColor(localColor);
    }

    return localShape;
}
