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
                    if (ctx.constants()[(int)constantId].getConstantType() ==
                        ParseGlobals::VIEW_POINT_CONSTANT) {
                        *givenVp = *((Camera *)ctx.constants()[(int)constantId]
                                .getConstantData());
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::reportUndeclared(ctx);
                }
                break;

            case Tokenizer::LOCATION_TOKEN:
                PrimitiveParser::parseVector(&givenVp->getLocation(), ctx);
                break;

            case Tokenizer::DIRECTION_TOKEN:
                PrimitiveParser::parseVector(&givenVp->getDirection(), ctx);
                break;

            case Tokenizer::UP_TOKEN:
                PrimitiveParser::parseVector(&givenVp->getUp(), ctx);
                break;

            case Tokenizer::RIGHT_TOKEN:
                PrimitiveParser::parseVector(&givenVp->getRight(), ctx);
                break;

            case Tokenizer::SKY_TOKEN:
                PrimitiveParser::parseVector(&givenVp->getSky(), ctx);
                break;

            case Tokenizer::LOOK_AT_TOKEN:
                directionLength = givenVp->getDirection().length();
                upLength = givenVp->getUp().length();
                rightLength = givenVp->getRight().length();
                tempVector = givenVp->getDirection().crossProduct(givenVp->getUp());
                handedness = tempVector.dotProduct(givenVp->getRight());
                PrimitiveParser::parseVector(&givenVp->getDirection(), ctx);

                givenVp->getDirection() =
                    givenVp->getDirection().subtract(givenVp->getLocation());
                givenVp->getDirection() = givenVp->getDirection().normalizedFast();
                givenVp->getRight() = givenVp->getDirection().crossProduct(givenVp->getSky());
                givenVp->getRight() = givenVp->getRight().normalizedFast();
                givenVp->getUp() = givenVp->getRight().crossProduct(givenVp->getDirection());
                givenVp->getDirection() = givenVp->getDirection().multiply(directionLength);
                if (handedness >= 0.0) {
                    givenVp->getRight() = givenVp->getRight().multiply(rightLength);
                } else {
                    givenVp->getRight() = givenVp->getRight().multiply(-rightLength);
                }

                givenVp->getUp() = givenVp->getUp().multiply(upLength);
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
