#include "io/pov/material/TextureParser.h"
#include "io/pov/material/PovRayMaterialBuilder.h"
#include "environment/material/PovRayMaterialUtils.h"
#include "environment/material/SolidTextureBumpyNames.h"
#include "environment/material/SolidTextureColorNames.h"
#include "environment/material/ValuesBuilder.h"
#include "io/image/GifFormat.h"
#include "io/image/IffFormat.h"
#include "io/image/RawDumpFormat.h"
#include "io/image/TargaFormat.h"
#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/material/ColorMapParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/media/IndexedColorImageHDRUncompressed.h"
#include "vsdk/toolkit/media/solidTexture/from2d/ControlledRGBAImageHDRUncompressed.h"
#include "vsdk/toolkit/media/solidTexture/from2d/ImageToSolidTextureInterpolationTypes.h"
#include "vsdk/toolkit/media/solidTexture/from2d/ImageToSolidTextureProjectionMethods.h"

void
TextureParser::TextureParser::wireIndexedInToTextureImage(ControlledRGBAImageHDRUncompressed *ti, IndexedColorImageHDRUncompressed *idx)
{
    ti->setIndexedData(idx);
    ti->allocate(idx->getXSize(), idx->getYSize());
}

PovRayMaterial *
TextureParser::copyTexture(PovRayMaterial *texture)
{
    return PovRayMaterialUtils::copyTexture(texture);
}

PovRayMaterialBuilder
TextureParser::editorFor(PovRayMaterial *texture)
{
    return PovRayMaterialBuilder(texture).setConstant(false);
}

PovRayMaterial *
TextureParser::parseTexture()
{
    ParserContext ctx;
    return TextureParser::parseTexture(ctx);
}

PovRayMaterial *
TextureParser::parseTexture(ParserContext &ctx)
{
    return TextureParser::parseTexture(
        ctx.getDefaultTexture(), ctx);
}

PovRayMaterial *
TextureParser::parseTexture(PovRayMaterial *baseTexture, ParserContext &ctx)
{
    (void)ctx;
    Vector3Dd localVector;
    int constantId;
    PovRayMaterial *texture;
    PovRayMaterial *localTexture;
    int reg;

    texture = baseTexture;

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        bool innerExitFlag = false;
        while (!innerExitFlag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].getConstantType() ==
                        ParseGlobals::TEXTURE_CONSTANT) {
                        texture = ((PovRayMaterial *)ctx.constants()[(int)constantId]
                                .getConstantData());
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::reportUndeclared(ctx);
                }
                break;

            case Tokenizer::FLOAT_TOKEN:
                ctx.tokenStream().ungetToken();
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setTextureRandomness(PrimitiveParser::parseFloat(ctx));
                    texture = b.build();
                }
                break;

            case Tokenizer::ONCE_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setOnceFlag(true);
                    texture = b.build();
                }
                break;

            case Tokenizer::TURBULENCE_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setTurbulence(PrimitiveParser::parseFloat(ctx));
                    texture = b.build();
                }
                break;

            case Tokenizer::OCTAVES_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setOctaves((int)PrimitiveParser::parseFloat(ctx));
                    if (b.getOctaves() < 1) {
                        b.setOctaves(6);
                    }
                    if (b.getOctaves() > 10) { // Avoid DOMAIN errors
                        b.setOctaves(10);
                    }
                    texture = b.build();
                }
                break;

            case Tokenizer::BOZO_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setTextureNumber(SolidTextureColorNames::BOZO_TEXTURE);
                    texture = b.build();
                }
                break;

            case Tokenizer::MORTAR_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setMortar(PrimitiveParser::parseFloat(ctx));
                    if (b.getMortar() < 0) {
                        b.setMortar(0.2);
                    }
                    texture = b.build();
                }
                break;

            case Tokenizer::BRICK_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setTextureNumber(SolidTextureColorNames::BRICK_TEXTURE);
                    {
                        bool localExitFlag;
                        localExitFlag = false;
                        while (!localExitFlag) {
                            ctx.tokenStream().getToken();
                            switch (ctx.token().getTokenId()) {
                            case Tokenizer::COLOUR_TOKEN:
                                b.setColor1(ValuesBuilder::getColor());
                                b.setColor2(ValuesBuilder::getColor());
                                PrimitiveParser::parseColor(b.getColor1(), ctx);
                                ParseHelpers::getExpectedToken(Tokenizer::COLOUR_TOKEN, ctx);
                                PrimitiveParser::parseColor(b.getColor2(), ctx);
                                break;

                            default:
                                ctx.tokenStream().ungetToken();
                                localExitFlag = true;
                                break;
                            }
                        }
                    }
                    texture = b.build();
                }
                break;

            case Tokenizer::CHECKER_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setTextureNumber(SolidTextureColorNames::CHECKER_TEXTURE);
                    {
                        bool localExitFlag;
                        localExitFlag = false;
                        while (!localExitFlag) {
                            ctx.tokenStream().getToken();
                            switch (ctx.token().getTokenId()) {
                            case Tokenizer::COLOUR_TOKEN:
                                b.setColor1(ValuesBuilder::getColor());
                                b.setColor2(ValuesBuilder::getColor());
                                PrimitiveParser::parseColor(b.getColor1(), ctx);
                                ParseHelpers::getExpectedToken(Tokenizer::COLOUR_TOKEN, ctx);
                                PrimitiveParser::parseColor(b.getColor2(), ctx);
                                break;

                            default:
                                ctx.tokenStream().ungetToken();
                                localExitFlag = true;
                                break;
                            }
                        }
                    }
                    texture = b.build();
                }
                break;

            case Tokenizer::CHECKER_TEXTURE_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setTextureNumber(SolidTextureColorNames::CHECKER_TEXTURE_TEXTURE);

                    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

                    {
                        bool localExitFlag;
                        localExitFlag = false;
                        while (!localExitFlag) {
                            ctx.tokenStream().getToken();
                            switch (ctx.token().getTokenId()) {
                            case Tokenizer::TEXTURE_TOKEN:
                                localTexture = TextureParser::parseTexture(ctx);
                                {
                                    PovRayMaterialBuilder lb = TextureParser::editorFor(localTexture);
                                    localTexture = lb.build();
                                    PovRayMaterial *color1Head = (PovRayMaterial *)b.getColor1();
                                    PovRayMaterialUtils::prependTextureLayers(localTexture, color1Head);
                                    b.setColor1((ColorRgba *)color1Head);
                                }
                                break;
                            default:
                                ctx.tokenStream().ungetToken();
                                localExitFlag = true;
                                break;
                            }
                        }
                    }

                    ParseHelpers::getExpectedToken(Tokenizer::TILE2_TOKEN, ctx);
                    {
                        bool localExitFlag;
                        localExitFlag = false;
                        while (!localExitFlag) {
                            ctx.tokenStream().getToken();
                            switch (ctx.token().getTokenId()) {
                            case Tokenizer::TEXTURE_TOKEN:
                                localTexture = TextureParser::parseTexture(ctx);
                                {
                                    PovRayMaterialBuilder lb = TextureParser::editorFor(localTexture);
                                    localTexture = lb.build();
                                    PovRayMaterial *color2Head = (PovRayMaterial *)b.getColor2();
                                    PovRayMaterialUtils::prependTextureLayers(localTexture, color2Head);
                                    b.setColor2((ColorRgba *)color2Head);
                                }
                                break;
                            default:
                                ctx.tokenStream().ungetToken();
                                localExitFlag = true;
                                break;
                            }
                        }
                    }
                    ParseHelpers::getExpectedToken(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                    texture = b.build();
                }
                break;

            case Tokenizer::MARBLE_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setTextureNumber(SolidTextureColorNames::MARBLE_TEXTURE);
                    texture = b.build();
                }
                break;

            case Tokenizer::WOOD_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setTextureNumber(SolidTextureColorNames::WOOD_TEXTURE);
                    texture = b.build();
                }
                break;

            case Tokenizer::SPOTTED_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setTextureNumber(SolidTextureColorNames::SPOTTED_TEXTURE);
                    texture = b.build();
                }
                break;

            case Tokenizer::AGATE_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setTextureNumber(SolidTextureColorNames::AGATE_TEXTURE);
                    texture = b.build();
                }
                break;

            case Tokenizer::GRANITE_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setTextureNumber(SolidTextureColorNames::GRANITE_TEXTURE);
                    texture = b.build();
                }
                break;

            case Tokenizer::GRADIENT_TOKEN:
                {
                    Vector3Dd gradientVec;
                    PrimitiveParser::parseVector(&gradientVec, ctx);
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setTextureNumber(SolidTextureColorNames::GRADIENT_TEXTURE);
                    b.setTextureGradient(gradientVec);
                    texture = b.build();
                }
                break;

            case Tokenizer::AMBIENT_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setObjectAmbient(PrimitiveParser::parseFloat(ctx));
                    texture = b.build();
                }
                break;

            case Tokenizer::BRILLIANCE_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setObjectBrilliance(PrimitiveParser::parseFloat(ctx));
                    texture = b.build();
                }
                break;

            case Tokenizer::ROUGHNESS_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setObjectRoughness(PrimitiveParser::parseFloat(ctx));
                    // No training wheels
                    // if (texture -> objectRoughness > 1.0)
                    //     texture -> objectRoughness = 1.0;
                    // if (texture -> objectRoughness < 0.001)
                    //     texture -> objectRoughness = 0.001;
                    texture = b.build();
                }
                break;

            case Tokenizer::PHONGSIZE_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setObjectPhongSize(PrimitiveParser::parseFloat(ctx));
                    // No training wheels
                    // if (texture -> objectPhongSize < 1.0)
                    //     texture -> objectPhongSize = 1.0;
                    // if (texture -> objectPhongSize > 100)
                    //     texture -> objectPhongSize = 100;
                    texture = b.build();
                }
                break;

            case Tokenizer::DIFFUSE_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setObjectDiffuse(PrimitiveParser::parseFloat(ctx));
                    texture = b.build();
                }
                break;

            case Tokenizer::SPECULAR_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setObjectSpecular(PrimitiveParser::parseFloat(ctx));
                    texture = b.build();
                }
                break;

            case Tokenizer::PHONG_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setObjectPhong(PrimitiveParser::parseFloat(ctx));
                    texture = b.build();
                }
                break;

            case Tokenizer::METALLIC_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setMetallicFlag(true);
                    texture = b.build();
                }
                break;

            case Tokenizer::IOR_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setObjectIndexOfRefraction(
                        PrimitiveParser::parseFloat(ctx));
                    texture = b.build();
                }
                break;

            case Tokenizer::REFRACTION_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setObjectRefraction(PrimitiveParser::parseFloat(ctx));
                    texture = b.build();
                }
                break;

            case Tokenizer::TRANSMIT_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setObjectTransmit(PrimitiveParser::parseFloat(ctx));
                    texture = b.build();
                }
                break;

            case Tokenizer::REFLECTION_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setObjectReflection(PrimitiveParser::parseFloat(ctx));
                    texture = b.build();
                }
                break;

            case Tokenizer::IMAGEMAP_TOKEN:
                {
                    ControlledRGBAImageHDRUncompressed *image = new ControlledRGBAImageHDRUncompressed;
                    if (image == nullptr) {
                        ParseErrorReporter::reportError(
                            "Out of memory. Cannot allocate image map texture", ctx);
                    }
                    image->setImageGradient(Vector3Dd(1.0, -1.0, 0.0));
                    image->setMapType(ImageToSolidTextureProjectionMethods::PLANAR_MAP);
                    image->setInterpolationType(ImageToSolidTextureInterpolationTypes::NO_INTERPOLATION);
                    image->setOnceFlag(false);
                    image->setUseColorFlag(true);

                    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

                    {
                        bool localExitFlag;
                        localExitFlag = false;
                        while (!localExitFlag) {
                            ctx.tokenStream().getToken();
                            switch (ctx.token().getTokenId()) {
                            case Tokenizer::DASH_TOKEN:
                            case Tokenizer::PLUS_TOKEN:
                            case Tokenizer::FLOAT_TOKEN:
                                ctx.tokenStream().ungetToken();
                                image->setMapType(PrimitiveParser::parseFloat(ctx));
                                break;

                            case Tokenizer::LEFT_ANGLE_TOKEN:
                                ctx.tokenStream().ungetToken();
                                {
                                    Vector3Dd _g;
                                    PrimitiveParser::parseVector(&_g, ctx);
                                    image->setImageGradient(_g);
                                }
                                break;

                            case Tokenizer::IFF_TOKEN: {
                                ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                                IndexedColorImageHDRUncompressed * const idx = IffFormat::readIffImage(
                                    image, ctx.token().getTokenString(),
                                    *ctx.tokenizer().getFileLocator());
                                if (idx != nullptr) {
                                    TextureParser::wireIndexedInToTextureImage(
                                        image, idx);
                                }
                                localExitFlag = true;
                                break;
                            }

                            case Tokenizer::GIF_TOKEN: {
                                ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                                IndexedColorImageHDRUncompressed * const idx = new IndexedColorImageHDRUncompressed;
                                GifFormat::readGifImage(idx, ctx.token().getTokenString(), *ctx.tokenizer().getFileLocator());
                                TextureParser::wireIndexedInToTextureImage(image, idx);
                                localExitFlag = true;
                                break;
                            }

                            case Tokenizer::TGA_TOKEN:
                                ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                                TargaFormat::readTargaImage(
                                    image, ctx.token().getTokenString(),
                                    *ctx.tokenizer().getFileLocator());
                                localExitFlag = true;
                                break;

                            case Tokenizer::DUMP_TOKEN:
                                ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                                RawDumpFormat::readDumpImage(
                                    image, ctx.token().getTokenString(),
                                    *ctx.tokenizer().getFileLocator());
                                localExitFlag = true;
                                break;

                            default:
                                ParseErrorReporter::parseError(Tokenizer::GIF_TOKEN, ctx);
                                break;
                            }
                        }
                    }

                    {
                        bool localExitFlag;
                        localExitFlag = false;
                        while (!localExitFlag) {
                            ctx.tokenStream().getToken();
                            switch (ctx.token().getTokenId()) {
                            case Tokenizer::ONCE_TOKEN:
                                image->setOnceFlag(true);
                                break;

                            case Tokenizer::INTERPOLATE_TOKEN:
                                image->setInterpolationType(
                                    PrimitiveParser::parseFloat(ctx));
                                break;

                            case Tokenizer::MAPTYPE_TOKEN:
                                image->setMapType(
                                    (int)PrimitiveParser::parseFloat(ctx));
                                break;

                            case Tokenizer::USE_COLOUR_TOKEN:
                                image->setUseColorFlag(true);
                                break;

                            case Tokenizer::USE_INDEX_TOKEN:
                                image->setUseColorFlag(false);
                                break;

                            case Tokenizer::ALPHA_TOKEN: {
                                bool alphaInnerExitFlag = false;
                                while (!alphaInnerExitFlag) {
                                    ctx.tokenStream().getToken();
                                    switch (ctx.token().getTokenId()) {
                                    case Tokenizer::FLOAT_TOKEN:
                                        reg = (int)(ctx.token().getTokenFloat() + 0.01);
                                        if (image->getIndexedData() == nullptr) {
                                            ParseErrorReporter::reportError(
                                                "Can't apply ALPHA to a non "
                                                "color-mapped image\n", ctx);
                                        }

                                        if ((reg < 0) ||
                                            (reg >=
                                                image->getIndexedData()->getColorMapSize())) {
                                            ParseErrorReporter::reportError(
                                                "ALPHA color register value out "
                                                "of range.\n", ctx);
                                        }

                                        image->getIndexedData()->getColorTable()[reg].a =
                                            (unsigned short)(255.0 *
                                                             PrimitiveParser::
                                                                 parseFloat(ctx));
                                        alphaInnerExitFlag = true;
                                        break;

                                    case Tokenizer::ALL_TOKEN: {
                                        const double alpha = PrimitiveParser::parseFloat(ctx);

                                        for (reg = 0;
                                            reg < image->getIndexedData()->getColorMapSize();
                                            reg++) {
                                            image->getIndexedData()->getColorTable()[reg].a = (unsigned short)(alpha * 255.0);
                                        }
                                        alphaInnerExitFlag = true;
                                    }

                                    break;
                                    }
                                }
                            } break;

                            case Tokenizer::RIGHT_CURLY_TOKEN:
                                localExitFlag = true;
                                break;

                            default:
                                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                                break;
                            }
                        }
                    }

                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setTextureNumber(SolidTextureColorNames::IMAGE_MAP_TEXTURE);
                    b.setImage(image);
                    texture = b.build();
                }
                break;

            case Tokenizer::WAVES_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setBumpNumber(SolidTextureBumpyNames::WAVES);
                    b.setBumpAmount(PrimitiveParser::parseFloat(ctx));
                    {
                        bool localExitFlag = false;
                        while (!localExitFlag) {
                            ctx.tokenStream().getToken();
                            switch (ctx.token().getTokenId()) {
                            case Tokenizer::PHASE_TOKEN:
                                b.setPhase(PrimitiveParser::parseFloat(ctx));
                                localExitFlag = true;
                                break;

                            default:
                                ctx.tokenStream().ungetToken();
                                localExitFlag = true;
                                break;
                            }
                        }
                    }
                    texture = b.build();
                }
                break;

            case Tokenizer::FREQUENCY_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setFrequency(PrimitiveParser::parseFloat(ctx));
                    texture = b.build();
                }
                break;

            case Tokenizer::PHASE_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setPhase(PrimitiveParser::parseFloat(ctx));
                    texture = b.build();
                }
                break;

            case Tokenizer::RIPPLES_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setBumpNumber(SolidTextureBumpyNames::RIPPLES);
                    b.setBumpAmount(PrimitiveParser::parseFloat(ctx));
                    texture = b.build();
                }
                break;

            case Tokenizer::WRINKLES_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setBumpNumber(SolidTextureBumpyNames::WRINKLES);
                    b.setBumpAmount(PrimitiveParser::parseFloat(ctx));
                    texture = b.build();
                }
                break;

            case Tokenizer::BUMPS_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setBumpNumber(SolidTextureBumpyNames::BUMPS);
                    b.setBumpAmount(PrimitiveParser::parseFloat(ctx));
                    texture = b.build();
                }
                break;

            case Tokenizer::DENTS_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setBumpNumber(SolidTextureBumpyNames::DENTS);
                    b.setBumpAmount(PrimitiveParser::parseFloat(ctx));
                    texture = b.build();
                }
                break;

            case Tokenizer::TRANSLATE_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    texture = b.build();
                    PrimitiveParser::parseVector(&localVector, ctx);
                    PovRayMaterialUtils::translateTexture(&texture, &localVector);
                }
                break;

            case Tokenizer::ROTATE_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    texture = b.build();
                    PrimitiveParser::parseVector(&localVector, ctx);
                    PovRayMaterialUtils::rotateTexture(&texture, &localVector);
                }
                break;

            case Tokenizer::SCALE_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    texture = b.build();
                    PrimitiveParser::parseVector(&localVector, ctx);
                    PovRayMaterialUtils::scaleTexture(&texture, &localVector);
                }
                break;

            case Tokenizer::COLOUR_TOKEN:
                {
                    ColorRgba *c = ValuesBuilder::getColor();
                    PrimitiveParser::parseColor(c, ctx);
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setColor1(c);
                    b.setTextureNumber(SolidTextureColorNames::COLOUR_TEXTURE);
                    texture = b.build();
                }
                break;

            case Tokenizer::COLOUR_MAP_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setColorMap(ColorMapParser::parseColorMap(ctx));
                    texture = b.build();
                }
                break;

            case Tokenizer::ONION_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setTextureNumber(SolidTextureColorNames::ONION_TEXTURE);
                    texture = b.build();
                }
                break;

            case Tokenizer::LEOPARD_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setTextureNumber(SolidTextureColorNames::LEOPARD_TEXTURE);
                    texture = b.build();
                }
                break;

            case Tokenizer::BUMPY1_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setBumpNumber(SolidTextureBumpyNames::BUMPY1);
                    b.setBumpAmount(PrimitiveParser::parseFloat(ctx));
                    texture = b.build();
                }
                break;

            case Tokenizer::BUMPY2_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setBumpNumber(SolidTextureBumpyNames::BUMPY2);
                    b.setBumpAmount(PrimitiveParser::parseFloat(ctx));
                    texture = b.build();
                }
                break;

            case Tokenizer::BUMPY3_TOKEN:
                {
                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setBumpNumber(SolidTextureBumpyNames::BUMPY3);
                    b.setBumpAmount(PrimitiveParser::parseFloat(ctx));
                    texture = b.build();
                }
                break;

            case Tokenizer::BUMPMAP_TOKEN:
                {
                    ControlledRGBAImageHDRUncompressed *bumpImage = new ControlledRGBAImageHDRUncompressed;
                    if (bumpImage == nullptr) {
                        ParseErrorReporter::reportError(
                            "Out of memory. Cannot allocate bump map texture", ctx);
                    }
                    bumpImage->setImageGradient(Vector3Dd(1.0, -1.0, 0.0));
                    bumpImage->setMapType(ImageToSolidTextureProjectionMethods::PLANAR_MAP);
                    bumpImage->setInterpolationType((int)ImageToSolidTextureInterpolationTypes::NO_INTERPOLATION);
                    bumpImage->setOnceFlag(false);
                    bumpImage->setUseColorFlag(true);

                    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

                    {
                        bool localExitFlag;
                        localExitFlag = false;
                        while (!localExitFlag) {
                            ctx.tokenStream().getToken();
                            switch (ctx.token().getTokenId()) {
                            case Tokenizer::DASH_TOKEN:
                            case Tokenizer::PLUS_TOKEN:
                            case Tokenizer::FLOAT_TOKEN:
                                ctx.tokenStream().ungetToken();
                                bumpImage->setMapType(
                                    (int)PrimitiveParser::parseFloat(ctx));
                                break;

                            case Tokenizer::LEFT_ANGLE_TOKEN:
                                ctx.tokenStream().ungetToken();
                                {
                                    Vector3Dd _g;
                                    PrimitiveParser::parseVector(&_g, ctx);
                                    bumpImage->setImageGradient(_g);
                                }
                                break;

                            case Tokenizer::IFF_TOKEN: {
                                ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                                IndexedColorImageHDRUncompressed * const idx = IffFormat::readIffImage(
                                    bumpImage, ctx.token().getTokenString(),
                                    *ctx.tokenizer().getFileLocator());
                                if (idx != nullptr) {
                                    TextureParser::wireIndexedInToTextureImage(
                                        bumpImage, idx);
                                }
                                localExitFlag = true;
                                break;
                            }

                            case Tokenizer::GIF_TOKEN: {
                                ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                                IndexedColorImageHDRUncompressed * const idx = new IndexedColorImageHDRUncompressed;
                                GifFormat::readGifImage(idx, ctx.token().getTokenString(), *ctx.tokenizer().getFileLocator());
                                TextureParser::wireIndexedInToTextureImage(
                                    bumpImage, idx);
                                localExitFlag = true;
                                break;
                            }

                            case Tokenizer::TGA_TOKEN:
                                ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                                TargaFormat::readTargaImage(
                                    bumpImage, ctx.token().getTokenString(),
                                    *ctx.tokenizer().getFileLocator());
                                localExitFlag = true;
                                break;

                            case Tokenizer::DUMP_TOKEN:
                                ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                                RawDumpFormat::readDumpImage(
                                    bumpImage, ctx.token().getTokenString(),
                                    *ctx.tokenizer().getFileLocator());
                                localExitFlag = true;
                                break;

                            default:
                                ParseErrorReporter::parseError(Tokenizer::GIF_TOKEN, ctx);
                                break;
                            }
                        }
                    }

                    double bumpAmount = 0.0;
                    {
                        bool localExitFlag;
                        localExitFlag = false;
                        while (!localExitFlag) {
                            ctx.tokenStream().getToken();
                            switch (ctx.token().getTokenId()) {
                            case Tokenizer::ONCE_TOKEN:
                                bumpImage->setOnceFlag(true);
                                break;

                            case Tokenizer::MAPTYPE_TOKEN:
                                bumpImage->setMapType(
                                    (int)PrimitiveParser::parseFloat(ctx));
                                break;

                            case Tokenizer::INTERPOLATE_TOKEN:
                                bumpImage->setInterpolationType(
                                    (int)PrimitiveParser::parseFloat(ctx));
                                break;

                            case Tokenizer::BUMPSIZE_TOKEN:
                                bumpAmount =
                                    PrimitiveParser::parseFloat(ctx);
                                break;

                            case Tokenizer::USE_COLOUR_TOKEN:
                                bumpImage->setUseColorFlag(true);
                                break;
                            case Tokenizer::USE_INDEX_TOKEN:
                                bumpImage->setUseColorFlag(false);
                                break;

                            case Tokenizer::RIGHT_CURLY_TOKEN:
                                localExitFlag = true;
                                break;
                            default:
                                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                                break;
                            }
                        }
                    }

                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setBumpNumber(SolidTextureBumpyNames::BUMP_MAP);
                    b.setBumpImage(bumpImage);
                    b.setBumpAmount(bumpAmount);
                    texture = b.build();
                }
                break;

            case Tokenizer::MATERIAL_MAP_TOKEN:
                {
                    ControlledRGBAImageHDRUncompressed *materialImage = new ControlledRGBAImageHDRUncompressed;
                    if (materialImage == nullptr) {
                        ParseErrorReporter::reportError(
                            "Out of memory. Cannot allocate material map texture", ctx);
                    }
                    materialImage->setImageGradient(Vector3Dd(1.0, -1.0, 0.0));
                    materialImage->setMapType(ImageToSolidTextureProjectionMethods::PLANAR_MAP);
                    materialImage->setInterpolationType((int)ImageToSolidTextureInterpolationTypes::NO_INTERPOLATION);
                    materialImage->setOnceFlag(false);
                    materialImage->setUseColorFlag(false);

                    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

                    {
                        bool localExitFlag;
                        localExitFlag = false;
                        while (!localExitFlag) {
                            ctx.tokenStream().getToken();
                            switch (ctx.token().getTokenId()) {
                            case Tokenizer::DASH_TOKEN:
                            case Tokenizer::PLUS_TOKEN:
                            case Tokenizer::FLOAT_TOKEN:
                                ctx.tokenStream().ungetToken();
                                materialImage->setMapType(
                                    (int)PrimitiveParser::parseFloat(ctx));
                                break;

                            case Tokenizer::LEFT_ANGLE_TOKEN:
                                ctx.tokenStream().ungetToken();
                                {
                                    Vector3Dd _g;
                                    PrimitiveParser::parseVector(&_g, ctx);
                                    materialImage->setImageGradient(_g);
                                }
                                break;

                            case Tokenizer::IFF_TOKEN: {
                                ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                                IndexedColorImageHDRUncompressed * const idx = IffFormat::readIffImage(
                                    materialImage, ctx.token().getTokenString(),
                                    *ctx.tokenizer().getFileLocator());
                                if (idx != nullptr) {
                                    TextureParser::wireIndexedInToTextureImage(
                                        materialImage, idx);
                                }
                                localExitFlag = true;
                                break;
                            }

                            case Tokenizer::GIF_TOKEN: {
                                ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                                IndexedColorImageHDRUncompressed * const idx = new IndexedColorImageHDRUncompressed;
                                GifFormat::readGifImage(idx, ctx.token().getTokenString(), *ctx.tokenizer().getFileLocator());
                                TextureParser::wireIndexedInToTextureImage(
                                    materialImage, idx);
                                localExitFlag = true;
                                break;
                            }

                            case Tokenizer::TGA_TOKEN:
                                ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                                TargaFormat::readTargaImage(materialImage,
                                    ctx.token().getTokenString(), *ctx.tokenizer().getFileLocator());
                                localExitFlag = true;
                                break;

                            case Tokenizer::DUMP_TOKEN:
                                ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                                RawDumpFormat::readDumpImage(materialImage,
                                    ctx.token().getTokenString(), *ctx.tokenizer().getFileLocator());
                                localExitFlag = true;
                                break;

                            default:
                                ParseErrorReporter::parseError(Tokenizer::GIF_TOKEN, ctx);
                                break;
                            }
                        }
                    }

                    PovRayMaterialBuilder b = TextureParser::editorFor(texture);
                    b.setTextureNumber(SolidTextureColorNames::MATERIAL_MAP_TEXTURE);
                    b.setMaterialImage(materialImage);
                    b.setTextureGradient(Vector3Dd(1.0, -1.0, 0.0));

                    {
                        bool localExitFlag;
                        localExitFlag = false;
                        while (!localExitFlag) {
                            ctx.tokenStream().getToken();
                            switch (ctx.token().getTokenId()) {

                            case Tokenizer::MAPTYPE_TOKEN:
                                b.getMaterialImage()->setMapType(
                                    (int)PrimitiveParser::parseFloat(ctx));
                                break;

                            case Tokenizer::INTERPOLATE_TOKEN:
                                b.getMaterialImage()->setInterpolationType(
                                    (int)PrimitiveParser::parseFloat(ctx));
                                break;

                            case Tokenizer::ONCE_TOKEN:
                                b.getMaterialImage()->setOnceFlag(true);
                                break;

                            case Tokenizer::TEXTURE_TOKEN: {
                                PovRayMaterial * const newMat = TextureParser::parseTexture(ctx);
                                b.materials().add(newMat);
                            } break;

                            case Tokenizer::RIGHT_CURLY_TOKEN: {
                                localExitFlag = true;
                            } break;

                            default:
                                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                                break;
                            }
                        }
                    }

                    texture = b.build();
                }
                break;

            case Tokenizer::RIGHT_CURLY_TOKEN:
                innerExitFlag = true;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }
    return texture;
}
