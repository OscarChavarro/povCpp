#include "io/pov/ParserContext.h"
#include "io/pov/geometryParser/BicubicPatchParser.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/surface/parametric/ParametricPatch.h"
#include "environment/scene/SimpleBodyFactory.h"
#include "environment/scene/ModelBuilder.h"
#include "io/pov/ParseErrorReporter.h"
#include "io/pov/ParseGlobals.h"
#include "io/pov/ParseHelpers.h"
#include "io/pov/PrimitiveParser.h"
#include "io/pov/mediaParser/TextureParser.h"
#include "common/logger/Logger.h"
#include <cstdlib>

namespace {
bool shouldLogMonkeyDiagnostics()
{
    const char *flag = std::getenv("POVCPP_DIAG_MONKEY");
    return flag != nullptr && flag[0] != '\0';
}

void logBicubicPatchOnce(const char *prefix, const ParametricBiCubicPatch *shape)
{
    static int logged = 0;
    if (!shouldLogMonkeyDiagnostics() || logged++ > 0 || shape == nullptr) {
        return;
    }
    const Texture *texture = shape->Shape_Texture;
    const RGBAColor *colour = shape->Shape_Colour;
    Logger::info(
        "[DIAG-MONKEY] %s bicubic patchType=%d flatness=%.6f uSteps=%d vSteps=%d colour=<%.6f,%.6f,%.6f,%.6f> texNum=%d amb=%.6f diff=%.6f spec=%.6f rough=%.6f phong=%.6f cp00=<%.6f,%.6f,%.6f> cp33=<%.6f,%.6f,%.6f>\n",
        prefix, shape->patchType, shape->flatnessValue, shape->uSteps, shape->vSteps,
        colour != nullptr ? colour->Red : -1.0,
        colour != nullptr ? colour->Green : -1.0,
        colour != nullptr ? colour->Blue : -1.0,
        colour != nullptr ? colour->Alpha : -1.0,
        texture != nullptr ? texture->textureNumber : -1,
        texture != nullptr ? texture->objectAmbient : -1.0,
        texture != nullptr ? texture->objectDiffuse : -1.0,
        texture != nullptr ? texture->objectSpecular : -1.0,
        texture != nullptr ? texture->objectRoughness : -1.0,
        texture != nullptr ? texture->objectPhong : -1.0,
        shape->Control_Points[0][0].x, shape->Control_Points[0][0].y, shape->Control_Points[0][0].z,
        shape->Control_Points[3][3].x, shape->Control_Points[3][3].y, shape->Control_Points[3][3].z);
}
}

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
                    ParseErrorReporter::Undeclared(ctx);
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

    logBicubicPatchOnce("legacy", localShape);
    return ((Geometry *)localShape);
}
