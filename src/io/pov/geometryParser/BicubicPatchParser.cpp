#include "io/pov/ParserContext.h"
#include "io/pov/geometryParser/BicubicPatchParser.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/surface/parametric/ParametricPatch.h"
#include "environment/scene/SimpleBodyFactory.h"
#include "io/pov/Parse.h"
#include "io/pov/ParseHelpers.h"
#include "io/pov/PrimitiveParser.h"
#include "io/pov/SceneConfigParser.h"
#include "io/pov/mediaParser/TextureParser.h"


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
    CONSTANT constantId;
    Texture *localTexture;
    int i;
    int j;

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        int Exit_Flag;
        Exit_Flag = LegacyBoolean::FALSE_VALUE;
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
                            &(localShape->Control_Points[i][j]));
                    }
                }
                ParametricBiCubicPatch::precomputePatchValues(
                    localShape); /* interpolated mesh coords */
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant(ctx)) != -1) {
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
                    ParseErrorReporter::Undeclared(ctx);
                }
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::LEFT_ANGLE_TOKEN, ctx);
                break;
            }
        }
    }

    {
        int Exit_Flag;
        Exit_Flag = LegacyBoolean::FALSE_VALUE;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
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
