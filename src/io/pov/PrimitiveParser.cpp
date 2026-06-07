#include "io/pov/context/ParserContext.h"
#include "common/linealAlgebra/Transformation.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "common/color/Color.h"
#include "io/pov/ParseErrorReporter.h"
#include "io/pov/context/ParseGlobals.h"
#include "io/pov/ParseHelpers.h"
#include "io/pov/PrimitiveParser.h"



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
    int constantId;
    bool negative;
    bool signParsed;

    negative = false;
    signParsed = false;

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = ctx.findConstant()) != -1) {
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
                    ParseErrorReporter::reportUndeclared(ctx);
                }
                Exit_Flag = true;
                break;

            case Tokenizer::PLUS_TOKEN:
                if (signParsed) {
                    ParseErrorReporter::parseError(Tokenizer::FLOAT_TOKEN, ctx);
                }
                signParsed = true;
                break;

            case Tokenizer::DASH_TOKEN:
                if (signParsed) {
                    ParseErrorReporter::parseError(Tokenizer::FLOAT_TOKEN, ctx);
                }
                negative = true;
                signParsed = true;
                break;

            case Tokenizer::FLOAT_TOKEN:
                localFloat = ctx.token().tokenFloat;
                if (negative) {
                    localFloat *= -1.0;
                }
                Exit_Flag = true;
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
    int constantId;

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        ParseGlobals::VECTOR_CONSTANT) {
                        *givenVector = *((Vector3Dd *)ctx.constants()[(int)constantId]
                                .constantData);
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::reportUndeclared(ctx);
                }
                Exit_Flag = true;
                break;

            case Tokenizer::LEFT_ANGLE_TOKEN:
                (givenVector->x) = PrimitiveParser::parseFloat(ctx);
                (givenVector->y) = PrimitiveParser::parseFloat(ctx);
                (givenVector->z) = PrimitiveParser::parseFloat(ctx);
                ParseHelpers::getExpectedToken(Tokenizer::RIGHT_ANGLE_TOKEN, ctx);
                Exit_Flag = true;
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
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::LEFT_ANGLE_TOKEN:
                for (i = 0; i < ctx.termCounts()[order]; i++) {
                    givenCoeffs[i] = PrimitiveParser::parseFloat(ctx);
                }
                ParseHelpers::getExpectedToken(Tokenizer::RIGHT_ANGLE_TOKEN, ctx);
                Exit_Flag = true;
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
    int constantId;
    Color::makeColor(givenColour, 0.0, 0.0, 0.0);
    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        ParseGlobals::COLOUR_CONSTANT) {
                        *givenColour = *((RGBAColor *)ctx.constants()[(int)constantId]
                                .constantData);
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::reportUndeclared(ctx);
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
                Exit_Flag = true;
                break;
            }
        }
    }
}
