#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "common/linealAlgebra/Vector3DdOps.h"
#include "environment/camera/Camera.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/camera/CameraParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/context/ParseGlobals.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"

void
CameraParser::parseCamera(Camera *givenVp)
{
    ParserContext ctx;
    CameraParser::parseCamera(givenVp, ctx);
}

void
CameraParser::parseCamera(Camera *givenVp, ParserContext &ctx)
{
    int constantId;
    Vector3Dd localVector;
    Vector3Dd tempVector;
    double directionLength;
    double upLength;
    double rightLength;
    double handedness;

    givenVp->initializeDefaults();

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        ParseGlobals::VIEW_POINT_CONSTANT) {
                        *givenVp = *((Camera *)ctx.constants()[(int)constantId]
                                .constantData);
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::reportUndeclared(ctx);
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

                givenVp->Direction =
                    givenVp->Direction.subtract(givenVp->Location);
                givenVp->Direction = Vec3::normalized(givenVp->Direction);
                givenVp->Right = givenVp->Direction.crossProduct(givenVp->Sky);
                givenVp->Right = Vec3::normalized(givenVp->Right);
                givenVp->Up = givenVp->Right.crossProduct(givenVp->Direction);
                givenVp->Direction = Vec3::scaled(givenVp->Direction, directionLength);
                if (handedness >= 0.0) {
                    givenVp->Right = Vec3::scaled(givenVp->Right, rightLength);
                } else {
                    givenVp->Right = Vec3::scaled(givenVp->Right, -rightLength);
                }

                givenVp->Up = Vec3::scaled(givenVp->Up, upLength);
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
                Exit_Flag = true;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }
}
