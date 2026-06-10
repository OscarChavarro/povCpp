#include "io/pov/texture/TextureParser.h"
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
#include "environment/material/MaterialUtils.h"
#include "environment/scene/ModelBuilder.h"
#include "io/image/GifFormat.h"
#include "io/image/IffFormat.h"
#include "io/image/RawDumpFormat.h"
#include "io/image/TargaFormat.h"
#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"
#include "io/pov/texture/ColorMapParser.h"
#include "java/util/ArrayList.txx"
#include "solidTexture/SolidTextureBitmapInterpolationTypes.h"
#include "solidTexture/SolidTextureBumpyTextures.h"
#include "solidTexture/SolidTextureColorTextures.h"
#include "solidTexture/SolidTextureProjectionMethods.h"
#include "solidTexture/TextureImage.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/media/IndexedColorImageHDRUncompressed.h"

static void
wireIndexedInToTextureImage(TextureImage *ti, IndexedColorImageHDRUncompressed *idx)
{
    ti->setIndexedData(idx);
    ti->allocate(idx->getXSize(), idx->getYSize());
}

bool
TextureParser::shouldLogTextureState()
{
    const char *flag = std::getenv("POVCPP_DIAG_TEXTURE_STATE");
    return flag != nullptr && flag[0] != '\0';
}

void
TextureParser::logTextureStateLegacy(const char *prefix, const Material *texture)
{
    if (!shouldLogTextureState() || texture == nullptr) {
        return;
    }

    {
        char _logMsg[1024];
        snprintf(_logMsg, sizeof(_logMsg), "[TEXTURE-STATE] %s type=%d ambient=%.6f diffuse=%.6f brilliance=%.6f reflection=%.6f turbulence=%.6f frequency=%.6f phase=%.6f octaves=%d bumpNumber=%d bumpAmount=%.6f texXform=%s\n", prefix,         texture->textureNumber,         texture->objectAmbient,         texture->objectDiffuse,         texture->objectBrilliance,         texture->objectReflection,         texture->turbulence,         texture->frequency,         texture->phase,         texture->octaves,         texture->bumpNumber,         texture->bumpAmount,         texture->textureTransformation != nullptr ? "yes" : "no");
        Logger::reportMessage("TextureParser", Logger::WARNING, "", _logMsg);
    }
    if (texture->textureTransformation != nullptr) {
        {
            char _logMsg[1024];
            snprintf(_logMsg, sizeof(_logMsg), "[TEXTURE-STATE] %s xform row0=<%.6f,%.6f,%.6f,%.6f> row1=<%.6f,%.6f,%.6f,%.6f> row2=<%.6f,%.6f,%.6f,%.6f> row3=<%.6f,%.6f,%.6f,%.6f>\n", prefix,             texture->textureTransformation->get(0, 0), texture->textureTransformation->get(0, 1),             texture->textureTransformation->get(0, 2), texture->textureTransformation->get(0, 3),             texture->textureTransformation->get(1, 0), texture->textureTransformation->get(1, 1),             texture->textureTransformation->get(1, 2), texture->textureTransformation->get(1, 3),             texture->textureTransformation->get(2, 0), texture->textureTransformation->get(2, 1),             texture->textureTransformation->get(2, 2), texture->textureTransformation->get(2, 3),             texture->textureTransformation->get(3, 0), texture->textureTransformation->get(3, 1),             texture->textureTransformation->get(3, 2), texture->textureTransformation->get(3, 3));
            Logger::reportMessage("TextureParser", Logger::WARNING, "", _logMsg);
        }
    }
}



Material *
TextureParser::copyTexture(Material *texture)
{
    return TextureUtils::instance().copyTexture(texture);
}

void
TextureParser::prependTextureLayers(Material *newHead, Material *&existingHead)
{
    if (existingHead != nullptr) {
        newHead->layers.add(existingHead);
        for (long int i = 0; i < existingHead->layers.size(); i++) {
            newHead->layers.add(existingHead->layers[i]);
        }
        existingHead->layers.clear();
    }
    existingHead = newHead;
}

Material *
TextureParser::parseTexture()
{
    ParserContext ctx;
    return TextureParser::parseTexture(ctx);
}

Material *
TextureParser::parseTexture(ParserContext &ctx)
{
    (void)ctx;
    Vector3Dd localVector;
    int constantId;
    Material *texture;
    Material *localTexture;
    Material *firstTexture;
    int reg;

    texture = MaterialUtils::instance().defaultTexture();

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        ParseGlobals::TEXTURE_CONSTANT) {
                        texture = ((Material *)ctx.constants()[(int)constantId]
                                .constantData);
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::reportUndeclared(ctx);
                }
                break;

            case Tokenizer::FLOAT_TOKEN:
                ctx.tokenStream().ungetToken();
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->textureRandomness = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::ONCE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->onceFlag = true;
                break;

            case Tokenizer::TURBULENCE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->turbulence = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::OCTAVES_TOKEN: /* dmf 02/05 for turb */
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->octaves = (int)PrimitiveParser::parseFloat(ctx);
                if (texture->octaves < 1) {
                    texture->octaves = 6;
                }
                if (texture->octaves > 10) { /* Avoid DOMAIN errors */
                    texture->octaves = 10;
                }
                break;

            case Tokenizer::BOZO_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->textureNumber = (int)SolidTextureColorTextures::BOZO_TEXTURE;
                break;

            case Tokenizer::MORTAR_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->mortar = PrimitiveParser::parseFloat(ctx);
                if (texture->mortar < 0) {
                    texture->mortar = 0.2;
                }
                break;

            case Tokenizer::BRICK_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->textureNumber = (int)SolidTextureColorTextures::BRICK_TEXTURE;
                {
                    bool Exit_Flag;
                    Exit_Flag = false;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().tokenId) {
                        case Tokenizer::COLOUR_TOKEN:
                            texture->color1 = ModelBuilder::getColor();
                            texture->color2 = ModelBuilder::getColor();
                            PrimitiveParser::parseColor(texture->color1, ctx);
                            ParseHelpers::getExpectedToken(Tokenizer::COLOUR_TOKEN, ctx);
                            PrimitiveParser::parseColor(texture->color2, ctx);
                            break;

                        default:
                            ctx.tokenStream().ungetToken();
                            Exit_Flag = true;
                            break;
                        }
                    }
                }
                break;

            case Tokenizer::CHECKER_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->textureNumber = (int)SolidTextureColorTextures::CHECKER_TEXTURE;
                {
                    bool Exit_Flag;
                    Exit_Flag = false;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().tokenId) {
                        case Tokenizer::COLOUR_TOKEN:
                            texture->color1 = ModelBuilder::getColor();
                            texture->color2 = ModelBuilder::getColor();
                            PrimitiveParser::parseColor(texture->color1, ctx);
                            ParseHelpers::getExpectedToken(Tokenizer::COLOUR_TOKEN, ctx);
                            PrimitiveParser::parseColor(texture->color2, ctx);
                            break;

                        default:
                            ctx.tokenStream().ungetToken();
                            Exit_Flag = true;
                            break;
                        }
                    }
                }
                break;

            case Tokenizer::CHECKER_TEXTURE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->textureNumber = (int)SolidTextureColorTextures::CHECKER_TEXTURE_TEXTURE;

                ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

                {
                    bool Exit_Flag;
                    Exit_Flag = false;
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
                                Material *color1Head = (Material *)texture->color1;
                                TextureParser::prependTextureLayers(localTexture, color1Head);
                                texture->color1 = (ColorRgba *)color1Head;
                            }
                            break;
                        default:
                            ctx.tokenStream().ungetToken();
                            Exit_Flag = true;
                            break;
                        }
                    }
                }

                ParseHelpers::getExpectedToken(Tokenizer::TILE2_TOKEN, ctx);
                {
                    bool Exit_Flag;
                    Exit_Flag = false;
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
                                Material *color2Head = (Material *)texture->color2;
                                TextureParser::prependTextureLayers(localTexture, color2Head);
                                texture->color2 = (ColorRgba *)color2Head;
                            }
                            break;
                        default:
                            ctx.tokenStream().ungetToken();
                            Exit_Flag = true;
                            break;
                        }
                    }
                }
                ParseHelpers::getExpectedToken(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;

            case Tokenizer::MARBLE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->textureNumber = (int)SolidTextureColorTextures::MARBLE_TEXTURE;
                break;

            case Tokenizer::WOOD_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->textureNumber = (int)SolidTextureColorTextures::WOOD_TEXTURE;
                break;

            case Tokenizer::SPOTTED_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->textureNumber = (int)SolidTextureColorTextures::SPOTTED_TEXTURE;
                break;

            case Tokenizer::AGATE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->textureNumber = (int)SolidTextureColorTextures::AGATE_TEXTURE;
                break;

            case Tokenizer::GRANITE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->textureNumber = (int)SolidTextureColorTextures::GRANITE_TEXTURE;
                break;

            case Tokenizer::GRADIENT_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->textureNumber = (int)SolidTextureColorTextures::GRADIENT_TEXTURE;
                PrimitiveParser::parseVector(&(texture->textureGradient), ctx);
                break;

            case Tokenizer::AMBIENT_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                (texture->objectAmbient) = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::BRILLIANCE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                (texture->objectBrilliance) = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::ROUGHNESS_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
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
                    texture->constantFlag = false;
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
                    texture->constantFlag = false;
                }
                (texture->objectDiffuse) = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::SPECULAR_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                (texture->objectSpecular) = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::PHONG_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                (texture->objectPhong) = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::METALLIC_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->metallicFlag = true;
                break;

            case Tokenizer::IOR_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                (texture->objectIndexOfRefraction) =
                    PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::REFRACTION_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                (texture->objectRefraction) = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::TRANSMIT_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                (texture->objectTransmit) = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::REFLECTION_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                (texture->objectReflection) = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::IMAGEMAP_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->textureNumber = (int)SolidTextureColorTextures::IMAGEMAP_TEXTURE;
                texture->image = new TextureImage;
                if (texture->image == nullptr) {
                    ParseErrorReporter::reportError(
                        "Out of memory. Cannot allocate imagemap texture", ctx);
                }
                texture->image->setImageGradient(Vector3Dd(1.0, -1.0, 0.0));
                texture->image->setMapType((int)SolidTextureProjectionMethods::PLANAR_MAP);
                texture->image->setInterpolationType((int)SolidTextureBitmapInterpolationTypes::NO_INTERPOLATION);
                texture->image->setOnceFlag(false);
                texture->image->setUseColorFlag(true);

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
                            texture->image->setMapType(
                                (int)PrimitiveParser::parseFloat(ctx));
                            break;

                        case Tokenizer::LEFT_ANGLE_TOKEN:
                            ctx.tokenStream().ungetToken();
                            {
                                Vector3Dd _g;
                                PrimitiveParser::parseVector(&_g, ctx);
                                texture->image->setImageGradient(_g);
                            }
                            break;

                        case Tokenizer::IFF_TOKEN: {
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            IndexedColorImageHDRUncompressed *idx = IffFormat::readIffImage(
                                texture->image, ctx.token().Token_String);
                            if (idx != nullptr) {
                                wireIndexedInToTextureImage(
                                    texture->image, idx);
                            }
                            Exit_Flag = true;
                            break;
                        }

                        case Tokenizer::GIF_TOKEN: {
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            IndexedColorImageHDRUncompressed *idx = new IndexedColorImageHDRUncompressed;
                            GifFormat::readGifImage(idx, ctx.token().Token_String);
                            wireIndexedInToTextureImage(texture->image, idx);
                            Exit_Flag = true;
                            break;
                        }

                        case Tokenizer::TGA_TOKEN:
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            TargaFormat::readTargaImage(
                                texture->image, ctx.token().Token_String);
                            Exit_Flag = true;
                            break;

                        case Tokenizer::DUMP_TOKEN:
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            RawDumpFormat::readDumpImage(
                                texture->image, ctx.token().Token_String);
                            Exit_Flag = true;
                            break;

                        default:
                            ParseErrorReporter::parseError(Tokenizer::GIF_TOKEN, ctx);
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
                        case Tokenizer::ONCE_TOKEN:
                            texture->image->setOnceFlag(true);
                            break;

                        case Tokenizer::INTERPOLATE_TOKEN:
                            texture->image->setInterpolationType(
                                (int)PrimitiveParser::parseFloat(ctx));
                            break;

                        case Tokenizer::MAPTYPE_TOKEN:
                            texture->image->setMapType(
                                (int)PrimitiveParser::parseFloat(ctx));
                            break;

                        case Tokenizer::USE_COLOUR_TOKEN:
                            texture->image->setUseColorFlag(true);
                            break;

                        case Tokenizer::USE_INDEX_TOKEN:
                            texture->image->setUseColorFlag(false);
                            break;

                        case Tokenizer::ALPHA_TOKEN: {
                            bool Exit_Flag;
                            Exit_Flag = false;
                            while (!Exit_Flag) {
                                ctx.tokenStream().getToken();
                                switch (ctx.token().tokenId) {
                                case Tokenizer::FLOAT_TOKEN:
                                    reg = (int)(ctx.token().tokenFloat + 0.01);
                                    if (texture->image->getIndexedData() == nullptr) {
                                        ParseErrorReporter::reportError(
                                            "Can't apply ALPHA to a non "
                                            "color-mapped image\n", ctx);
                                    }

                                    if ((reg < 0) ||
                                        (reg >=
                                            texture->image->getIndexedData()->getColorMapSize())) {
                                        ParseErrorReporter::reportError(
                                            "ALPHA color register value out "
                                            "of range.\n", ctx);
                                    }

                                    texture->image->getIndexedData()->getColorTable()[reg].a =
                                        (unsigned short)(255.0 *
                                                         PrimitiveParser::
                                                             parseFloat(ctx));
                                    Exit_Flag = true;
                                    break;

                                case Tokenizer::ALL_TOKEN: {
                                    double alpha;
                                    alpha = PrimitiveParser::parseFloat(ctx);

                                    for (reg = 0;
                                        reg < texture->image->getIndexedData()->getColorMapSize();
                                        reg++) {
                                        texture->image->getIndexedData()->getColorTable()[reg].a = (unsigned short)(alpha * 255.0);
                                    }
                                    Exit_Flag = true;
                                }

                                break;
                                }
                            }
                        } break;

                        case Tokenizer::RIGHT_CURLY_TOKEN:
                            Exit_Flag = true;
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
                    texture->constantFlag = false;
                }
                texture->bumpNumber = (int)SolidTextureBumpyTextures::WAVES;
                texture->bumpAmount = PrimitiveParser::parseFloat(ctx);
                {
                    bool Exit_Flag;
                    Exit_Flag = false;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().tokenId) {
                        case Tokenizer::PHASE_TOKEN:
                            texture->phase = PrimitiveParser::parseFloat(ctx);
                            Exit_Flag = true;
                            break;

                        default:
                            ctx.tokenStream().ungetToken();
                            Exit_Flag = true;
                            break;
                        }
                    }
                }
                break;

            case Tokenizer::FREQUENCY_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->frequency = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::PHASE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->phase = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::RIPPLES_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->bumpNumber = (int)SolidTextureBumpyTextures::RIPPLES;
                texture->bumpAmount = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::WRINKLES_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->bumpNumber = (int)SolidTextureBumpyTextures::WRINKLES;
                texture->bumpAmount = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::BUMPS_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->bumpNumber = (int)SolidTextureBumpyTextures::BUMPS;
                texture->bumpAmount = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::DENTS_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->bumpNumber = (int)SolidTextureBumpyTextures::DENTS;
                texture->bumpAmount = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::TRANSLATE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                PrimitiveParser::parseVector(&localVector, ctx);
                TextureUtils::instance().translateTexture(&texture, &localVector);
                break;

            case Tokenizer::ROTATE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                PrimitiveParser::parseVector(&localVector, ctx);
                TextureUtils::instance().rotateTexture(&texture, &localVector);
                break;

            case Tokenizer::SCALE_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                PrimitiveParser::parseVector(&localVector, ctx);
                TextureUtils::instance().scaleTexture(&texture, &localVector);
                break;

            case Tokenizer::COLOUR_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->color1 = ModelBuilder::getColor();
                PrimitiveParser::parseColor(texture->color1, ctx);
                texture->textureNumber = (int)SolidTextureColorTextures::COLOUR_TEXTURE;
                break;

            case Tokenizer::COLOUR_MAP_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->colorMap = ColorMapParser::parseColorMap(ctx);
                break;

            case Tokenizer::ONION_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->textureNumber = (int)SolidTextureColorTextures::ONION_TEXTURE;
                break;

            case Tokenizer::LEOPARD_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->textureNumber = (int)SolidTextureColorTextures::LEOPARD_TEXTURE;
                break;

            /* New Material Parsing - Cdw */
            case Tokenizer::PAINTED1_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->textureNumber = (int)SolidTextureColorTextures::PAINTED1_TEXTURE;
                break;

            case Tokenizer::PAINTED2_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->textureNumber = (int)SolidTextureColorTextures::PAINTED2_TEXTURE;
                break;

            case Tokenizer::PAINTED3_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->textureNumber = (int)SolidTextureColorTextures::PAINTED3_TEXTURE;
                break;

            case Tokenizer::BUMPY1_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->bumpNumber = (int)SolidTextureBumpyTextures::BUMPY1;
                texture->bumpAmount = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::BUMPY2_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->bumpNumber = (int)SolidTextureBumpyTextures::BUMPY2;
                texture->bumpAmount = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::BUMPY3_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->bumpNumber = (int)SolidTextureBumpyTextures::BUMPY3;
                texture->bumpAmount = PrimitiveParser::parseFloat(ctx);
                break;

            case Tokenizer::BUMPMAP_TOKEN:
                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->bumpNumber = (int)SolidTextureBumpyTextures::BUMPMAP;
                texture->bumpImage = new TextureImage;
                if (texture->bumpImage == nullptr) {
                    ParseErrorReporter::reportError(
                        "Out of memory. Cannot allocate bumpmap texture", ctx);
                }
                texture->bumpImage->setImageGradient(Vector3Dd(1.0, -1.0, 0.0));
                texture->bumpImage->setMapType((int)SolidTextureProjectionMethods::PLANAR_MAP);
                texture->bumpImage->setInterpolationType((int)SolidTextureBitmapInterpolationTypes::NO_INTERPOLATION);
                texture->bumpImage->setOnceFlag(false);
                texture->bumpImage->setUseColorFlag(true);

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
                            texture->bumpImage->setMapType(
                                (int)PrimitiveParser::parseFloat(ctx));
                            break;

                        case Tokenizer::LEFT_ANGLE_TOKEN:
                            ctx.tokenStream().ungetToken();
                            {
                                Vector3Dd _g;
                                PrimitiveParser::parseVector(&_g, ctx);
                                texture->bumpImage->setImageGradient(_g);
                            }
                            break;

                        case Tokenizer::IFF_TOKEN: {
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            IndexedColorImageHDRUncompressed *idx = IffFormat::readIffImage(
                                texture->bumpImage, ctx.token().Token_String);
                            if (idx != nullptr) {
                                wireIndexedInToTextureImage(
                                    texture->bumpImage, idx);
                            }
                            Exit_Flag = true;
                            break;
                        }

                        case Tokenizer::GIF_TOKEN: {
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            IndexedColorImageHDRUncompressed *idx = new IndexedColorImageHDRUncompressed;
                            GifFormat::readGifImage(idx, ctx.token().Token_String);
                            wireIndexedInToTextureImage(
                                texture->bumpImage, idx);
                            Exit_Flag = true;
                            break;
                        }

                        case Tokenizer::TGA_TOKEN:
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            TargaFormat::readTargaImage(
                                texture->bumpImage, ctx.token().Token_String);
                            Exit_Flag = true;
                            break;

                        case Tokenizer::DUMP_TOKEN:
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            RawDumpFormat::readDumpImage(
                                texture->bumpImage, ctx.token().Token_String);
                            Exit_Flag = true;
                            break;

                        default:
                            ParseErrorReporter::parseError(Tokenizer::GIF_TOKEN, ctx);
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
                        case Tokenizer::ONCE_TOKEN:
                            texture->bumpImage->setOnceFlag(true);
                            break;

                        case Tokenizer::MAPTYPE_TOKEN:
                            texture->bumpImage->setMapType(
                                (int)PrimitiveParser::parseFloat(ctx));
                            break;

                        case Tokenizer::INTERPOLATE_TOKEN:
                            texture->bumpImage->setInterpolationType(
                                (int)PrimitiveParser::parseFloat(ctx));
                            break;

                        case Tokenizer::BUMPSIZE_TOKEN:
                            texture->bumpAmount =
                                PrimitiveParser::parseFloat(ctx);
                            break;

                        case Tokenizer::USE_COLOUR_TOKEN:
                            texture->bumpImage->setUseColorFlag(true);
                            break;
                        case Tokenizer::USE_INDEX_TOKEN:
                            texture->bumpImage->setUseColorFlag(false);
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
                break;

            case Tokenizer::MATERIAL_MAP_TOKEN:

                if (texture->constantFlag) {
                    texture = TextureParser::copyTexture(texture);
                    texture->constantFlag = false;
                }
                texture->textureNumber = (int)SolidTextureColorTextures::MATERIAL_MAP_TEXTURE;
                texture->materialImage = new TextureImage;
                if (texture->materialImage == nullptr) {
                    ParseErrorReporter::reportError(
                        "Out of memory. Cannot allocate material map texture", ctx);
                }
                *&texture->textureGradient = Vector3Dd(1.0, -1.0, 0.0);
                texture->materialImage->setMapType((int)SolidTextureProjectionMethods::PLANAR_MAP);
                texture->materialImage->setInterpolationType((int)SolidTextureBitmapInterpolationTypes::NO_INTERPOLATION);
                texture->materialImage->setOnceFlag(false);
                texture->materialImage->setUseColorFlag(false);

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
                            texture->image->setMapType(
                                (int)PrimitiveParser::parseFloat(ctx));
                            break;

                        case Tokenizer::LEFT_ANGLE_TOKEN:
                            ctx.tokenStream().ungetToken();
                            {
                                Vector3Dd _g;
                                PrimitiveParser::parseVector(&_g, ctx);
                                texture->materialImage->setImageGradient(_g);
                            }
                            break;

                        case Tokenizer::IFF_TOKEN: {
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            IndexedColorImageHDRUncompressed *idx = IffFormat::readIffImage(
                                texture->materialImage, ctx.token().Token_String);
                            if (idx != nullptr) {
                                wireIndexedInToTextureImage(
                                    texture->materialImage, idx);
                            }
                            Exit_Flag = true;
                            break;
                        }

                        case Tokenizer::GIF_TOKEN: {
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            IndexedColorImageHDRUncompressed *idx = new IndexedColorImageHDRUncompressed;
                            GifFormat::readGifImage(idx, ctx.token().Token_String);
                            wireIndexedInToTextureImage(
                                texture->materialImage, idx);
                            Exit_Flag = true;
                            break;
                        }

                        case Tokenizer::TGA_TOKEN:
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            TargaFormat::readTargaImage(texture->materialImage,
                                ctx.token().Token_String);
                            Exit_Flag = true;
                            break;

                        case Tokenizer::DUMP_TOKEN:
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            RawDumpFormat::readDumpImage(texture->materialImage,
                                ctx.token().Token_String);
                            Exit_Flag = true;
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
                    bool Exit_Flag;
                    Exit_Flag = false;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().tokenId) {

                        case Tokenizer::MAPTYPE_TOKEN:
                            texture->materialImage->setMapType(
                                (int)PrimitiveParser::parseFloat(ctx));
                            break;

                        case Tokenizer::INTERPOLATE_TOKEN:
                            texture->materialImage->setInterpolationType(
                                (int)PrimitiveParser::parseFloat(ctx));
                            break;

                        case Tokenizer::ONCE_TOKEN:
                            texture->materialImage->setOnceFlag(true);
                            break;

                        case Tokenizer::TEXTURE_TOKEN: {
                            Material *newMat = TextureParser::parseTexture(ctx);
                            firstTexture->materials.add(newMat);
                            texture = newMat;
                        } break;

                        case Tokenizer::RIGHT_CURLY_TOKEN: {
                            texture = firstTexture;
                            Exit_Flag = true;
                        } break;

                        default:
                            ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                            break;
                        }
                    }
                }
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
    logTextureStateLegacy("legacy", texture);
    return (texture);
}
