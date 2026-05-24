#include "io/pov/ParserContext.h"
#include "io/pov/cameraParser/CameraParser.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/camera/Camera.h"
#include "io/pov/Parse.h"


static CONSTANT
findConstant(ParserContext &ctx)
{
    int i;

    for (i = 1; i <= ctx.numberOfConstants(); i++) {
        if (ctx.constants()[i].identifierNumber == ctx.token().identifierNumber) {
            return (i);
        }
    }

    return (-1);
}

void
CameraParser::parseCamera(Camera *givenVp)
{
    ParserContext ctx;
    CameraParser::parseCamera(givenVp, ctx);
}

void
CameraParser::parseCamera(Camera *givenVp, ParserContext &ctx)
{
    CONSTANT constantId;
    Vector3Dd localVector;
    Vector3Dd tempVector;
    double directionLength;
    double upLength;
    double rightLength;
    double handedness;

    givenVp->initializeDefaults();

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        int Exit_Flag;
        Exit_Flag = LegacyBoolean::FALSE_VALUE;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = findConstant(ctx)) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        ParseGlobals::VIEW_POINT_CONSTANT) {
                        *givenVp = *((Camera *)ctx.constants()[(int)constantId]
                                .constantData);
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::Undeclared(ctx);
                }
                break;

            case Tokenizer::LOCATION_TOKEN:
                PrimitiveParser::parseVector(&(givenVp->Location), ctx);
                break;

            case Tokenizer::DIRECTION_TOKEN:
                PrimitiveParser::parseVector(&(givenVp->Direction), ctx);
                break;

            case Tokenizer::UP_TOKEN:
                PrimitiveParser::parseVector(&(givenVp->Up), ctx);
                break;

            case Tokenizer::RIGHT_TOKEN:
                PrimitiveParser::parseVector(&(givenVp->Right), ctx);
                break;

            case Tokenizer::SKY_TOKEN:
                PrimitiveParser::parseVector(&(givenVp->Sky), ctx);
                break;

            case Tokenizer::LOOK_AT_TOKEN:
                directionLength = givenVp->Direction.length();
                upLength = givenVp->Up.length();
                rightLength = givenVp->Right.length();
                tempVector = givenVp->Direction.crossProduct(givenVp->Up);
                handedness = tempVector.dotProduct(givenVp->Right);
                PrimitiveParser::parseVector(&givenVp->Direction, ctx);

                givenVp->Direction.sub(givenVp->Location);
                givenVp->Direction.normalize();
                givenVp->Right = givenVp->Direction.crossProduct(givenVp->Sky);
                givenVp->Right.normalize();
                givenVp->Up = givenVp->Right.crossProduct(givenVp->Direction);
                givenVp->Direction.scale(directionLength);
                if (handedness >= 0.0) {
                    givenVp->Right.scale(rightLength);
                } else {
                    givenVp->Right.scale(-rightLength);
                }

                givenVp->Up.scale(upLength);
                break;

            case Tokenizer::TRANSLATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::translate(
                    (SimpleBody *)givenVp, &localVector);
                break;

            case Tokenizer::ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::rotate((SimpleBody *)givenVp, &localVector);
                break;

            case Tokenizer::SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::scale((SimpleBody *)givenVp, &localVector);
                break;

            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }
}
