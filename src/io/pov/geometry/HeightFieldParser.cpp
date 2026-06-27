#include "java/util/PriorityQueue.txx"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/IndexedColorImageHDRUncompressed.h"
#include "vsdk/toolkit/media/RGBAImageHDRUncompressed.h"

#include "environment/geometry/volume/HeightField.h"
#include "io/pov/geometry/SceneBuilder.h"
#include "io/pov/geometry/SimpleBodyBuilder.h"

#include "io/image/GifFormat.h"
#include "io/image/TargaFormat.h"
#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/geometry/HeightFieldParser.h"
#include "environment/material/povray/PovRayMaterialConstancy.h"
#include "io/pov/material/TextureParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"

SimpleBodyBuilder *
HeightFieldParser::parseHeightField()
{
    ParserContext ctx;
    return HeightFieldParser::parseHeightField(ctx);
}

SimpleBodyBuilder *
HeightFieldParser::parseHeightField(ParserContext &ctx)
{
    (void)ctx;
    HeightField *localShape = nullptr;
    SimpleBodyBuilder *body = nullptr;
    Vector3Dd localVector;
    IndexedColorImageHDRUncompressed *indexedImage = nullptr;
    RGBAImageHDRUncompressed *directImage = nullptr;
    int imageType = 0;

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        bool Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (
                ctx.token().getTokenId()) { // This should be modified to include
                                        // other image types
            case Tokenizer::GIF_TOKEN:
            {
                imageType = HeightField::GIF;
                indexedImage = new IndexedColorImageHDRUncompressed;
                if (indexedImage == nullptr) {
                    ParseErrorReporter::reportError("Out of memory. Cannot allocate "
                                              "space for Height Field (1st "
                                              "message, ctx).");
                }
                ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                GifFormat::readGifImage(indexedImage, ctx.token().getTokenString(), *ctx.tokenizer().getFileLocator());
                Vector3Dd minBounds = Vector3Dd(1.0, 0.0, 1.0);
                Vector3Dd maxBounds = Vector3Dd(
                    indexedImage->getXSize() - 2.0, 256.0,
                    indexedImage->getYSize() - 2.0);
                localVector = Vector3Dd(1.0 / indexedImage->getXSize(),
                    1.0 / 256.0, 1.0 / indexedImage->getYSize());
                Matrix4x4d localTransformation = Matrix4x4d().scale(
                    localVector.x(), localVector.y(), localVector.z());
                Matrix4x4d localTransformationInverse = Matrix4x4d().scale(
                    1.0 / localVector.x(), 1.0 / localVector.y(), 1.0 / localVector.z());
                localShape = new HeightField(localTransformation,
                    localTransformationInverse, minBounds, maxBounds);
                body = SceneBuilder::wrap(localShape);
                Exit_Flag = true;
                break;
            }

            case Tokenizer::POT_TOKEN:
            {
                imageType = HeightField::POT;
                indexedImage = new IndexedColorImageHDRUncompressed;
                if (indexedImage == nullptr) {
                    ParseErrorReporter::reportError("Out of memory. Cannot allocate "
                                              "space for Height Field (1st "
                                              "message, ctx).");
                }
                ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                GifFormat::readGifImage(indexedImage, ctx.token().getTokenString(), *ctx.tokenizer().getFileLocator());
                Vector3Dd minBounds = Vector3Dd(1.0, 0.0, 1.0);
                Vector3Dd maxBounds = Vector3Dd(
                    indexedImage->getXSize() / 2.0 - 2.0, 256.0, indexedImage->getYSize() - 2.0);
                localVector = Vector3Dd(2.0 / indexedImage->getXSize(),
                    1.0 / 256.0, 1.0 / indexedImage->getYSize());
                Matrix4x4d localTransformation = Matrix4x4d().scale(
                    localVector.x(), localVector.y(), localVector.z());
                Matrix4x4d localTransformationInverse = Matrix4x4d().scale(
                    1.0 / localVector.x(), 1.0 / localVector.y(), 1.0 / localVector.z());
                localShape = new HeightField(localTransformation,
                    localTransformationInverse, minBounds, maxBounds);
                body = SceneBuilder::wrap(localShape);
                Exit_Flag = true;
                break;
            }

            case Tokenizer::TGA_TOKEN:
            {
                imageType = HeightField::TGA;
                directImage = new RGBAImageHDRUncompressed;
                if (directImage == nullptr) {
                    ParseErrorReporter::reportError("Cannot allocate space for "
                                              "Height Field (1st message, ctx).");
                }
                ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                TargaFormat::readTargaImage(directImage, ctx.token().getTokenString(), *ctx.tokenizer().getFileLocator());
                Vector3Dd minBounds = Vector3Dd(1.0, 0.0, 1.0);
                Vector3Dd maxBounds = Vector3Dd(
                    directImage->getXSize() - 2.0, 256.0,
                    directImage->getYSize() - 2.0);
                localVector = Vector3Dd(1.0 / directImage->getXSize(),
                    1.0 / 256.0, 1.0 / directImage->getYSize());
                Matrix4x4d localTransformation = Matrix4x4d().scale(
                    localVector.x(), localVector.y(), localVector.z());
                Matrix4x4d localTransformationInverse = Matrix4x4d().scale(
                    1.0 / localVector.x(), 1.0 / localVector.y(), 1.0 / localVector.z());
                localShape = new HeightField(localTransformation,
                    localTransformationInverse, minBounds, maxBounds);
                body = SceneBuilder::wrap(localShape);
                Exit_Flag = true;
                break;
            }

            case Tokenizer::IDENTIFIER_TOKEN:
            {
                int constantId;
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].getConstantType() ==
                        ParseGlobals::HEIGHT_FIELD_CONSTANT) {
                        body = new SimpleBodyBuilder(
                                *(SimpleBodyBuilder *)ctx.constants()[(int)constantId]
                                    .getConstantData());
                        localShape = (HeightField *)body->getGeometry();
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
                ParseErrorReporter::parseError(Tokenizer::GIF_TOKEN, ctx);
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

            case Tokenizer::WATER_LEVEL_TOKEN:
            {
                localShape->getBoundingBox()->getBounds()[0] =
                    localShape->getBoundingBox()->getBounds()[0].withY(
                        PrimitiveParser::parseFloat(ctx));
                break;
            }

            case Tokenizer::TRANSLATE_TOKEN:
            {
                PrimitiveParser::parseVector(&localVector, ctx);
                body->translate(&localVector);
                break;
            }

            case Tokenizer::ROTATE_TOKEN:
            {
                PrimitiveParser::parseVector(&localVector, ctx);
                body->rotate(&localVector);
                break;
            }

            case Tokenizer::SCALE_TOKEN:
            {
                PrimitiveParser::parseVector(&localVector, ctx);
                body->scale(&localVector);
                break;
            }

            case Tokenizer::INVERSE_TOKEN:
            {
                body->invert();
                break;
            }

            case Tokenizer::TEXTURE_TOKEN:
            {
                PovRayMaterial *localTexture = TextureParser::parseTexture(ctx);
                if (PovRayMaterialConstancy::isConstant(localTexture)) {
                    localTexture = TextureParser::copyTexture(localTexture);
                }

                body->prependMaterialLayers(localTexture);
                break;
            }

            case Tokenizer::COLOUR_TOKEN:
            {
                PrimitiveParser::parseColor(body->ensureShapeColor(), ctx);
                break;
            }

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    if (indexedImage != nullptr) {
        HeightField::findHfMinMax(localShape, indexedImage, imageType);
    } else if (directImage != nullptr) {
        HeightField::findHfMinMax(localShape, directImage, imageType);
    }
    // indexedImage/directImage are scratch buffers: their pixel data is read
    // into localShape's own block/Map arrays by allocateHfBlocks()/
    // findHfMinMax() above, never aliased or referenced afterwards.
    delete indexedImage;
    delete directImage;
    return body;
}
