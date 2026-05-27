#include "io/pov/ParserContext.h"
#include "io/pov/cameraParser/CameraParser.h"
#include <cstdlib>
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/camera/Camera.h"
#include "io/pov/Parse.h"
#include "common/logger/Logger.h"

namespace {
bool shouldLogMonkeyDiagnostics()
{
    const char *flag = std::getenv("POVCPP_DIAG_MONKEY");
    return flag != nullptr && flag[0] != '\0';
}

void logCameraOnce(const char *prefix, const Camera *camera)
{
    static int logged = 0;
    if (!shouldLogMonkeyDiagnostics() || logged++ > 0 || camera == nullptr) {
        return;
    }
    Logger::info(
        "[DIAG-MONKEY] %s camera loc=<%.6f,%.6f,%.6f> dir=<%.6f,%.6f,%.6f> up=<%.6f,%.6f,%.6f> right=<%.6f,%.6f,%.6f> sky=<%.6f,%.6f,%.6f>\n",
        prefix,
        camera->Location.x, camera->Location.y, camera->Location.z,
        camera->Direction.x, camera->Direction.y, camera->Direction.z,
        camera->Up.x, camera->Up.y, camera->Up.z,
        camera->Right.x, camera->Right.y, camera->Right.z,
        camera->Sky.x, camera->Sky.y, camera->Sky.z);
}

void logCameraOp(const char *prefix, const char *opName, const Vector3Dd *value)
{
    if (!shouldLogMonkeyDiagnostics() || value == nullptr) {
        return;
    }
    Logger::info(
        "[DIAG-MONKEY] %s camera op=%s value=<%.6f,%.6f,%.6f>\n",
        prefix, opName, value->x, value->y, value->z);
}
}

CONSTANT
CameraParser::findConstant(ParserContext &ctx)
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
                if ((constantId = CameraParser::findConstant(ctx)) != -1) {
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
                logCameraOp("legacy", "location", &givenVp->Location);
                break;

            case Tokenizer::DIRECTION_TOKEN:
                PrimitiveParser::parseVector(&(givenVp->Direction), ctx);
                logCameraOp("legacy", "direction", &givenVp->Direction);
                break;

            case Tokenizer::UP_TOKEN:
                PrimitiveParser::parseVector(&(givenVp->Up), ctx);
                logCameraOp("legacy", "up", &givenVp->Up);
                break;

            case Tokenizer::RIGHT_TOKEN:
                PrimitiveParser::parseVector(&(givenVp->Right), ctx);
                logCameraOp("legacy", "right", &givenVp->Right);
                break;

            case Tokenizer::SKY_TOKEN:
                PrimitiveParser::parseVector(&(givenVp->Sky), ctx);
                logCameraOp("legacy", "sky", &givenVp->Sky);
                break;

            case Tokenizer::LOOK_AT_TOKEN:
                directionLength = givenVp->Direction.length();
                upLength = givenVp->Up.length();
                rightLength = givenVp->Right.length();
                tempVector = givenVp->Direction.crossProduct(givenVp->Up);
                handedness = tempVector.dotProduct(givenVp->Right);
                PrimitiveParser::parseVector(&givenVp->Direction, ctx);
                logCameraOp("legacy", "look_at", &givenVp->Direction);

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

    logCameraOnce("legacy", givenVp);
}
