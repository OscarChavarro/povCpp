#include <cstdlib>

#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/media/IndexedColorImageHDRUncompressed.h"
#include "vsdk/toolkit/media/solidTexture/from2d/ControlledRGBAImageHDRUncompressed.h"
#include "vsdk/toolkit/media/solidTexture/from2d/ImageToSolidTextureInterpolationTypes.h"
#include "vsdk/toolkit/media/solidTexture/from2d/ImageToSolidTextureProjectionMethods.h"
#include "environment/camera/Camera.h"
#include "environment/geometry/element/Triangle.h"
#include "environment/geometry/volume/Blob.h"
#include "environment/material/MaterialUtils.h"
#include "environment/material/SolidTextureBumpyNames.h"
#include "environment/material/SolidTextureColorNames.h"
#include "environment/scene/ModelBuilder.h"
#include "io/image/GifFormat.h"
#include "io/image/IffFormat.h"
#include "io/image/RawDumpFormat.h"
#include "io/image/TargaFormat.h"
#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/material/ColorMapParser.h"
#include "io/pov/material/TextureParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"

void
TextureParser::TextureParser::wireIndexedInToTextureImage(ControlledRGBAImageHDRUncompressed *ti, IndexedColorImageHDRUncompressed *idx)
{
    ti->setIndexedData(idx);
    ti->allocate(idx->getXSize(), idx->getYSize());
}

bool
TextureParser::shouldLogTextureState()
{
    const char *flag = getenv("POVCPP_DIAG_TEXTURE_STATE");
    return flag != nullptr && flag[0] != '\0';
}

void
TextureParser::logTextureStateLegacy(const char *prefix, const PovrayMaterial *texture)
{
    if (!shouldLogTextureState() || texture == nullptr) {
        return;
    }

    {
        char _logMsg[1024];
        snprintf(_logMsg, sizeof(_logMsg), "[TEXTURE-STATE] %s type=%d ambient=%.6f diffuse=%.6f brilliance=%.6f reflection=%.6f turbulence=%.6f frequency=%.6f phase=%.6f octaves=%d bumpNumber=%d bumpAmount=%.6f texXform=%s\n", prefix,         texture->getTextureNumber(),         texture->getObjectAmbient(),         texture->getObjectDiffuse(),         texture->getObjectBrilliance(),         texture->getObjectReflection(),         texture->getTurbulence(),         texture->getFrequency(),         texture->getPhase(),         texture->getOctaves(),         texture->getBumpNumber(),         texture->getBumpAmount(),         texture->getTextureTransformation() != nullptr ? "yes" : "no");
        Logger::reportMessage("TextureParser", Logger::WARNING, "", _logMsg);
    }
    if (texture->getTextureTransformation() != nullptr) {
        {
            char _logMsg[1024];
            snprintf(_logMsg, sizeof(_logMsg), "[TEXTURE-STATE] %s xform row0=<%.6f,%.6f,%.6f,%.6f> row1=<%.6f,%.6f,%.6f,%.6f> row2=<%.6f,%.6f,%.6f,%.6f> row3=<%.6f,%.6f,%.6f,%.6f>\n", prefix,             texture->getTextureTransformation()->get(0, 0), texture->getTextureTransformation()->get(0, 1),             texture->getTextureTransformation()->get(0, 2), texture->getTextureTransformation()->get(0, 3),             texture->getTextureTransformation()->get(1, 0), texture->getTextureTransformation()->get(1, 1),             texture->getTextureTransformation()->get(1, 2), texture->getTextureTransformation()->get(1, 3),             texture->getTextureTransformation()->get(2, 0), texture->getTextureTransformation()->get(2, 1),             texture->getTextureTransformation()->get(2, 2), texture->getTextureTransformation()->get(2, 3),             texture->getTextureTransformation()->get(3, 0), texture->getTextureTransformation()->get(3, 1),             texture->getTextureTransformation()->get(3, 2), texture->getTextureTransformation()->get(3, 3));
            Logger::reportMessage("TextureParser", Logger::WARNING, "", _logMsg);
        }
    }
}



PovrayMaterial *
TextureParser::copyTexture(PovrayMaterial *texture)
{
    return MaterialUtils::instance().copyTexture(texture);
}

void
TextureParser::prependTextureLayers(PovrayMaterial *newHead, Material *&existingHead)
{
    PovrayMaterial *existingPovrayHead = static_cast<PovrayMaterial *>(existingHead);
    prependTextureLayers(newHead, existingPovrayHead);
    existingHead = existingPovrayHead;
}

void
TextureParser::prependTextureLayers(PovrayMaterial *newHead, PovrayMaterial *&existingHead)
{
    if (existingHead != nullptr) {
        newHead->getLayers().add(existingHead);
        for (long int i = 0; i < existingHead->getLayers().size(); i++) {
            newHead->getLayers().add(existingHead->getLayers()[i]);
        }
        existingHead->getLayers().clear();
    }
    existingHead = newHead;
}

PovrayMaterial *
TextureParser::parseTexture()
{
    ParserContext ctx;
    return TextureParser::parseTexture(ctx);
}

PovrayMaterial *
TextureParser::parseTexture(ParserContext &ctx)
{
    (void)ctx;
    Vector3Dd localVector;
    int constantId;
    PovrayMaterial *texture;
    PovrayMaterial *localTexture;
    PovrayMaterial *firstTexture;
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
                        texture = ((PovrayMaterial *)ctx.constants()[(int)constantId]
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
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setTextureRandomness(PrimitiveParser::parseFloat(ctx));
                break;

            case Tokenizer::ONCE_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setOnceFlag(true);
                break;

            case Tokenizer::TURBULENCE_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setTurbulence(PrimitiveParser::parseFloat(ctx));
                break;

            case Tokenizer::OCTAVES_TOKEN: // dmf 02/05 for turb
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setOctaves((int)PrimitiveParser::parseFloat(ctx));
                if (texture->getOctaves() < 1) {
                    texture->setOctaves(6);
                }
                if (texture->getOctaves() > 10) { // Avoid DOMAIN errors
                    texture->setOctaves(10);
                }
                break;

            case Tokenizer::BOZO_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setTextureNumber(SolidTextureColorNames::BOZO_TEXTURE);
                break;

            case Tokenizer::MORTAR_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setMortar(PrimitiveParser::parseFloat(ctx));
                if (texture->getMortar() < 0) {
                    texture->setMortar(0.2);
                }
                break;

            case Tokenizer::BRICK_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setTextureNumber(SolidTextureColorNames::BRICK_TEXTURE);
                {
                    bool Exit_Flag;
                    Exit_Flag = false;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().tokenId) {
                        case Tokenizer::COLOUR_TOKEN:
                            texture->setColor1(ModelBuilder::getColor());
                            texture->setColor2(ModelBuilder::getColor());
                            PrimitiveParser::parseColor(texture->getColor1(), ctx);
                            ParseHelpers::getExpectedToken(Tokenizer::COLOUR_TOKEN, ctx);
                            PrimitiveParser::parseColor(texture->getColor2(), ctx);
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
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setTextureNumber(SolidTextureColorNames::CHECKER_TEXTURE);
                {
                    bool Exit_Flag;
                    Exit_Flag = false;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().tokenId) {
                        case Tokenizer::COLOUR_TOKEN:
                            texture->setColor1(ModelBuilder::getColor());
                            texture->setColor2(ModelBuilder::getColor());
                            PrimitiveParser::parseColor(texture->getColor1(), ctx);
                            ParseHelpers::getExpectedToken(Tokenizer::COLOUR_TOKEN, ctx);
                            PrimitiveParser::parseColor(texture->getColor2(), ctx);
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
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setTextureNumber(SolidTextureColorNames::CHECKER_TEXTURE_TEXTURE);

                ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

                {
                    bool Exit_Flag;
                    Exit_Flag = false;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().tokenId) {
                        case Tokenizer::TEXTURE_TOKEN:
                            localTexture = TextureParser::parseTexture(ctx);
                            if (localTexture->isConstant()) {
                                localTexture =
                                    TextureParser::copyTexture(localTexture);
                            }
                            {
                                PovrayMaterial *color1Head = (PovrayMaterial *)texture->getColor1();
                                TextureParser::prependTextureLayers(localTexture, color1Head);
                                texture->setColor1((ColorRgba *)color1Head);
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
                            if (localTexture->isConstant()) {
                                localTexture =
                                    TextureParser::copyTexture(localTexture);
                            }
                            {
                                PovrayMaterial *color2Head = (PovrayMaterial *)texture->getColor2();
                                TextureParser::prependTextureLayers(localTexture, color2Head);
                                texture->setColor2((ColorRgba *)color2Head);
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
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setTextureNumber(SolidTextureColorNames::MARBLE_TEXTURE);
                break;

            case Tokenizer::WOOD_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setTextureNumber(SolidTextureColorNames::WOOD_TEXTURE);
                break;

            case Tokenizer::SPOTTED_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setTextureNumber(SolidTextureColorNames::SPOTTED_TEXTURE);
                break;

            case Tokenizer::AGATE_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setTextureNumber(SolidTextureColorNames::AGATE_TEXTURE);
                break;

            case Tokenizer::GRANITE_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setTextureNumber(SolidTextureColorNames::GRANITE_TEXTURE);
                break;

            case Tokenizer::GRADIENT_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setTextureNumber(SolidTextureColorNames::GRADIENT_TEXTURE);
                PrimitiveParser::parseVector(&texture->getTextureGradient(), ctx);
                break;

            case Tokenizer::AMBIENT_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setObjectAmbient(PrimitiveParser::parseFloat(ctx));
                break;

            case Tokenizer::BRILLIANCE_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setObjectBrilliance(PrimitiveParser::parseFloat(ctx));
                break;

            case Tokenizer::ROUGHNESS_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setObjectRoughness(PrimitiveParser::parseFloat(ctx));
                // No training wheels
                // if (texture -> objectRoughness > 1.0)
                //     texture -> objectRoughness = 1.0;
                // if (texture -> objectRoughness < 0.001)
                //     texture -> objectRoughness = 0.001;
                break;

            case Tokenizer::PHONGSIZE_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setObjectPhongSize(PrimitiveParser::parseFloat(ctx));
                // No training wheels
                // if (texture -> objectPhongSize < 1.0)
                //     texture -> objectPhongSize = 1.0;
                // if (texture -> objectPhongSize > 100)
                //     texture -> objectPhongSize = 100;
                break;

            case Tokenizer::DIFFUSE_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setObjectDiffuse(PrimitiveParser::parseFloat(ctx));
                break;

            case Tokenizer::SPECULAR_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setObjectSpecular(PrimitiveParser::parseFloat(ctx));
                break;

            case Tokenizer::PHONG_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setObjectPhong(PrimitiveParser::parseFloat(ctx));
                break;

            case Tokenizer::METALLIC_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setMetallicFlag(true);
                break;

            case Tokenizer::IOR_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setObjectIndexOfRefraction(
                    PrimitiveParser::parseFloat(ctx));
                break;

            case Tokenizer::REFRACTION_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setObjectRefraction(PrimitiveParser::parseFloat(ctx));
                break;

            case Tokenizer::TRANSMIT_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setObjectTransmit(PrimitiveParser::parseFloat(ctx));
                break;

            case Tokenizer::REFLECTION_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setObjectReflection(PrimitiveParser::parseFloat(ctx));
                break;

            case Tokenizer::IMAGEMAP_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setTextureNumber(SolidTextureColorNames::IMAGE_MAP_TEXTURE);
                texture->setImage(new ControlledRGBAImageHDRUncompressed);
                if (texture->getImage() == nullptr) {
                    ParseErrorReporter::reportError(
                        "Out of memory. Cannot allocate imagemap texture", ctx);
                }
                texture->getImage()->setImageGradient(Vector3Dd(1.0, -1.0, 0.0));
                texture->getImage()->setMapType(ImageToSolidTextureProjectionMethods::PLANAR_MAP);
                texture->getImage()->setInterpolationType(ImageToSolidTextureInterpolationTypes::NO_INTERPOLATION);
                texture->getImage()->setOnceFlag(false);
                texture->getImage()->setUseColorFlag(true);

                ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

                {
                    bool exitFlag;
                    exitFlag = false;
                    while (!exitFlag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().tokenId) {
                        case Tokenizer::DASH_TOKEN:
                        case Tokenizer::PLUS_TOKEN:
                        case Tokenizer::FLOAT_TOKEN:
                            ctx.tokenStream().ungetToken();
                            texture->getImage()->setMapType(PrimitiveParser::parseFloat(ctx));
                            break;

                        case Tokenizer::LEFT_ANGLE_TOKEN:
                            ctx.tokenStream().ungetToken();
                            {
                                Vector3Dd _g;
                                PrimitiveParser::parseVector(&_g, ctx);
                                texture->getImage()->setImageGradient(_g);
                            }
                            break;

                        case Tokenizer::IFF_TOKEN: {
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            IndexedColorImageHDRUncompressed * const idx = IffFormat::readIffImage(
                                texture->getImage(), ctx.token().Token_String);
                            if (idx != nullptr) {
                                TextureParser::wireIndexedInToTextureImage(
                                    texture->getImage(), idx);
                            }
                            exitFlag = true;
                            break;
                        }

                        case Tokenizer::GIF_TOKEN: {
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            IndexedColorImageHDRUncompressed * const idx = new IndexedColorImageHDRUncompressed;
                            GifFormat::readGifImage(idx, ctx.token().Token_String);
                            TextureParser::wireIndexedInToTextureImage(texture->getImage(), idx);
                            exitFlag = true;
                            break;
                        }

                        case Tokenizer::TGA_TOKEN:
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            TargaFormat::readTargaImage(
                                texture->getImage(), ctx.token().Token_String);
                            exitFlag = true;
                            break;

                        case Tokenizer::DUMP_TOKEN:
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            RawDumpFormat::readDumpImage(
                                texture->getImage(), ctx.token().Token_String);
                            exitFlag = true;
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
                            texture->getImage()->setOnceFlag(true);
                            break;

                        case Tokenizer::INTERPOLATE_TOKEN:
                            texture->getImage()->setInterpolationType(
                                PrimitiveParser::parseFloat(ctx));
                            break;

                        case Tokenizer::MAPTYPE_TOKEN:
                            texture->getImage()->setMapType(
                                (int)PrimitiveParser::parseFloat(ctx));
                            break;

                        case Tokenizer::USE_COLOUR_TOKEN:
                            texture->getImage()->setUseColorFlag(true);
                            break;

                        case Tokenizer::USE_INDEX_TOKEN:
                            texture->getImage()->setUseColorFlag(false);
                            break;

                        case Tokenizer::ALPHA_TOKEN: {
                            bool Exit_Flag;
                            Exit_Flag = false;
                            while (!Exit_Flag) {
                                ctx.tokenStream().getToken();
                                switch (ctx.token().tokenId) {
                                case Tokenizer::FLOAT_TOKEN:
                                    reg = (int)(ctx.token().tokenFloat + 0.01);
                                    if (texture->getImage()->getIndexedData() == nullptr) {
                                        ParseErrorReporter::reportError(
                                            "Can't apply ALPHA to a non "
                                            "color-mapped image\n", ctx);
                                    }

                                    if ((reg < 0) ||
                                        (reg >=
                                            texture->getImage()->getIndexedData()->getColorMapSize())) {
                                        ParseErrorReporter::reportError(
                                            "ALPHA color register value out "
                                            "of range.\n", ctx);
                                    }

                                    texture->getImage()->getIndexedData()->getColorTable()[reg].a =
                                        (unsigned short)(255.0 *
                                                         PrimitiveParser::
                                                             parseFloat(ctx));
                                    Exit_Flag = true;
                                    break;

                                case Tokenizer::ALL_TOKEN: {
                                    const double alpha = PrimitiveParser::parseFloat(ctx);

                                    for (reg = 0;
                                        reg < texture->getImage()->getIndexedData()->getColorMapSize();
                                        reg++) {
                                        texture->getImage()->getIndexedData()->getColorTable()[reg].a = (unsigned short)(alpha * 255.0);
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
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setBumpNumber(SolidTextureBumpyNames::WAVES);
                texture->setBumpAmount(PrimitiveParser::parseFloat(ctx));
                {
                    bool exitFlag = false;
                    while (!exitFlag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().tokenId) {
                        case Tokenizer::PHASE_TOKEN:
                            texture->setPhase(PrimitiveParser::parseFloat(ctx));
                            exitFlag = true;
                            break;

                        default:
                            ctx.tokenStream().ungetToken();
                            exitFlag = true;
                            break;
                        }
                    }
                }
                break;

            case Tokenizer::FREQUENCY_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setFrequency(PrimitiveParser::parseFloat(ctx));
                break;

            case Tokenizer::PHASE_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setPhase(PrimitiveParser::parseFloat(ctx));
                break;

            case Tokenizer::RIPPLES_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setBumpNumber(SolidTextureBumpyNames::RIPPLES);
                texture->setBumpAmount(PrimitiveParser::parseFloat(ctx));
                break;

            case Tokenizer::WRINKLES_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setBumpNumber(SolidTextureBumpyNames::WRINKLES);
                texture->setBumpAmount(PrimitiveParser::parseFloat(ctx));
                break;

            case Tokenizer::BUMPS_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setBumpNumber(SolidTextureBumpyNames::BUMPS);
                texture->setBumpAmount(PrimitiveParser::parseFloat(ctx));
                break;

            case Tokenizer::DENTS_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setBumpNumber(SolidTextureBumpyNames::DENTS);
                texture->setBumpAmount(PrimitiveParser::parseFloat(ctx));
                break;

            case Tokenizer::TRANSLATE_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                PrimitiveParser::parseVector(&localVector, ctx);
                MaterialUtils::instance().translateTexture(&texture, &localVector);
                break;

            case Tokenizer::ROTATE_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                PrimitiveParser::parseVector(&localVector, ctx);
                MaterialUtils::instance().rotateTexture(&texture, &localVector);
                break;

            case Tokenizer::SCALE_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                PrimitiveParser::parseVector(&localVector, ctx);
                MaterialUtils::instance().scaleTexture(&texture, &localVector);
                break;

            case Tokenizer::COLOUR_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setColor1(ModelBuilder::getColor());
                PrimitiveParser::parseColor(texture->getColor1(), ctx);
                texture->setTextureNumber(SolidTextureColorNames::COLOUR_TEXTURE);
                break;

            case Tokenizer::COLOUR_MAP_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setColorMap(ColorMapParser::parseColorMap(ctx));
                break;

            case Tokenizer::ONION_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setTextureNumber(SolidTextureColorNames::ONION_TEXTURE);
                break;

            case Tokenizer::LEOPARD_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setTextureNumber(SolidTextureColorNames::LEOPARD_TEXTURE);
                break;

            case Tokenizer::BUMPY1_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setBumpNumber(SolidTextureBumpyNames::BUMPY1);
                texture->setBumpAmount(PrimitiveParser::parseFloat(ctx));
                break;

            case Tokenizer::BUMPY2_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setBumpNumber(SolidTextureBumpyNames::BUMPY2);
                texture->setBumpAmount(PrimitiveParser::parseFloat(ctx));
                break;

            case Tokenizer::BUMPY3_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setBumpNumber(SolidTextureBumpyNames::BUMPY3);
                texture->setBumpAmount(PrimitiveParser::parseFloat(ctx));
                break;

            case Tokenizer::BUMPMAP_TOKEN:
                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setBumpNumber(SolidTextureBumpyNames::BUMP_MAP);
                texture->setBumpImage(new ControlledRGBAImageHDRUncompressed);
                if (texture->getBumpImage() == nullptr) {
                    ParseErrorReporter::reportError(
                        "Out of memory. Cannot allocate bumpmap texture", ctx);
                }
                texture->getBumpImage()->setImageGradient(Vector3Dd(1.0, -1.0, 0.0));
                texture->getBumpImage()->setMapType(ImageToSolidTextureProjectionMethods::PLANAR_MAP);
                texture->getBumpImage()->setInterpolationType((int)ImageToSolidTextureInterpolationTypes::NO_INTERPOLATION);
                texture->getBumpImage()->setOnceFlag(false);
                texture->getBumpImage()->setUseColorFlag(true);

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
                            texture->getBumpImage()->setMapType(
                                (int)PrimitiveParser::parseFloat(ctx));
                            break;

                        case Tokenizer::LEFT_ANGLE_TOKEN:
                            ctx.tokenStream().ungetToken();
                            {
                                Vector3Dd _g;
                                PrimitiveParser::parseVector(&_g, ctx);
                                texture->getBumpImage()->setImageGradient(_g);
                            }
                            break;

                        case Tokenizer::IFF_TOKEN: {
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            IndexedColorImageHDRUncompressed * const idx = IffFormat::readIffImage(
                                texture->getBumpImage(), ctx.token().Token_String);
                            if (idx != nullptr) {
                                TextureParser::wireIndexedInToTextureImage(
                                    texture->getBumpImage(), idx);
                            }
                            Exit_Flag = true;
                            break;
                        }

                        case Tokenizer::GIF_TOKEN: {
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            IndexedColorImageHDRUncompressed * const idx = new IndexedColorImageHDRUncompressed;
                            GifFormat::readGifImage(idx, ctx.token().Token_String);
                            TextureParser::wireIndexedInToTextureImage(
                                texture->getBumpImage(), idx);
                            Exit_Flag = true;
                            break;
                        }

                        case Tokenizer::TGA_TOKEN:
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            TargaFormat::readTargaImage(
                                texture->getBumpImage(), ctx.token().Token_String);
                            Exit_Flag = true;
                            break;

                        case Tokenizer::DUMP_TOKEN:
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            RawDumpFormat::readDumpImage(
                                texture->getBumpImage(), ctx.token().Token_String);
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
                            texture->getBumpImage()->setOnceFlag(true);
                            break;

                        case Tokenizer::MAPTYPE_TOKEN:
                            texture->getBumpImage()->setMapType(
                                (int)PrimitiveParser::parseFloat(ctx));
                            break;

                        case Tokenizer::INTERPOLATE_TOKEN:
                            texture->getBumpImage()->setInterpolationType(
                                (int)PrimitiveParser::parseFloat(ctx));
                            break;

                        case Tokenizer::BUMPSIZE_TOKEN:
                            texture->setBumpAmount(
                                PrimitiveParser::parseFloat(ctx));
                            break;

                        case Tokenizer::USE_COLOUR_TOKEN:
                            texture->getBumpImage()->setUseColorFlag(true);
                            break;
                        case Tokenizer::USE_INDEX_TOKEN:
                            texture->getBumpImage()->setUseColorFlag(false);
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

                if (texture->isConstant()) {
                    texture = TextureParser::copyTexture(texture);
                    texture->setConstant(false);
                }
                texture->setTextureNumber(SolidTextureColorNames::MATERIAL_MAP_TEXTURE);
                texture->setMaterialImage(new ControlledRGBAImageHDRUncompressed);
                if (texture->getMaterialImage() == nullptr) {
                    ParseErrorReporter::reportError(
                        "Out of memory. Cannot allocate material map texture", ctx);
                }
                texture->getTextureGradient() = Vector3Dd(1.0, -1.0, 0.0);
                texture->getMaterialImage()->setMapType(ImageToSolidTextureProjectionMethods::PLANAR_MAP);
                texture->getMaterialImage()->setInterpolationType((int)ImageToSolidTextureInterpolationTypes::NO_INTERPOLATION);
                texture->getMaterialImage()->setOnceFlag(false);
                texture->getMaterialImage()->setUseColorFlag(false);

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
                            texture->getMaterialImage()->setMapType(
                                (int)PrimitiveParser::parseFloat(ctx));
                            break;

                        case Tokenizer::LEFT_ANGLE_TOKEN:
                            ctx.tokenStream().ungetToken();
                            {
                                Vector3Dd _g;
                                PrimitiveParser::parseVector(&_g, ctx);
                                texture->getMaterialImage()->setImageGradient(_g);
                            }
                            break;

                        case Tokenizer::IFF_TOKEN: {
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            IndexedColorImageHDRUncompressed * const idx = IffFormat::readIffImage(
                                texture->getMaterialImage(), ctx.token().Token_String);
                            if (idx != nullptr) {
                                TextureParser::wireIndexedInToTextureImage(
                                    texture->getMaterialImage(), idx);
                            }
                            Exit_Flag = true;
                            break;
                        }

                        case Tokenizer::GIF_TOKEN: {
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            IndexedColorImageHDRUncompressed * const idx = new IndexedColorImageHDRUncompressed;
                            GifFormat::readGifImage(idx, ctx.token().Token_String);
                            TextureParser::wireIndexedInToTextureImage(
                                texture->getMaterialImage(), idx);
                            Exit_Flag = true;
                            break;
                        }

                        case Tokenizer::TGA_TOKEN:
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            TargaFormat::readTargaImage(texture->getMaterialImage(),
                                ctx.token().Token_String);
                            Exit_Flag = true;
                            break;

                        case Tokenizer::DUMP_TOKEN:
                            ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                            RawDumpFormat::readDumpImage(texture->getMaterialImage(),
                                ctx.token().Token_String);
                            Exit_Flag = true;
                            break;

                        default:
                            ParseErrorReporter::parseError(Tokenizer::GIF_TOKEN, ctx);
                            break;
                        }
                    }
                }

                // Remember where the First_Texture is
                firstTexture = texture;

                {
                    bool Exit_Flag;
                    Exit_Flag = false;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().tokenId) {

                        case Tokenizer::MAPTYPE_TOKEN:
                            texture->getMaterialImage()->setMapType(
                                (int)PrimitiveParser::parseFloat(ctx));
                            break;

                        case Tokenizer::INTERPOLATE_TOKEN:
                            texture->getMaterialImage()->setInterpolationType(
                                (int)PrimitiveParser::parseFloat(ctx));
                            break;

                        case Tokenizer::ONCE_TOKEN:
                            texture->getMaterialImage()->setOnceFlag(true);
                            break;

                        case Tokenizer::TEXTURE_TOKEN: {
                            PovrayMaterial * const newMat = TextureParser::parseTexture(ctx);
                            firstTexture->getMaterials().add(newMat);
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
