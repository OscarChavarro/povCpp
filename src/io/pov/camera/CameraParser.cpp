#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

#include "environment/camera/Camera.h"

#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/ParserContext.h"

#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"

#include "io/pov/camera/CameraParser.h"

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
                PrimitiveParser::parseVector(&(givenVp->location), ctx);
                break;

            case Tokenizer::DIRECTION_TOKEN:
                PrimitiveParser::parseVector(&(givenVp->direction), ctx);
                break;

            case Tokenizer::UP_TOKEN:
                PrimitiveParser::parseVector(&(givenVp->up), ctx);
                break;

            case Tokenizer::RIGHT_TOKEN:
                PrimitiveParser::parseVector(&(givenVp->right), ctx);
                break;

            case Tokenizer::SKY_TOKEN:
                PrimitiveParser::parseVector(&(givenVp->sky), ctx);
                break;

            case Tokenizer::LOOK_AT_TOKEN:
                directionLength = givenVp->direction.length();
                upLength = givenVp->up.length();
                rightLength = givenVp->right.length();
                tempVector = givenVp->direction.crossProduct(givenVp->up);
                handedness = tempVector.dotProduct(givenVp->right);
                PrimitiveParser::parseVector(&givenVp->direction, ctx);

                givenVp->direction =
                    givenVp->direction.subtract(givenVp->location);
                givenVp->direction = givenVp->direction.normalizedFast();
                givenVp->right = givenVp->direction.crossProduct(givenVp->sky);
                givenVp->right = givenVp->right.normalizedFast();
                givenVp->up = givenVp->right.crossProduct(givenVp->direction);
                givenVp->direction = givenVp->direction.multiply(directionLength);
                if (handedness >= 0.0) {
                    givenVp->right = givenVp->right.multiply(rightLength);
                } else {
                    givenVp->right = givenVp->right.multiply(-rightLength);
                }

                givenVp->up = givenVp->up.multiply(upLength);
                break;

            case Tokenizer::TRANSLATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                givenVp->translate(&localVector);
                break;

            case Tokenizer::ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                givenVp->rotate(&localVector);
                break;

            case Tokenizer::SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                givenVp->scale(&localVector);
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
#include "java/util/PriorityQueue.txx"
