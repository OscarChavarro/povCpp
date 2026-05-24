#include "io/pov/ParserContext.h"
#include "common/LegacyBoolean.h"
#include "common/linealAlgebra/Transformation.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "io/image/DumpFormat.h"
#include "io/image/GifFormat.h"
#include "io/image/IffFormat.h"
#include "io/image/TargaFormat.h"
#include "io/pov/Parse.h"

#include "environment/camera/Camera.h"
#include "environment/geometry/elements/Triangle.h"
#include "environment/geometry/surface/InfinitePlane.h"
#include "environment/geometry/surface/parametric/ParametricPatch.h"
#include "environment/geometry/volume/Blob.h"
#include "environment/geometry/volume/Box.h"
#include "environment/geometry/volume/HeightField.h"
#include "environment/geometry/volume/Quadric.h"
#include "environment/geometry/volume/Sphere.h"
#include "environment/geometry/volume/compound/CSG.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "environment/geometry/volume/polynomial/PolynomialShape.h"
#include "environment/light/Light.h"



/* Parse a float.  Doesn't handle exponentiation. */
double
PrimitiveParser::parseFloat()
{
    ParserContext ctx;
    return PrimitiveParser::parseFloat(ctx);
}

double
PrimitiveParser::parseFloat(ParserContext &ctx)
{
    double localFloat = 0.0;
    CONSTANT constantId;
    int negative;
    int signParsed;

    negative = LegacyBoolean::FALSE_VALUE;
    signParsed = LegacyBoolean::FALSE_VALUE;

    {
        int Exit_Flag;
        Exit_Flag = LegacyBoolean::FALSE_VALUE;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant(ctx)) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        ParseGlobals::FLOAT_CONSTANT) {
                        localFloat = *(
                            (double *)ctx.constants()[(int)constantId].constantData);
                        if (negative) {
                            localFloat *= -1.0;
                        }
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::Undeclared(ctx);
                }
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::PLUS_TOKEN:
                if (signParsed) {
                    ParseErrorReporter::parseError(Tokenizer::FLOAT_TOKEN, ctx);
                }
                signParsed = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::DASH_TOKEN:
                if (signParsed) {
                    ParseErrorReporter::parseError(Tokenizer::FLOAT_TOKEN, ctx);
                }
                negative = LegacyBoolean::TRUE_VALUE;
                signParsed = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::FLOAT_TOKEN:
                localFloat = ctx.token().tokenFloat;
                if (negative) {
                    localFloat *= -1.0;
                }
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::FLOAT_TOKEN, ctx);
                break;
            }
        }
    }

    return (localFloat);
}

void
PrimitiveParser::parseVector(Vector3Dd *givenVector)
{
    ParserContext ctx;
    PrimitiveParser::parseVector(givenVector, ctx);
}

void
PrimitiveParser::parseVector(Vector3Dd *givenVector, ParserContext &ctx)
{
    CONSTANT constantId;

    {
        int Exit_Flag;
        Exit_Flag = LegacyBoolean::FALSE_VALUE;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant(ctx)) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        ParseGlobals::VECTOR_CONSTANT) {
                        *givenVector = *((Vector3Dd *)ctx.constants()[(int)constantId]
                                .constantData);
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::Undeclared(ctx);
                }
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::LEFT_ANGLE_TOKEN:
                (givenVector->x) = PrimitiveParser::parseFloat(ctx);
                (givenVector->y) = PrimitiveParser::parseFloat(ctx);
                (givenVector->z) = PrimitiveParser::parseFloat(ctx);
                ParseHelpers::getExpectedToken(Tokenizer::RIGHT_ANGLE_TOKEN, ctx);
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::LEFT_ANGLE_TOKEN, ctx);
                break;
            }
        }
    }
}

void
PrimitiveParser::parseCoeffs(int order, double *givenCoeffs)
{
    ParserContext ctx;
    PrimitiveParser::parseCoeffs(order, givenCoeffs, ctx);
}

void
PrimitiveParser::parseCoeffs(int order, double *givenCoeffs, ParserContext &ctx)
{
    int i;

    {
        int Exit_Flag;
        Exit_Flag = LegacyBoolean::FALSE_VALUE;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::LEFT_ANGLE_TOKEN:
                for (i = 0; i < ctx.termCounts()[order]; i++) {
                    givenCoeffs[i] = PrimitiveParser::parseFloat(ctx);
                }
                ParseHelpers::getExpectedToken(Tokenizer::RIGHT_ANGLE_TOKEN, ctx);
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::LEFT_ANGLE_TOKEN, ctx);
                break;
            }
        }
    }
}

void
PrimitiveParser::parseColour(RGBAColor *givenColour)
{
    ParserContext ctx;
    PrimitiveParser::parseColour(givenColour, ctx);
}

void
PrimitiveParser::parseColour(RGBAColor *givenColour, ParserContext &ctx)
{
    CONSTANT constantId;
    Color::makeColor(givenColour, 0.0, 0.0, 0.0);
    {
        int Exit_Flag;
        Exit_Flag = LegacyBoolean::FALSE_VALUE;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant(ctx)) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        ParseGlobals::COLOUR_CONSTANT) {
                        *givenColour = *((RGBAColor *)ctx.constants()[(int)constantId]
                                .constantData);
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::Undeclared(ctx);
                }
                break;

            case Tokenizer::RED_TOKEN:
                (givenColour->Red) = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::GREEN_TOKEN:
                (givenColour->Green) = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::BLUE_TOKEN:
                (givenColour->Blue) = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::ALPHA_TOKEN:
                (givenColour->Alpha) = PrimitiveParser::parseFloat(ctx);
                break;

            default:
                ctx.tokenStream().ungetToken();
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;
            }
        }
    }
}
