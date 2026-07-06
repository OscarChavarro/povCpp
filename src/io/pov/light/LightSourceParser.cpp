#include "java/lang/Math.h"

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/environment/light/PointLight.h"
#include "vsdk/toolkit/environment/light/SpotLight.h"

#include "io/pov/light/LightSourceParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"
#include "java/util/PriorityQueue.txx"


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
    double coefficient = 10.0;
    double radius = 0.35;
    double falloff = 0.35;
    bool spotlight = false;
    ColorRgba *localColor = nullptr;
    // Both branches that set localColor (LEFT_ANGLE_TOKEN: fresh allocation;
    // IDENTIFIER_TOKEN: a clone of the referenced LIGHT_SOURCE_CONSTANT's own
    // color, never an alias - the constant is owned independently and
    // could be freed by a #declare re-declare while this Light is still being
    // built) leave this function owning localColor. Light/PointLight/
    // SpotLight's constructor always clones color internally, so
    // localColor itself is no longer needed once the Light below is built.
    bool ownsLocalColor = false;
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
                coefficient = 10.0;
                radius = 0.35;
                falloff = 0.35;
                spotlight = false;
                localColor = new ColorRgba(0.0, 0.0, 0.0, 0.0);
                ownsLocalColor = true;
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
                        center = constantLight->getPosition();
                        // Clone, don't alias: the second token loop below may
                        // mutate localColor in place via parseColor(), and
                        // constantLight is owned by the LIGHT_SOURCE_CONSTANT
                        // slot - it can be freed independently (e.g. by
                        // DeclarationParser re-declaring this same identifier
                        // later in the file) while this Light is still being
                        // built.
                        localColor = new ColorRgba(constantLight->getColor());
                        ownsLocalColor = true;
                        // pointsAt/coefficient/radius/falloff live only on
                        // SpotLight now; a cloned PointLight constant has no
                        // spot state to carry forward, so the defaults set
                        // above stand.
                        if (const SpotLight *constantSpotLight =
                                dynamic_cast<const SpotLight *>(constantLight)) {
                            pointsAt = constantSpotLight->getPointsAt();
                            coefficient = constantSpotLight->getCoefficient();
                            radius = constantSpotLight->getRadius();
                            falloff = constantSpotLight->getFalloff();
                            spotlight = true;
                        } else {
                            spotlight = false;
                        }
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

    Light * const result = spotlight ?
        static_cast<Light *>(new SpotLight(
            *localColor, center, pointsAt, coefficient, radius, falloff)) :
        static_cast<Light *>(new PointLight(*localColor, center));
    if (ownsLocalColor) {
        delete localColor;
    }
    return result;
}
