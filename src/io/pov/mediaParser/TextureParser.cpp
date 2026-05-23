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

    texture = Default_Texture;

    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (ctx.token().tokenId) {
            case IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant(ctx)) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        TEXTURE_CONSTANT) {
                        texture = ((Texture *)ctx.constants()[(int)constantId]
                                .constantData);
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::Undeclared(ctx);
                }
                break;

            case FLOAT_TOKEN:
                Tokenizer::ungetToken();
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->textureRandomness = PrimitiveParser::parseFloat(ctx);
                break;

            case ONCE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->onceFlag = TRUE;
                break;

            case TURBULENCE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->Turbulence = PrimitiveParser::parseFloat(ctx);
                break;

            case OCTAVES_TOKEN: /* dmf 02/05 for turb */
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->Octaves = (int)PrimitiveParser::parseFloat(ctx);
                if (texture->Octaves < 1) {
                    texture->Octaves = 6;
                }
                if (texture->Octaves > 10) { /* Avoid DOMAIN errors */
                    texture->Octaves = 10;
                }
                break;

            case BOZO_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->textureNumber = BOZO_TEXTURE;
                break;

            case MORTAR_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->Mortar = PrimitiveParser::parseFloat(ctx);
                if (texture->Mortar < 0) {
                    texture->Mortar = 0.2;
                }
                break;

            case BRICK_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->textureNumber = BRICK_TEXTURE;
                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (ctx.token().tokenId) {
                        case COLOUR_TOKEN:
                            texture->Colour1 = SceneFactory::getColour();
                            texture->Colour2 = SceneFactory::getColour();
                            PrimitiveParser::parseColour(texture->Colour1, ctx);
                            ParseHelpers::getExpectedToken(COLOUR_TOKEN, ctx);
                            PrimitiveParser::parseColour(texture->Colour2, ctx);
                            break;

                        default:
                            Tokenizer::ungetToken();
                            Exit_Flag = TRUE;
                            break;
                        }
                    }
                }
                break;

            case CHECKER_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->textureNumber = CHECKER_TEXTURE;
                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (ctx.token().tokenId) {
                        case COLOUR_TOKEN:
                            texture->Colour1 = SceneFactory::getColour();
                            texture->Colour2 = SceneFactory::getColour();
                            PrimitiveParser::parseColour(texture->Colour1, ctx);
                            ParseHelpers::getExpectedToken(COLOUR_TOKEN, ctx);
                            PrimitiveParser::parseColour(texture->Colour2, ctx);
                            break;

                        default:
                            Tokenizer::ungetToken();
                            Exit_Flag = TRUE;
                            break;
                        }
                    }
                }
                break;

            case CHECKER_TEXTURE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->textureNumber = CHECKER_TEXTURE_TEXTURE;

                ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);

                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (ctx.token().tokenId) {
                        case TEXTURE_TOKEN:
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
                            Tokenizer::ungetToken();
                            Exit_Flag = TRUE;
                            break;
                        }
                    }
                }

                ParseHelpers::getExpectedToken(TILE2_TOKEN, ctx);
                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (ctx.token().tokenId) {
                        case TEXTURE_TOKEN:
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
                            Tokenizer::ungetToken();
                            Exit_Flag = TRUE;
                            break;
                        }
                    }
                }
                ParseHelpers::getExpectedToken(RIGHT_CURLY_TOKEN, ctx);
                break;

            case MARBLE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->textureNumber = MARBLE_TEXTURE;
                break;

            case WOOD_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->textureNumber = WOOD_TEXTURE;
                break;

            case SPOTTED_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->textureNumber = SPOTTED_TEXTURE;
                break;

            case AGATE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->textureNumber = AGATE_TEXTURE;
                break;

            case GRANITE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->textureNumber = GRANITE_TEXTURE;
                break;

            case GRADIENT_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->textureNumber = GRADIENT_TEXTURE;
                PrimitiveParser::parseVector(&(texture->textureGradient), ctx);
                break;

            case AMBIENT_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                (texture->objectAmbient) = PrimitiveParser::parseFloat(ctx);
                break;

            case BRILLIANCE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                (texture->objectBrilliance) = PrimitiveParser::parseFloat(ctx);
                break;

            case ROUGHNESS_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                (texture->objectRoughness) = PrimitiveParser::parseFloat(ctx);
                /* No training wheels */
                /* if (texture -> objectRoughness > 1.0)
                    texture -> objectRoughness = 1.0;
                if (texture -> objectRoughness < 0.001)
                    texture -> objectRoughness = 0.001; */
                break;

            case PHONGSIZE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                (texture->objectPhongSize) = PrimitiveParser::parseFloat(ctx);
                /* No training wheels */
                /*if (texture -> objectPhongSize < 1.0)
                    texture -> objectPhongSize = 1.0;
                if (texture -> objectPhongSize > 100)
                    texture -> objectPhongSize = 100; */
                break;

            case DIFFUSE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                (texture->objectDiffuse) = PrimitiveParser::parseFloat(ctx);
                break;

            case SPECULAR_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                (texture->objectSpecular) = PrimitiveParser::parseFloat(ctx);
                break;

            case PHONG_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                (texture->objectPhong) = PrimitiveParser::parseFloat(ctx);
                break;

            case METALLIC_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->metallicFlag = TRUE;
                break;

            case IOR_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                (texture->objectIndexOfRefraction) =
                    PrimitiveParser::parseFloat(ctx);
                break;

            case REFRACTION_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                (texture->objectRefraction) = PrimitiveParser::parseFloat(ctx);
                break;

            case TRANSMIT_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                (texture->objectTransmit) = PrimitiveParser::parseFloat(ctx);
                break;

            case REFLECTION_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                (texture->objectReflection) = PrimitiveParser::parseFloat(ctx);
                break;

            case IMAGEMAP_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->textureNumber = IMAGEMAP_TEXTURE;
                texture->Image = new RGBAImage;
                if (texture->Image == nullptr) {
                    ParseErrorReporter::Error(
                        "Out of memory. Cannot allocate imagemap texture", ctx);
                }
                *&texture->Image->imageGradient = Vector3Dd(1.0, -1.0, 0.0);
                texture->Image->mapType = PLANAR_MAP;
                texture->Image->interpolationType = NO_INTERPOLATION;
                texture->Image->onceFlag = FALSE;
                texture->Image->useColourFlag = TRUE;

                ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);

                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (ctx.token().tokenId) {
                        case DASH_TOKEN:
                        case PLUS_TOKEN:
                        case FLOAT_TOKEN:
                            Tokenizer::ungetToken();
                            (texture->Image->mapType) =
                                (int)PrimitiveParser::parseFloat(ctx);
                            break;

                        case LEFT_ANGLE_TOKEN:
                            Tokenizer::ungetToken();
                            PrimitiveParser::parseVector(
                                &(texture->Image->imageGradient));
                            break;

                        case IFF_TOKEN:
                            ParseHelpers::getExpectedToken(STRING_TOKEN, ctx);
                            IffFormat::readIffImage(
                                texture->Image, ctx.token().Token_String);
                            Exit_Flag = TRUE;
                            break;

                        case GIF_TOKEN:
                            ParseHelpers::getExpectedToken(STRING_TOKEN, ctx);
                            GifFormat::readGifImage(
                                texture->Image, ctx.token().Token_String);
                            Exit_Flag = TRUE;
                            break;

                        case TGA_TOKEN:
                            ParseHelpers::getExpectedToken(STRING_TOKEN, ctx);
                            TargaFormat::readTargaImage(
                                texture->Image, ctx.token().Token_String);
                            Exit_Flag = TRUE;
                            break;

                        case DUMP_TOKEN:
                            ParseHelpers::getExpectedToken(STRING_TOKEN, ctx);
                            DumpFormat::readDumpImage(
                                texture->Image, ctx.token().Token_String);
                            Exit_Flag = TRUE;
                            break;

                        default:
                            ParseErrorReporter::parseError(GIF_TOKEN, ctx);
                            break;
                        }
                    }
                }

                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (ctx.token().tokenId) {
                        case ONCE_TOKEN:
                            texture->Image->onceFlag = TRUE;
                            break;

                        case INTERPOLATE_TOKEN:
                            texture->Image->interpolationType =
                                (int)PrimitiveParser::parseFloat(ctx);
                            break;

                        case MAPTYPE_TOKEN:
                            (texture->Image->mapType) =
                                (int)PrimitiveParser::parseFloat(ctx);
                            break;

                        case USE_COLOUR_TOKEN:
                            texture->Image->useColourFlag = TRUE;
                            break;

                        case USE_INDEX_TOKEN:
                            texture->Image->useColourFlag = FALSE;
                            break;

                        case ALPHA_TOKEN: {
                            int Exit_Flag;
                            Exit_Flag = FALSE;
                            while (!Exit_Flag) {
                                Tokenizer::getToken();
                                switch (ctx.token().tokenId) {
                                case FLOAT_TOKEN:
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
                                    Exit_Flag = TRUE;
                                    break;

                                case ALL_TOKEN: {
                                    double alpha;
                                    alpha = PrimitiveParser::parseFloat(ctx);

                                    for (reg = 0;
                                        reg < texture->Image->colourMapSize;
                                        reg++) {
                                        texture->Image->Colour_Map[reg].Alpha =
                                            (unsigned short)(alpha * 255.0);
                                    }
                                    Exit_Flag = TRUE;
                                }

                                break;
                                }
                            }
                        } break;

                        case RIGHT_CURLY_TOKEN:
                            Exit_Flag = TRUE;
                            break;

                        default:
                            ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN, ctx);
                            break;
                        }
                    }
                }
                break;

            case WAVES_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->bumpNumber = WAVES;
                texture->bumpAmount = PrimitiveParser::parseFloat(ctx);
                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (ctx.token().tokenId) {
                        case PHASE_TOKEN:
                            texture->Phase = PrimitiveParser::parseFloat(ctx);
                            Exit_Flag = TRUE;
                            break;

                        default:
                            Tokenizer::ungetToken();
                            Exit_Flag = TRUE;
                            break;
                        }
                    }
                }
                break;

            case FREQUENCY_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->Frequency = PrimitiveParser::parseFloat(ctx);
                break;

            case PHASE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->Phase = PrimitiveParser::parseFloat(ctx);
                break;

            case RIPPLES_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->bumpNumber = RIPPLES;
                texture->bumpAmount = PrimitiveParser::parseFloat(ctx);
                break;

            case WRINKLES_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->bumpNumber = WRINKLES;
                texture->bumpAmount = PrimitiveParser::parseFloat(ctx);
                break;

            case BUMPS_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->bumpNumber = BUMPS;
                texture->bumpAmount = PrimitiveParser::parseFloat(ctx);
                break;

            case DENTS_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->bumpNumber = DENTS;
                texture->bumpAmount = PrimitiveParser::parseFloat(ctx);
                break;

            case TRANSLATE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                PrimitiveParser::parseVector(&localVector, ctx);
                TextureUtils::translateTexture(&texture, &localVector);
                break;

            case ROTATE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                PrimitiveParser::parseVector(&localVector, ctx);
                TextureUtils::rotateTexture(&texture, &localVector);
                break;

            case SCALE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                PrimitiveParser::parseVector(&localVector, ctx);
                TextureUtils::scaleTexture(&texture, &localVector);
                break;

            case COLOUR_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->Colour1 = SceneFactory::getColour();
                PrimitiveParser::parseColour(texture->Colour1, ctx);
                texture->textureNumber = COLOUR_TEXTURE;
                break;

            case COLOUR_MAP_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->Colour_Map = ColorMapParser::parseColorMap(ctx);
                break;

            case ONION_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->textureNumber = ONION_TEXTURE;
                break;

            case LEOPARD_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->textureNumber = LEOPARD_TEXTURE;
                break;

            /* New Texture Parsing - Cdw */
            case PAINTED1_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->textureNumber = PAINTED1_TEXTURE;
                break;

            case PAINTED2_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->textureNumber = PAINTED2_TEXTURE;
                break;

            case PAINTED3_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->textureNumber = PAINTED3_TEXTURE;
                break;

            case BUMPY1_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->bumpNumber = BUMPY1;
                texture->bumpAmount = PrimitiveParser::parseFloat(ctx);
                break;

            case BUMPY2_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->bumpNumber = BUMPY2;
                texture->bumpAmount = PrimitiveParser::parseFloat(ctx);
                break;

            case BUMPY3_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->bumpNumber = BUMPY3;
                texture->bumpAmount = PrimitiveParser::parseFloat(ctx);
                break;

            case BUMPMAP_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->bumpNumber = BUMPMAP;
                texture->Bump_Image = new RGBAImage;
                if (texture->Bump_Image == nullptr) {
                    ParseErrorReporter::Error(
                        "Out of memory. Cannot allocate bumpmap texture", ctx);
                }
                *&texture->Bump_Image->imageGradient =
                    Vector3Dd(1.0, -1.0, 0.0);
                texture->Bump_Image->mapType = PLANAR_MAP;
                texture->Bump_Image->interpolationType = NO_INTERPOLATION;
                texture->Bump_Image->onceFlag = FALSE;
                texture->Bump_Image->useColourFlag = TRUE;

                ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);

                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (ctx.token().tokenId) {
                        case DASH_TOKEN:
                        case PLUS_TOKEN:
                        case FLOAT_TOKEN:
                            Tokenizer::ungetToken();
                            (texture->Bump_Image->mapType) =
                                (int)PrimitiveParser::parseFloat(ctx);
                            break;

                        case LEFT_ANGLE_TOKEN:
                            Tokenizer::ungetToken();
                            PrimitiveParser::parseVector(
                                &(texture->Bump_Image->imageGradient));
                            break;

                        case IFF_TOKEN:
                            ParseHelpers::getExpectedToken(STRING_TOKEN, ctx);
                            IffFormat::readIffImage(
                                texture->Bump_Image, ctx.token().Token_String);
                            Exit_Flag = TRUE;
                            break;

                        case GIF_TOKEN:
                            ParseHelpers::getExpectedToken(STRING_TOKEN, ctx);
                            GifFormat::readGifImage(
                                texture->Bump_Image, ctx.token().Token_String);
                            Exit_Flag = TRUE;
                            break;

                        case TGA_TOKEN:
                            ParseHelpers::getExpectedToken(STRING_TOKEN, ctx);
                            TargaFormat::readTargaImage(
                                texture->Bump_Image, ctx.token().Token_String);
                            Exit_Flag = TRUE;
                            break;

                        case DUMP_TOKEN:
                            ParseHelpers::getExpectedToken(STRING_TOKEN, ctx);
                            DumpFormat::readDumpImage(
                                texture->Bump_Image, ctx.token().Token_String);
                            Exit_Flag = TRUE;
                            break;

                        default:
                            ParseErrorReporter::parseError(GIF_TOKEN, ctx);
                            break;
                        }
                    }
                }

                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (ctx.token().tokenId) {
                        case ONCE_TOKEN:
                            texture->Bump_Image->onceFlag = TRUE;
                            break;

                        case MAPTYPE_TOKEN:
                            (texture->Bump_Image->mapType) =
                                (int)PrimitiveParser::parseFloat(ctx);
                            break;

                        case INTERPOLATE_TOKEN:
                            texture->Bump_Image->interpolationType =
                                (int)PrimitiveParser::parseFloat(ctx);
                            break;

                        case BUMPSIZE_TOKEN:
                            texture->bumpAmount =
                                PrimitiveParser::parseFloat(ctx);
                            break;

                        case USE_COLOUR_TOKEN:
                            texture->Bump_Image->useColourFlag = TRUE;
                            break;
                        case USE_INDEX_TOKEN:
                            texture->Bump_Image->useColourFlag = FALSE;
                            break;

                        case RIGHT_CURLY_TOKEN:
                            Exit_Flag = TRUE;
                            break;
                        default:
                            ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN, ctx);
                            break;
                        }
                    }
                }
                break;

            case MATERIAL_MAP_TOKEN:

                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = FALSE;
                }
                texture->textureNumber = MATERIAL_MAP_TEXTURE;
                texture->Material_Image = new RGBAImage;
                if (texture->Material_Image == nullptr) {
                    ParseErrorReporter::Error(
                        "Out of memory. Cannot allocate material map texture", ctx);
                }
                *&texture->textureGradient = Vector3Dd(1.0, -1.0, 0.0);
                texture->Material_Image->mapType = PLANAR_MAP;
                texture->Material_Image->interpolationType = NO_INTERPOLATION;
                texture->Material_Image->onceFlag = FALSE;
                texture->Material_Image->useColourFlag = FALSE;

                ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);

                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (ctx.token().tokenId) {
                        case DASH_TOKEN:
                        case PLUS_TOKEN:
                        case FLOAT_TOKEN:
                            Tokenizer::ungetToken();
                            (texture->Image->mapType) =
                                (int)PrimitiveParser::parseFloat(ctx);
                            break;

                        case LEFT_ANGLE_TOKEN:
                            Tokenizer::ungetToken();
                            PrimitiveParser::parseVector(
                                &(texture->Material_Image->imageGradient));
                            break;

                        case IFF_TOKEN:
                            ParseHelpers::getExpectedToken(STRING_TOKEN, ctx);
                            IffFormat::readIffImage(texture->Material_Image,
                                ctx.token().Token_String);
                            Exit_Flag = TRUE;
                            break;

                        case GIF_TOKEN:
                            ParseHelpers::getExpectedToken(STRING_TOKEN, ctx);
                            GifFormat::readGifImage(texture->Material_Image,
                                ctx.token().Token_String);
                            Exit_Flag = TRUE;
                            break;

                        case TGA_TOKEN:
                            ParseHelpers::getExpectedToken(STRING_TOKEN, ctx);
                            TargaFormat::readTargaImage(texture->Material_Image,
                                ctx.token().Token_String);
                            Exit_Flag = TRUE;
                            break;

                        case DUMP_TOKEN:
                            ParseHelpers::getExpectedToken(STRING_TOKEN, ctx);
                            DumpFormat::readDumpImage(texture->Material_Image,
                                ctx.token().Token_String);
                            Exit_Flag = TRUE;
                            break;

                        default:
                            ParseErrorReporter::parseError(GIF_TOKEN, ctx);
                            break;
                        }
                    }
                }

                /* remember where the First_Texture is */
                firstTexture = texture;

                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (ctx.token().tokenId) {

                        case MAPTYPE_TOKEN:
                            (texture->Material_Image->mapType) =
                                (int)PrimitiveParser::parseFloat(ctx);
                            break;

                        case INTERPOLATE_TOKEN:
                            texture->Material_Image->interpolationType =
                                (int)PrimitiveParser::parseFloat(ctx);
                            break;

                        case ONCE_TOKEN:
                            texture->Material_Image->onceFlag = TRUE;
                            break;

                        case TEXTURE_TOKEN: {
                            texture->Next_Material =
                                TextureParser::parseTexture(ctx);
                            firstTexture->numberOfMaterials++;
                            texture = texture->Next_Material;
                        }

                        break;

                        case RIGHT_CURLY_TOKEN: {
                            texture->Next_Material = nullptr;
                            texture = firstTexture;
                            Exit_Flag = TRUE;
                        } break;

                        default:
                            ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN, ctx);
                            break;
                        }
                    }
                }
                break;

            case RIGHT_CURLY_TOKEN:
                Exit_Flag = TRUE;
                break;

            default:
                ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }
    return (texture);
}
