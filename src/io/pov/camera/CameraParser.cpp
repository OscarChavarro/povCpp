#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"
#include "io/pov/camera/CameraParser.h"

CameraSnapshot
CameraParser::parseCamera()
{
    ParserContext ctx;
    return CameraParser::parseCamera(ctx);
}

CameraSnapshot
CameraParser::parseCamera(ParserContext &ctx)
{
    PovCameraSpec parsedCamera;
    CameraParser::parseCamera(&parsedCamera, ctx);
    return parsedCamera.bake();
}

void
CameraParser::parseCamera(PovCameraSpec *givenVp)
{
    ParserContext ctx;
    CameraParser::parseCamera(givenVp, ctx);
}

void
CameraParser::parseCamera(PovCameraSpec *givenVp, ParserContext &ctx)
{
    PovCameraSpec parsedCamera;

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        bool exitFlag = false;
        while (!exitFlag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
            case Tokenizer::IDENTIFIER_TOKEN:
            {
                int constantId;
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].getConstantType() ==
                        ParseGlobals::VIEW_POINT_CONSTANT) {
                        parsedCamera = *((PovCameraSpec *)ctx.constants()[(int)constantId]
                                .getConstantData());
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::reportUndeclared(ctx);
                }
                break;
            }

            case Tokenizer::LOCATION_TOKEN:
                PrimitiveParser::parseVector(&parsedCamera.getLocation(), ctx);
                break;

            case Tokenizer::DIRECTION_TOKEN:
                PrimitiveParser::parseVector(&parsedCamera.getDirection(), ctx);
                break;

            case Tokenizer::UP_TOKEN:
                PrimitiveParser::parseVector(&parsedCamera.getUp(), ctx);
                break;

            case Tokenizer::RIGHT_TOKEN:
                PrimitiveParser::parseVector(&parsedCamera.getRight(), ctx);
                break;

            case Tokenizer::SKY_TOKEN:
                PrimitiveParser::parseVector(&parsedCamera.getSky(), ctx);
                break;

            case Tokenizer::LOOK_AT_TOKEN:
            {
                double directionLength = parsedCamera.getDirection().length();
                double upLength = parsedCamera.getUp().length();
                double rightLength = parsedCamera.getRight().length();
                Vector3Dd tempVector =
                    parsedCamera.getDirection().crossProduct(parsedCamera.getUp());
                double handedness = tempVector.dotProduct(parsedCamera.getRight());
                PrimitiveParser::parseVector(&parsedCamera.getDirection(), ctx);

                parsedCamera.getDirection() =
                    parsedCamera.getDirection().subtract(parsedCamera.getLocation());
                parsedCamera.getDirection() = parsedCamera.getDirection().normalizedFast();
                parsedCamera.getRight() =
                    parsedCamera.getDirection().crossProduct(parsedCamera.getSky());
                parsedCamera.getRight() = parsedCamera.getRight().normalizedFast();
                parsedCamera.getUp() =
                    parsedCamera.getRight().crossProduct(parsedCamera.getDirection());
                parsedCamera.getDirection() =
                    parsedCamera.getDirection().multiply(directionLength);
                if (handedness >= 0.0) {
                    parsedCamera.getRight() = parsedCamera.getRight().multiply(rightLength);
                } else {
                    parsedCamera.getRight() = parsedCamera.getRight().multiply(-rightLength);
                }

                parsedCamera.getUp() = parsedCamera.getUp().multiply(upLength);
                break;
            }

            case Tokenizer::TRANSLATE_TOKEN:
            {
                Vector3Dd localVector;
                PrimitiveParser::parseVector(&localVector, ctx);
                parsedCamera.translate(&localVector);
                break;
            }

            case Tokenizer::ROTATE_TOKEN:
            {
                Vector3Dd localVector;
                PrimitiveParser::parseVector(&localVector, ctx);
                parsedCamera.rotate(&localVector);
                break;
            }

            case Tokenizer::SCALE_TOKEN:
            {
                Vector3Dd localVector;
                PrimitiveParser::parseVector(&localVector, ctx);
                parsedCamera.scale(&localVector);
                break;
            }

            case Tokenizer::RIGHT_CURLY_TOKEN:
                exitFlag = true;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    *givenVp = parsedCamera;
}
#include "java/util/PriorityQueue.txx"
