

#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"
#include "java/util/PriorityQueue.txx"



// Parse a float.  Doesn't handle exponentiation
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
            switch (ctx.token().getTokenId()) {
            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].getConstantType() ==
                        ParseGlobals::FLOAT_CONSTANT) {
                        localFloat = *(
                            (double *)ctx.constants()[(int)constantId].getConstantData());
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
                localFloat = ctx.token().getTokenFloat();
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
            switch (ctx.token().getTokenId()) {
            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].getConstantType() ==
                        ParseGlobals::VECTOR_CONSTANT) {
                        *givenVector = *((Vector3Dd *)ctx.constants()[(int)constantId]
                                .getConstantData());
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::reportUndeclared(ctx);
                }
                Exit_Flag = true;
                break;

            case Tokenizer::LEFT_ANGLE_TOKEN: {
                // parseFloat consumes tokens; keep left-to-right order
                const double vx = PrimitiveParser::parseFloat(ctx);
                const double vy = PrimitiveParser::parseFloat(ctx);
                const double vz = PrimitiveParser::parseFloat(ctx);
                *givenVector = Vector3Dd(vx, vy, vz);
                ParseHelpers::getExpectedToken(Tokenizer::RIGHT_ANGLE_TOKEN, ctx);
                Exit_Flag = true;
                break;
            }

            default:
                ParseErrorReporter::parseError(Tokenizer::LEFT_ANGLE_TOKEN, ctx);
                break;
            }
        }
    }
}

void
PrimitiveParser::parseColor(ColorRgba *givenColor)
{
    ParserContext ctx;
    PrimitiveParser::parseColor(givenColor, ctx);
}

void
PrimitiveParser::parseColor(ColorRgba *givenColor, ParserContext &ctx)
{
    int constantId;
    givenColor->setR(0.0); givenColor->setG(0.0); givenColor->setB(0.0); givenColor->setA(0);
    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].getConstantType() ==
                        ParseGlobals::COLOUR_CONSTANT) {
                        *givenColor = *((ColorRgba *)ctx.constants()[(int)constantId]
                                .getConstantData());
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::reportUndeclared(ctx);
                }
                break;

            case Tokenizer::RED_TOKEN:
                givenColor->setR(PrimitiveParser::parseFloat(ctx));
                break;

            case Tokenizer::GREEN_TOKEN:
                givenColor->setG(PrimitiveParser::parseFloat(ctx));
                break;

            case Tokenizer::BLUE_TOKEN:
                givenColor->setB(PrimitiveParser::parseFloat(ctx));
                break;

            case Tokenizer::ALPHA_TOKEN:
                givenColor->setA(PrimitiveParser::parseFloat(ctx));
                break;

            default:
                ctx.tokenStream().ungetToken();
                Exit_Flag = true;
                break;
            }
        }
    }
}
