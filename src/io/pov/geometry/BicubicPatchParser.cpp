#include "java/util/PriorityQueue.txx"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

#include "environment/geometry/surface/parametric/ParametricPatch.h"
#include "environment/scene/ModelBuilder.h"
#include "environment/geometry/SimpleBody.h"

#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/geometry/BicubicPatchParser.h"
#include "io/pov/material/TextureParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"

SimpleBody *
BicubicPatchParser::parseBicubicPatch()
{
    ParserContext ctx;
    return BicubicPatchParser::parseBicubicPatch(ctx);
}

SimpleBody *
BicubicPatchParser::parseBicubicPatch(ParserContext &ctx)
{
    (void)ctx;
    ParametricBiCubicPatch *localShape = nullptr;
    SimpleBody *body = nullptr;
    Vector3Dd localVector;
    Vector3Dd controlPoints[4][4];

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        bool Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
            case Tokenizer::DASH_TOKEN:
            case Tokenizer::PLUS_TOKEN:
            case Tokenizer::FLOAT_TOKEN: {
                int patchType;
                int uSteps;
                int vSteps;
                double flatnessValue;
                int i;
                int j;
                ctx.tokenStream().ungetToken();
                patchType = (int)PrimitiveParser::parseFloat(ctx);
                if (patchType == 2 || patchType == 3) {
                    flatnessValue = PrimitiveParser::parseFloat(ctx);
                } else {
                    flatnessValue = 0.1;
                }
                uSteps = (int)PrimitiveParser::parseFloat(ctx);
                vSteps = (int)PrimitiveParser::parseFloat(ctx);
                for (i = 0; i < 4; i++) {
                    for (j = 0; j < 4; j++) {
                        PrimitiveParser::parseVector(&controlPoints[i][j], ctx);
                    }
                }
                localShape = new ParametricBiCubicPatch(
                    patchType, uSteps, vSteps, flatnessValue, controlPoints);
                body = ModelBuilder::wrap(localShape);
                ParametricBiCubicPatch::precomputePatchValues(
                    localShape); // interpolated mesh coords
                Exit_Flag = true;
                break;
            }

            case Tokenizer::IDENTIFIER_TOKEN: {
                int constantId;
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].getConstantType() ==
                        ParseGlobals::BICUBIC_PATCH_CONSTANT) {
                        body = new SimpleBody(
                                *(SimpleBody *)ctx.constants()[(int)constantId]
                                    .getConstantData());
                        localShape = (ParametricBiCubicPatch *)body->getGeometry();
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::reportUndeclared(ctx);
                }
                Exit_Flag = true;
                break;
            }

            default:
                ParseErrorReporter::parseError(Tokenizer::LEFT_ANGLE_TOKEN, ctx);
                break;
            }
        }
    }

    {
        bool Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = true;
                break;

            case Tokenizer::TRANSLATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                body->translate(&localVector);
                break;

            case Tokenizer::ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                body->rotate(&localVector);
                break;

            case Tokenizer::SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                body->scale(&localVector);
                break;

            case Tokenizer::INVERSE_TOKEN:
                body->invert();
                break;

            case Tokenizer::TEXTURE_TOKEN: {
                PovrayMaterial *localTexture = TextureParser::parseTexture(ctx);
                if (localTexture->isConstant()) {
                    localTexture = TextureParser::copyTexture(localTexture);
                }

                TextureParser::prependTextureLayers(localTexture, body);
                break;
            }

            case Tokenizer::COLOUR_TOKEN:
                PrimitiveParser::parseColor(body->ensureShapeColor(), ctx);
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    return body;
}
