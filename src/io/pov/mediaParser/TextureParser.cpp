#include "io/pov/ParserContext.h"
#include "common/LegacyBoolean.h"
#include "common/logger/Logger.h"
#include "common/linealAlgebra/Transformation.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "io/base/image/DumpFormat.h"
#include "io/base/image/GifFormat.h"
#include "io/base/image/IffFormat.h"
#include "io/base/image/TargaFormat.h"
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

namespace {
bool shouldLogTextureState()
{
    const char *flag = std::getenv("POVCPP_DIAG_TEXTURE_STATE");
    return flag != nullptr && flag[0] != '\0';
}

void logTextureStateLegacy(const char *prefix, const Texture *texture)
{
    if (!shouldLogTextureState() || texture == nullptr) {
        return;
    }

    Logger::info(
        "[TEXTURE-STATE] %s type=%d ambient=%.6f diffuse=%.6f brilliance=%.6f reflection=%.6f turbulence=%.6f frequency=%.6f phase=%.6f octaves=%d bumpNumber=%d bumpAmount=%.6f texXform=%s\n",
        prefix,
        texture->textureNumber,
        texture->objectAmbient,
        texture->objectDiffuse,
        texture->objectBrilliance,
        texture->objectReflection,
        texture->Turbulence,
        texture->Frequency,
        texture->Phase,
        texture->Octaves,
        texture->bumpNumber,
        texture->bumpAmount,
        texture->Texture_Transformation != nullptr ? "yes" : "no");
    if (texture->Texture_Transformation != nullptr) {
        Logger::info(
            "[TEXTURE-STATE] %s xform row0=<%.6f,%.6f,%.6f,%.6f> row1=<%.6f,%.6f,%.6f,%.6f> row2=<%.6f,%.6f,%.6f,%.6f> row3=<%.6f,%.6f,%.6f,%.6f>\n",
            prefix,
            texture->Texture_Transformation->matrix[0][0], texture->Texture_Transformation->matrix[0][1],
            texture->Texture_Transformation->matrix[0][2], texture->Texture_Transformation->matrix[0][3],
            texture->Texture_Transformation->matrix[1][0], texture->Texture_Transformation->matrix[1][1],
            texture->Texture_Transformation->matrix[1][2], texture->Texture_Transformation->matrix[1][3],
            texture->Texture_Transformation->matrix[2][0], texture->Texture_Transformation->matrix[2][1],
            texture->Texture_Transformation->matrix[2][2], texture->Texture_Transformation->matrix[2][3],
            texture->Texture_Transformation->matrix[3][0], texture->Texture_Transformation->matrix[3][1],
            texture->Texture_Transformation->matrix[3][2], texture->Texture_Transformation->matrix[3][3]);
    }
}
}



Texture *
TextureParser::copyTexture(Texture *texture)
{
    return TextureUtils::copyTexture(texture);
}

Texture *
TextureParser::parseTexture()
{
    ParserContext ctx;
    return TextureParser::parseTexture(ctx);
}

Texture *
TextureParser::parseTexture(ParserContext &ctx)
{
    (void)ctx;
    Vector3Dd localVector;
    CONSTANT constantId;
    Texture *texture;
    Texture *localTexture;
    Texture *firstTexture;
    Texture *tempTexture;
    int reg;

    texture = TextureUtils::defaultTexture();

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        int Exit_Flag;
        Exit_Flag = LegacyBoolean::FALSE_VALUE;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant(ctx)) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        ParseGlobals::TEXTURE_CONSTANT) {
                        texture = ((Texture *)ctx.constants()[(int)constantId]
                                .constantData);
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::Undeclared(ctx);
                }
                break;

            case Tokenizer::FLOAT_TOKEN:
                ctx.tokenStream().ungetToken();
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->textureRandomness = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::ONCE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->onceFlag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::TURBULENCE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->Turbulence = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::OCTAVES_TOKEN: /* dmf 02/05 for turb */
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->Octaves = (int)PrimitiveParser::parseFloat(ctx);
                if (texture->Octaves < 1) {
                    texture->Octaves = 6;
                }
                if (texture->Octaves > 10) { /* Avoid DOMAIN errors */
                    texture->Octaves = 10;
                }
                break;

            case Tokenizer::BOZO_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->textureNumber = Texture::BOZO_TEXTURE;
                break;

            case Tokenizer::MORTAR_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->Mortar = PrimitiveParser::parseFloat(ctx);
                if (texture->Mortar < 0) {
                    texture->Mortar = 0.2;
                }
                break;

            case Tokenizer::BRICK_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->textureNumber = Texture::BRICK_TEXTURE;
                {
                    int Exit_Flag;
                    Exit_Flag = LegacyBoolean::FALSE_VALUE;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().tokenId) {
                        case Tokenizer::COLOUR_TOKEN:
                            texture->Colour1 = ModelBuilder::getColour();
                            texture->Colour2 = ModelBuilder::getColour();
                            PrimitiveParser::parseColour(texture->Colour1, ctx);
                            ParseHelpers::getExpectedToken(Tokenizer::COLOUR_TOKEN, ctx);
                            PrimitiveParser::parseColour(texture->Colour2, ctx);
                            break;

                        default:
                            ctx.tokenStream().ungetToken();
                            Exit_Flag = LegacyBoolean::TRUE_VALUE;
                            break;
                        }
                    }
                }
                break;

            case Tokenizer::CHECKER_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->textureNumber = Texture::CHECKER_TEXTURE;
                {
                    int Exit_Flag;
                    Exit_Flag = LegacyBoolean::FALSE_VALUE;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().tokenId) {
                        case Tokenizer::COLOUR_TOKEN:
                            texture->Colour1 = ModelBuilder::getColour();
                            texture->Colour2 = ModelBuilder::getColour();
                            PrimitiveParser::parseColour(texture->Colour1, ctx);
                            ParseHelpers::getExpectedToken(Tokenizer::COLOUR_TOKEN, ctx);
                            PrimitiveParser::parseColour(texture->Colour2, ctx);
                            break;

                        default:
                            ctx.tokenStream().ungetToken();
                            Exit_Flag = LegacyBoolean::TRUE_VALUE;
                            break;
                        }
                    }
                }
                break;

            case Tokenizer::CHECKER_TEXTURE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->textureNumber = Texture::CHECKER_TEXTURE_TEXTURE;

                ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

                {
                    int Exit_Flag;
                    Exit_Flag = LegacyBoolean::FALSE_VALUE;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().tokenId) {
                        case Tokenizer::TEXTURE_TOKEN:
                            localTexture = TextureParser::parseTexture(ctx);
                            if (localTexture->constantFlag) {
                                localTexture =
                                    TextureParser::copyTexture(localTexture);
                            }
                            {
                                for (tempTexture = localTexture;
                                    tempTexture->Next_Texture != nullptr;
                                    tempTexture = tempTexture->Next_Texture) {
                                }

                                tempTexture->Next_Texture =
                                    (Texture *)texture->Colour1;
                                texture->Colour1 = (RGBAColor *)localTexture;
                            }
                            break;
                        default:
                            ctx.tokenStream().ungetToken();
                            Exit_Flag = LegacyBoolean::TRUE_VALUE;
                            break;
                        }
                    }
                }

                ParseHelpers::getExpectedToken(Tokenizer::TILE2_TOKEN, ctx);
                {
                    int Exit_Flag;
                    Exit_Flag = LegacyBoolean::FALSE_VALUE;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().tokenId) {
                        case Tokenizer::TEXTURE_TOKEN:
                            localTexture = TextureParser::parseTexture(ctx);
                            if (localTexture->constantFlag) {
                                localTexture =
                                    TextureParser::copyTexture(localTexture);
                            }

                            {
                                for (tempTexture = localTexture;
                                    tempTexture->Next_Texture != nullptr;
                                    tempTexture = tempTexture->Next_Texture) {
                                }

                                tempTexture->Next_Texture =
                                    (Texture *)texture->Colour2;
                                texture->Colour2 = (RGBAColor *)localTexture;
                            }
                            break;
                        default:
                            ctx.tokenStream().ungetToken();
                            Exit_Flag = LegacyBoolean::TRUE_VALUE;
                            break;
                        }
                    }
                }
                ParseHelpers::getExpectedToken(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;

            case Tokenizer::MARBLE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->textureNumber = Texture::MARBLE_TEXTURE;
                break;

            case Tokenizer::WOOD_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->textureNumber = Texture::WOOD_TEXTURE;
                break;

            case Tokenizer::SPOTTED_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->textureNumber = Texture::SPOTTED_TEXTURE;
                break;

            case Tokenizer::AGATE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->textureNumber = Texture::AGATE_TEXTURE;
                break;

            case Tokenizer::GRANITE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->textureNumber = Texture::GRANITE_TEXTURE;
                break;

            case Tokenizer::GRADIENT_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->textureNumber = Texture::GRADIENT_TEXTURE;
                PrimitiveParser::parseVector(&(texture->textureGradient), ctx);
                break;

            case Tokenizer::AMBIENT_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                (texture->objectAmbient) = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::BRILLIANCE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                (texture->objectBrilliance) = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::ROUGHNESS_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                (texture->objectRoughness) = PrimitiveParser::parseFloat(ctx);
                /* No training wheels */
                /* if (texture -> objectRoughness > 1.0)
                    texture -> objectRoughness = 1.0;
                if (texture -> objectRoughness < 0.001)
                    texture -> objectRoughness = 0.001; */
                break;

            case Tokenizer::PHONGSIZE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                (texture->objectPhongSize) = PrimitiveParser::parseFloat(ctx);
                /* No training wheels */
                /*if (texture -> objectPhongSize < 1.0)
                    texture -> objectPhongSize = 1.0;
                if (texture -> objectPhongSize > 100)
                    texture -> objectPhongSize = 100; */
                break;

            case Tokenizer::DIFFUSE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                (texture->objectDiffuse) = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::SPECULAR_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                (texture->objectSpecular) = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::PHONG_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                (texture->objectPhong) = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::METALLIC_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->metallicFlag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::IOR_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                (texture->objectIndexOfRefraction) =
                    PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::REFRACTION_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                (texture->objectRefraction) = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::TRANSMIT_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                (texture->objectTransmit) = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::REFLECTION_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                (texture->objectReflection) = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::IMAGEMAP_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->textureNumber = Texture::IMAGEMAP_TEXTURE;
                texture->Image = new RGBAImage;
                if (texture->Image == nullptr) {
                    ParseErrorReporter::Error(
                        "Out of memory. Cannot allocate imagemap texture", ctx);
                }
                *&texture->Image->imageGradient = Vector3Dd(1.0, -1.0, 0.0);
                texture->Image->mapType = Texture::PLANAR_MAP;
                texture->Image->interpolationType = Texture::NO_INTERPOLATION;
                texture->Image->onceFlag = LegacyBoolean::FALSE_VALUE;
                texture->Image->useColourFlag = LegacyBoolean::TRUE_VALUE;

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
                            (texture->Image->mapType) =
                                (int)PrimitiveParser::parseFloat(ctx);
                            break;

                        case Tokenizer::LEFT_ANGLE_TOKEN:
                            ctx.tokenStream().ungetToken();
                            PrimitiveParser::parseVector(
                                &(texture->Image->imageGradient), ctx);
                            break;

                        case Tokenizer::IFF_TOKEN:
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            IffFormat::readIffImage(
                                texture->Image, ctx.token().Token_String);
                            Exit_Flag = LegacyBoolean::TRUE_VALUE;
                            break;

                        case Tokenizer::GIF_TOKEN:
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            GifFormat::readGifImage(
                                texture->Image, ctx.token().Token_String);
                            Exit_Flag = LegacyBoolean::TRUE_VALUE;
                            break;

                        case Tokenizer::TGA_TOKEN:
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            TargaFormat::readTargaImage(
                                texture->Image, ctx.token().Token_String);
                            Exit_Flag = LegacyBoolean::TRUE_VALUE;
                            break;

                        case Tokenizer::DUMP_TOKEN:
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            DumpFormat::readDumpImage(
                                texture->Image, ctx.token().Token_String);
                            Exit_Flag = LegacyBoolean::TRUE_VALUE;
                            break;

                        default:
                            ParseErrorReporter::parseError(Tokenizer::GIF_TOKEN, ctx);
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
                        case Tokenizer::ONCE_TOKEN:
                            texture->Image->onceFlag = LegacyBoolean::TRUE_VALUE;
                            break;

                        case Tokenizer::INTERPOLATE_TOKEN:
                            texture->Image->interpolationType =
                                (int)PrimitiveParser::parseFloat(ctx);
                            break;

                        case Tokenizer::MAPTYPE_TOKEN:
                            (texture->Image->mapType) =
                                (int)PrimitiveParser::parseFloat(ctx);
                            break;

                        case Tokenizer::USE_COLOUR_TOKEN:
                            texture->Image->useColourFlag = LegacyBoolean::TRUE_VALUE;
                            break;

                        case Tokenizer::USE_INDEX_TOKEN:
                            texture->Image->useColourFlag = LegacyBoolean::FALSE_VALUE;
                            break;

                        case Tokenizer::ALPHA_TOKEN: {
                            int Exit_Flag;
                            Exit_Flag = LegacyBoolean::FALSE_VALUE;
                            while (!Exit_Flag) {
                                ctx.tokenStream().getToken();
                                switch (ctx.token().tokenId) {
                                case Tokenizer::FLOAT_TOKEN:
                                    reg = (int)(ctx.token().tokenFloat + 0.01);
                                    if (texture->Image->Colour_Map == nullptr) {
                                        ParseErrorReporter::Error(
                                            "Can't apply ALPHA to a non "
                                            "colour-mapped image\n", ctx);
                                    }

                                    if ((reg < 0) ||
                                        (reg >=
                                            texture->Image->colourMapSize)) {
                                        ParseErrorReporter::Error(
                                            "ALPHA colour register value out "
                                            "of range.\n", ctx);
                                    }

                                    texture->Image->Colour_Map[reg].Alpha =
                                        (unsigned short)(255.0 *
                                                         PrimitiveParser::
                                                             parseFloat(ctx));
                                    Exit_Flag = LegacyBoolean::TRUE_VALUE;
                                    break;

                                case Tokenizer::ALL_TOKEN: {
                                    double alpha;
                                    alpha = PrimitiveParser::parseFloat(ctx);

                                    for (reg = 0;
                                        reg < texture->Image->colourMapSize;
                                        reg++) {
                                        texture->Image->Colour_Map[reg].Alpha =
                                            (unsigned short)(alpha * 255.0);
                                    }
                                    Exit_Flag = LegacyBoolean::TRUE_VALUE;
                                }

                                break;
                                }
                            }
                        } break;

                        case Tokenizer::RIGHT_CURLY_TOKEN:
                            Exit_Flag = LegacyBoolean::TRUE_VALUE;
                            break;

                        default:
                            ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                            break;
                        }
                    }
                }
                break;

            case Tokenizer::WAVES_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->bumpNumber = Texture::WAVES;
                texture->bumpAmount = PrimitiveParser::parseFloat(ctx);
                {
                    int Exit_Flag;
                    Exit_Flag = LegacyBoolean::FALSE_VALUE;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().tokenId) {
                        case Tokenizer::PHASE_TOKEN:
                            texture->Phase = PrimitiveParser::parseFloat(ctx);
                            Exit_Flag = LegacyBoolean::TRUE_VALUE;
                            break;

                        default:
                            ctx.tokenStream().ungetToken();
                            Exit_Flag = LegacyBoolean::TRUE_VALUE;
                            break;
                        }
                    }
                }
                break;

            case Tokenizer::FREQUENCY_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->Frequency = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::PHASE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->Phase = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::RIPPLES_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->bumpNumber = Texture::RIPPLES;
                texture->bumpAmount = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::WRINKLES_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->bumpNumber = Texture::WRINKLES;
                texture->bumpAmount = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::BUMPS_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->bumpNumber = Texture::BUMPS;
                texture->bumpAmount = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::DENTS_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->bumpNumber = Texture::DENTS;
                texture->bumpAmount = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::TRANSLATE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                PrimitiveParser::parseVector(&localVector, ctx);
                TextureUtils::translateTexture(&texture, &localVector);
                break;

            case Tokenizer::ROTATE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                PrimitiveParser::parseVector(&localVector, ctx);
                TextureUtils::rotateTexture(&texture, &localVector);
                break;

            case Tokenizer::SCALE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                PrimitiveParser::parseVector(&localVector, ctx);
                TextureUtils::scaleTexture(&texture, &localVector);
                break;

            case Tokenizer::COLOUR_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->Colour1 = ModelBuilder::getColour();
                PrimitiveParser::parseColour(texture->Colour1, ctx);
                texture->textureNumber = Texture::COLOUR_TEXTURE;
                break;

            case Tokenizer::COLOUR_MAP_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->Colour_Map = ColorMapParser::parseColorMap(ctx);
                break;

            case Tokenizer::ONION_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->textureNumber = Texture::ONION_TEXTURE;
                break;

            case Tokenizer::LEOPARD_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->textureNumber = Texture::LEOPARD_TEXTURE;
                break;

            /* New Texture Parsing - Cdw */
            case Tokenizer::PAINTED1_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->textureNumber = Texture::PAINTED1_TEXTURE;
                break;

            case Tokenizer::PAINTED2_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->textureNumber = Texture::PAINTED2_TEXTURE;
                break;

            case Tokenizer::PAINTED3_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->textureNumber = Texture::PAINTED3_TEXTURE;
                break;

            case Tokenizer::BUMPY1_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->bumpNumber = Texture::BUMPY1;
                texture->bumpAmount = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::BUMPY2_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->bumpNumber = Texture::BUMPY2;
                texture->bumpAmount = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::BUMPY3_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->bumpNumber = Texture::BUMPY3;
                texture->bumpAmount = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::BUMPMAP_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->bumpNumber = Texture::BUMPMAP;
                texture->Bump_Image = new RGBAImage;
                if (texture->Bump_Image == nullptr) {
                    ParseErrorReporter::Error(
                        "Out of memory. Cannot allocate bumpmap texture", ctx);
                }
                *&texture->Bump_Image->imageGradient =
                    Vector3Dd(1.0, -1.0, 0.0);
                texture->Bump_Image->mapType = Texture::PLANAR_MAP;
                texture->Bump_Image->interpolationType = Texture::NO_INTERPOLATION;
                texture->Bump_Image->onceFlag = LegacyBoolean::FALSE_VALUE;
                texture->Bump_Image->useColourFlag = LegacyBoolean::TRUE_VALUE;

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
                            (texture->Bump_Image->mapType) =
                                (int)PrimitiveParser::parseFloat(ctx);
                            break;

                        case Tokenizer::LEFT_ANGLE_TOKEN:
                            ctx.tokenStream().ungetToken();
                            PrimitiveParser::parseVector(
                                &(texture->Bump_Image->imageGradient), ctx);
                            break;

                        case Tokenizer::IFF_TOKEN:
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            IffFormat::readIffImage(
                                texture->Bump_Image, ctx.token().Token_String);
                            Exit_Flag = LegacyBoolean::TRUE_VALUE;
                            break;

                        case Tokenizer::GIF_TOKEN:
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            GifFormat::readGifImage(
                                texture->Bump_Image, ctx.token().Token_String);
                            Exit_Flag = LegacyBoolean::TRUE_VALUE;
                            break;

                        case Tokenizer::TGA_TOKEN:
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            TargaFormat::readTargaImage(
                                texture->Bump_Image, ctx.token().Token_String);
                            Exit_Flag = LegacyBoolean::TRUE_VALUE;
                            break;

                        case Tokenizer::DUMP_TOKEN:
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            DumpFormat::readDumpImage(
                                texture->Bump_Image, ctx.token().Token_String);
                            Exit_Flag = LegacyBoolean::TRUE_VALUE;
                            break;

                        default:
                            ParseErrorReporter::parseError(Tokenizer::GIF_TOKEN, ctx);
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
                        case Tokenizer::ONCE_TOKEN:
                            texture->Bump_Image->onceFlag = LegacyBoolean::TRUE_VALUE;
                            break;

                        case Tokenizer::MAPTYPE_TOKEN:
                            (texture->Bump_Image->mapType) =
                                (int)PrimitiveParser::parseFloat(ctx);
                            break;

                        case Tokenizer::INTERPOLATE_TOKEN:
                            texture->Bump_Image->interpolationType =
                                (int)PrimitiveParser::parseFloat(ctx);
                            break;

                        case Tokenizer::BUMPSIZE_TOKEN:
                            texture->bumpAmount =
                                PrimitiveParser::parseFloat(ctx);
                            break;

                        case Tokenizer::USE_COLOUR_TOKEN:
                            texture->Bump_Image->useColourFlag = LegacyBoolean::TRUE_VALUE;
                            break;
                        case Tokenizer::USE_INDEX_TOKEN:
                            texture->Bump_Image->useColourFlag = LegacyBoolean::FALSE_VALUE;
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
                break;

            case Tokenizer::MATERIAL_MAP_TOKEN:

                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = LegacyBoolean::FALSE_VALUE;
                }
                texture->textureNumber = Texture::MATERIAL_MAP_TEXTURE;
                texture->Material_Image = new RGBAImage;
                if (texture->Material_Image == nullptr) {
                    ParseErrorReporter::Error(
                        "Out of memory. Cannot allocate material map texture", ctx);
                }
                *&texture->textureGradient = Vector3Dd(1.0, -1.0, 0.0);
                texture->Material_Image->mapType = Texture::PLANAR_MAP;
                texture->Material_Image->interpolationType = Texture::NO_INTERPOLATION;
                texture->Material_Image->onceFlag = LegacyBoolean::FALSE_VALUE;
                texture->Material_Image->useColourFlag = LegacyBoolean::FALSE_VALUE;

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
                            (texture->Image->mapType) =
                                (int)PrimitiveParser::parseFloat(ctx);
                            break;

                        case Tokenizer::LEFT_ANGLE_TOKEN:
                            ctx.tokenStream().ungetToken();
                            PrimitiveParser::parseVector(
                                &(texture->Material_Image->imageGradient), ctx);
                            break;

                        case Tokenizer::IFF_TOKEN:
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            IffFormat::readIffImage(texture->Material_Image,
                                ctx.token().Token_String);
                            Exit_Flag = LegacyBoolean::TRUE_VALUE;
                            break;

                        case Tokenizer::GIF_TOKEN:
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            GifFormat::readGifImage(texture->Material_Image,
                                ctx.token().Token_String);
                            Exit_Flag = LegacyBoolean::TRUE_VALUE;
                            break;

                        case Tokenizer::TGA_TOKEN:
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            TargaFormat::readTargaImage(texture->Material_Image,
                                ctx.token().Token_String);
                            Exit_Flag = LegacyBoolean::TRUE_VALUE;
                            break;

                        case Tokenizer::DUMP_TOKEN:
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            DumpFormat::readDumpImage(texture->Material_Image,
                                ctx.token().Token_String);
                            Exit_Flag = LegacyBoolean::TRUE_VALUE;
                            break;

                        default:
                            ParseErrorReporter::parseError(Tokenizer::GIF_TOKEN, ctx);
                            break;
                        }
                    }
                }

                /* remember where the First_Texture is */
                firstTexture = texture;

                {
                    int Exit_Flag;
                    Exit_Flag = LegacyBoolean::FALSE_VALUE;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().tokenId) {

                        case Tokenizer::MAPTYPE_TOKEN:
                            (texture->Material_Image->mapType) =
                                (int)PrimitiveParser::parseFloat(ctx);
                            break;

                        case Tokenizer::INTERPOLATE_TOKEN:
                            texture->Material_Image->interpolationType =
                                (int)PrimitiveParser::parseFloat(ctx);
                            break;

                        case Tokenizer::ONCE_TOKEN:
                            texture->Material_Image->onceFlag = LegacyBoolean::TRUE_VALUE;
                            break;

                        case Tokenizer::TEXTURE_TOKEN: {
                            texture->Next_Material =
                                TextureParser::parseTexture(ctx);
                            firstTexture->numberOfMaterials++;
                            texture = texture->Next_Material;
                        }

                        break;

                        case Tokenizer::RIGHT_CURLY_TOKEN: {
                            texture->Next_Material = nullptr;
                            texture = firstTexture;
                            Exit_Flag = LegacyBoolean::TRUE_VALUE;
                        } break;

                        default:
                            ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                            break;
                        }
                    }
                }
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
    if (std::getenv("POVCPP_DIAG_MONKEY") != nullptr) {
        Logger::info(
            "[DIAG-MONKEY] legacy texture number=%d ambient=%.6f diffuse=%.6f brilliance=%.6f refraction=%.6f transmit=%.6f specular=%.6f roughness=%.6f phong=%.6f colour1=<%.6f,%.6f,%.6f,%.6f> colour2=<%.6f,%.6f,%.6f,%.6f> materials=%d\n",
            texture != nullptr ? texture->textureNumber : -1,
            texture != nullptr ? texture->objectAmbient : -1.0,
            texture != nullptr ? texture->objectDiffuse : -1.0,
            texture != nullptr ? texture->objectBrilliance : -1.0,
            texture != nullptr ? texture->objectIndexOfRefraction : -1.0,
            texture != nullptr ? texture->objectTransmit : -1.0,
            texture != nullptr ? texture->objectSpecular : -1.0,
            texture != nullptr ? texture->objectRoughness : -1.0,
            texture != nullptr ? texture->objectPhong : -1.0,
            texture != nullptr && texture->Colour1 != nullptr ? texture->Colour1->Red : -1.0,
            texture != nullptr && texture->Colour1 != nullptr ? texture->Colour1->Green : -1.0,
            texture != nullptr && texture->Colour1 != nullptr ? texture->Colour1->Blue : -1.0,
            texture != nullptr && texture->Colour1 != nullptr ? texture->Colour1->Alpha : -1.0,
            texture != nullptr && texture->Colour2 != nullptr ? texture->Colour2->Red : -1.0,
            texture != nullptr && texture->Colour2 != nullptr ? texture->Colour2->Green : -1.0,
            texture != nullptr && texture->Colour2 != nullptr ? texture->Colour2->Blue : -1.0,
            texture != nullptr && texture->Colour2 != nullptr ? texture->Colour2->Alpha : -1.0,
            texture != nullptr ? texture->numberOfMaterials : -1);
    }
    logTextureStateLegacy("legacy", texture);
    return (texture);
}
