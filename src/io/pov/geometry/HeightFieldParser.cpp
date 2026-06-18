#include "java/util/PriorityQueue.txx"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/IndexedColorImageHDRUncompressed.h"
#include "vsdk/toolkit/media/RGBAImageHDRUncompressed.h"

#include "environment/geometry/volume/HeightField.h"
#include "environment/scene/ModelBuilder.h"
#include "environment/geometry/SimpleBody.h"

#include "io/image/GifFormat.h"
#include "io/image/TargaFormat.h"
#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/geometry/HeightFieldParser.h"
#include "io/pov/material/TextureParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"

SimpleBody *
HeightFieldParser::parseHeightField()
{
    ParserContext ctx;
    return HeightFieldParser::parseHeightField(ctx);
}

SimpleBody *
HeightFieldParser::parseHeightField(ParserContext &ctx)
{
    (void)ctx;
    HeightField *localShape;
    SimpleBody *body = nullptr;
    int constantId;
    Vector3Dd localVector;
    PovrayMaterial *localTexture;
    IndexedColorImageHDRUncompressed *indexedImage = nullptr;
    RGBAImageHDRUncompressed *directImage = nullptr;
    int imageType = 0;

    localShape = nullptr;

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (
                ctx.token().getTokenId()) { // This should be modified to include
                                        // other image types - CdW
            case Tokenizer::GIF_TOKEN:
                imageType = HeightField::GIF;
                localShape = ModelBuilder::getHeightFieldShape();
                body = ModelBuilder::wrap(localShape);
                indexedImage = new IndexedColorImageHDRUncompressed;
                if (indexedImage == nullptr) {
                    ParseErrorReporter::reportError("Out of memory. Cannot allocate "
                                              "space for Height Field (1st "
                                              "message, ctx).");
                }
                ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                GifFormat::readGifImage(indexedImage, ctx.token().tokenString);
                localShape->getBoundingBox()->getBounds()[0] = Vector3Dd(1.0, 0.0, 1.0);
                localShape->getBoundingBox()->getBounds()[1] =
                    Vector3Dd(indexedImage->getXSize() - 2.0, 256.0, indexedImage->getYSize() - 2.0);
                localVector = Vector3Dd(1.0 / indexedImage->getXSize(),
                    1.0 / 256.0, 1.0 / indexedImage->getYSize());
                *localShape->getTransformation() = Matrix4x4d().scale(
                    localVector.x(), localVector.y(), localVector.z());
                *localShape->getTransformationInverse() = Matrix4x4d().scale(
                    1.0 / localVector.x(), 1.0 / localVector.y(), 1.0 / localVector.z());
                Exit_Flag = true;
                break;

            case Tokenizer::POT_TOKEN:
                imageType = HeightField::POT;
                localShape = ModelBuilder::getHeightFieldShape();
                body = ModelBuilder::wrap(localShape);
                indexedImage = new IndexedColorImageHDRUncompressed;
                if (indexedImage == nullptr) {
                    ParseErrorReporter::reportError("Out of memory. Cannot allocate "
                                              "space for Height Field (1st "
                                              "message, ctx).");
                }
                ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                GifFormat::readGifImage(indexedImage, ctx.token().tokenString);
                localShape->getBoundingBox()->getBounds()[0] = Vector3Dd(1.0, 0.0, 1.0);
                localShape->getBoundingBox()->getBounds()[1] = Vector3Dd(
                    indexedImage->getXSize() / 2.0 - 2.0, 256.0, indexedImage->getYSize() - 2.0);
                localVector = Vector3Dd(2.0 / indexedImage->getXSize(),
                    1.0 / 256.0, 1.0 / indexedImage->getYSize());
                *localShape->getTransformation() = Matrix4x4d().scale(
                    localVector.x(), localVector.y(), localVector.z());
                *localShape->getTransformationInverse() = Matrix4x4d().scale(
                    1.0 / localVector.x(), 1.0 / localVector.y(), 1.0 / localVector.z());
                Exit_Flag = true;
                break;

            case Tokenizer::TGA_TOKEN:
                imageType = HeightField::TGA;
                localShape = ModelBuilder::getHeightFieldShape();
                body = ModelBuilder::wrap(localShape);
                directImage = new RGBAImageHDRUncompressed;
                if (directImage == nullptr) {
                    ParseErrorReporter::reportError("Cannot allocate space for "
                                              "Height Field (1st message, ctx).");
                }
                ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                TargaFormat::readTargaImage(directImage, ctx.token().tokenString);
                localShape->getBoundingBox()->getBounds()[0] = Vector3Dd(1.0, 0.0, 1.0);
                localShape->getBoundingBox()->getBounds()[1] =
                    Vector3Dd(directImage->getXSize() - 2.0, 256.0, directImage->getYSize() - 2.0);
                localVector = Vector3Dd(1.0 / directImage->getXSize(),
                    1.0 / 256.0, 1.0 / directImage->getYSize());
                *localShape->getTransformation() = Matrix4x4d().scale(
                    localVector.x(), localVector.y(), localVector.z());
                *localShape->getTransformationInverse() = Matrix4x4d().scale(
                    1.0 / localVector.x(), 1.0 / localVector.y(), 1.0 / localVector.z());
                Exit_Flag = true;
                break;

            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].getConstantType() ==
                        ParseGlobals::HEIGHT_FIELD_CONSTANT) {
                        body = (SimpleBody *)((TransformableElement *)ctx.constants()[(int)constantId]
                                .getConstantData())->copy();
                        localShape = (HeightField *)body->getGeometry();
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::reportUndeclared(ctx);
                }
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
            switch (ctx.token().getTokenId()) {
            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = true;
                break;

            case Tokenizer::WATER_LEVEL_TOKEN:
                localShape->getBoundingBox()->getBounds()[0] =
                    localShape->getBoundingBox()->getBounds()[0].withY(
                        PrimitiveParser::parseFloat(ctx));
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

            case Tokenizer::TEXTURE_TOKEN:
                localTexture = TextureParser::parseTexture(ctx);
                if (localTexture->isConstant()) {
                    localTexture = TextureParser::copyTexture(localTexture);
                }

                TextureParser::prependTextureLayers(localTexture, body->getMaterialRef());
                break;

            case Tokenizer::COLOUR_TOKEN:
                body->setShapeColor(ModelBuilder::getColor());
                PrimitiveParser::parseColor(body->getShapeColor(), ctx);
                break;

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
    return body;
}
