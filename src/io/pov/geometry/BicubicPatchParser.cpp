#include "io/pov/context/ParserContext.h"
#include "io/pov/geometry/BicubicPatchParser.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/surface/parametric/ParametricPatch.h"
#include "environment/scene/SimpleBodyFactory.h"
#include "environment/scene/ModelBuilder.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/context/ParseGlobals.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"
#include "io/pov/texture/TextureParser.h"

Geometry *
BicubicPatchParser::parseBicubicPatch()
{
    ParserContext ctx;
    return BicubicPatchParser::parseBicubicPatch(ctx);
}

Geometry *
BicubicPatchParser::parseBicubicPatch(ParserContext &ctx)
{
    (void)ctx;
    ParametricBiCubicPatch *localShape = nullptr;
    Vector3Dd localVector;
    int constantId;
    Texture *localTexture;
    int i;
    int j;

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::DASH_TOKEN:
            case Tokenizer::PLUS_TOKEN:
            case Tokenizer::FLOAT_TOKEN:
                ctx.tokenStream().ungetToken();
                localShape = ModelBuilder::getBicubicPatchShape();
                localShape->patchType = (int)PrimitiveParser::parseFloat(ctx);
                if (localShape->patchType == 2 ||
                    localShape->patchType == 3) {
                    localShape->flatnessValue = PrimitiveParser::parseFloat(ctx);
                } else {
                    localShape->flatnessValue = 0.1;
                }
                localShape->uSteps = (int)PrimitiveParser::parseFloat(ctx);
                localShape->vSteps = (int)PrimitiveParser::parseFloat(ctx);
                for (i = 0; i < 4; i++) {
                    for (j = 0; j < 4; j++) {
                        PrimitiveParser::parseVector(
                            &(localShape->Control_Points[i][j]), ctx);
                    }
                }
                ParametricBiCubicPatch::precomputePatchValues(
                    localShape); /* interpolated mesh coords */
                Exit_Flag = true;
                break;

            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        ParseGlobals::BICUBIC_PATCH_CONSTANT) {
                        localShape =
                            (ParametricBiCubicPatch *)GeometryOperations::copy(
                                (SimpleBody *)ctx.constants()[(int)constantId]
                                    .constantData);
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::reportUndeclared(ctx);
                }
                Exit_Flag = true;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::LEFT_ANGLE_TOKEN, ctx);
                break;
            }
        }
    }

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = true;
                break;

            case Tokenizer::TRANSLATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::translate(
                    (SimpleBody *)localShape, &localVector);
                break;

            case Tokenizer::ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::rotate(
                    (SimpleBody *)localShape, &localVector);
                break;

            case Tokenizer::SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::scale(
                    (SimpleBody *)localShape, &localVector);
                break;

            case Tokenizer::INVERSE_TOKEN:
                GeometryOperations::invert((SimpleBody *)localShape);
                break;

            case Tokenizer::TEXTURE_TOKEN:
                localTexture = TextureParser::parseTexture(ctx);
                if (localTexture->constantFlag) {
                    localTexture = TextureParser::copyTexture(localTexture);
                }

                SimpleBodyFactory::link((SimpleBody *)localTexture,
                    (SimpleBody **)&localTexture->Next_Texture,
                    (SimpleBody **)&localShape->Shape_Texture);
                break;

            case Tokenizer::COLOUR_TOKEN:
                localShape->Shape_Colour = ModelBuilder::getColour();
                PrimitiveParser::parseColour(localShape->Shape_Colour, ctx);
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    return ((Geometry *)localShape);
}
