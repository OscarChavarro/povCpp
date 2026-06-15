#include "io/pov/geometry/HeightFieldParser.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/volume/HeightField.h"
#include "environment/scene/ModelBuilder.h"
#include "io/image/GifFormat.h"
#include "io/image/TargaFormat.h"
#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/material/TextureParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/IndexedColorImageHDRUncompressed.h"
#include "vsdk/toolkit/media/RGBAImageHDRUncompressed.h"

Geometry *
HeightFieldParser::parseHeightField()
{
    ParserContext ctx;
    return HeightFieldParser::parseHeightField(ctx);
}

Geometry *
HeightFieldParser::parseHeightField(ParserContext &ctx)
{
    (void)ctx;
    HeightField *localShape;
    int constantId;
    Vector3Dd localVector;
    Material *localTexture;
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
                ctx.token().tokenId) { // This should be modified to include
                                        // other image types - CdW
            case Tokenizer::GIF_TOKEN:
                imageType = HeightField::GIF;
                localShape = ModelBuilder::getHeightFieldShape();
                indexedImage = new IndexedColorImageHDRUncompressed;
                if (indexedImage == nullptr) {
                    ParseErrorReporter::reportError("Out of memory. Cannot allocate "
                                              "space for Height Field (1st "
                                              "message, ctx).");
                }
                ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                GifFormat::readGifImage(indexedImage, ctx.token().Token_String);
                localShape->boundingBox->bounds[0] = Vector3Dd(1.0, 0.0, 1.0);
                localShape->boundingBox->bounds[1] =
                    Vector3Dd(indexedImage->getXSize() - 2.0, 256.0, indexedImage->getYSize() - 2.0);
                localVector = Vector3Dd(1.0 / indexedImage->getXSize(),
                    1.0 / 256.0, 1.0 / indexedImage->getYSize());
                *localShape->transformation = Matrix4x4d().scale(
                    localVector.x(), localVector.y(), localVector.z());
                *localShape->transformationInverse = Matrix4x4d().scale(
                    1.0 / localVector.x(), 1.0 / localVector.y(), 1.0 / localVector.z());
                Exit_Flag = true;
                break;

            case Tokenizer::POT_TOKEN:
                imageType = HeightField::POT;
                localShape = ModelBuilder::getHeightFieldShape();
                indexedImage = new IndexedColorImageHDRUncompressed;
                if (indexedImage == nullptr) {
                    ParseErrorReporter::reportError("Out of memory. Cannot allocate "
                                              "space for Height Field (1st "
                                              "message, ctx).");
                }
                ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                GifFormat::readGifImage(indexedImage, ctx.token().Token_String);
                localShape->boundingBox->bounds[0] = Vector3Dd(1.0, 0.0, 1.0);
                localShape->boundingBox->bounds[1] = Vector3Dd(
                    indexedImage->getXSize() / 2.0 - 2.0, 256.0, indexedImage->getYSize() - 2.0);
                localVector = Vector3Dd(2.0 / indexedImage->getXSize(),
                    1.0 / 256.0, 1.0 / indexedImage->getYSize());
                *localShape->transformation = Matrix4x4d().scale(
                    localVector.x(), localVector.y(), localVector.z());
                *localShape->transformationInverse = Matrix4x4d().scale(
                    1.0 / localVector.x(), 1.0 / localVector.y(), 1.0 / localVector.z());
                Exit_Flag = true;
                break;

            case Tokenizer::TGA_TOKEN:
                imageType = HeightField::TGA;
                localShape = ModelBuilder::getHeightFieldShape();
                directImage = new RGBAImageHDRUncompressed;
                if (directImage == nullptr) {
                    ParseErrorReporter::reportError("Cannot allocate space for "
                                              "Height Field (1st message, ctx).");
                }
                ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                TargaFormat::readTargaImage(directImage, ctx.token().Token_String);
                localShape->boundingBox->bounds[0] = Vector3Dd(1.0, 0.0, 1.0);
                localShape->boundingBox->bounds[1] =
                    Vector3Dd(directImage->getXSize() - 2.0, 256.0, directImage->getYSize() - 2.0);
                localVector = Vector3Dd(1.0 / directImage->getXSize(),
                    1.0 / 256.0, 1.0 / directImage->getYSize());
                *localShape->transformation = Matrix4x4d().scale(
                    localVector.x(), localVector.y(), localVector.z());
                *localShape->transformationInverse = Matrix4x4d().scale(
                    1.0 / localVector.x(), 1.0 / localVector.y(), 1.0 / localVector.z());
                Exit_Flag = true;
                break;

            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        ParseGlobals::HEIGHT_FIELD_CONSTANT) {
                        localShape = (HeightField *)GeometryOperations::copy(
                            (TransformableElement *)ctx.constants()[(int)constantId]
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
            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = true;
                break;

            case Tokenizer::WATER_LEVEL_TOKEN:
                localShape->boundingBox->bounds[0] =
                    localShape->boundingBox->bounds[0].withY(
                        PrimitiveParser::parseFloat(ctx));
                break;

            case Tokenizer::TRANSLATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::translate(
                    localShape, &localVector);
                break;

            case Tokenizer::ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::rotate(
                    localShape, &localVector);
                break;

            case Tokenizer::SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::scale(
                    localShape, &localVector);
                break;

            case Tokenizer::INVERSE_TOKEN:
                GeometryOperations::invert(localShape);
                break;

            case Tokenizer::TEXTURE_TOKEN:
                localTexture = TextureParser::parseTexture(ctx);
                if (localTexture->constantFlag) {
                    localTexture = TextureParser::copyTexture(localTexture);
                }

                TextureParser::prependTextureLayers(localTexture, localShape->material);
                break;

            case Tokenizer::COLOUR_TOKEN:
                localShape->shapeColor = ModelBuilder::getColor();
                PrimitiveParser::parseColor(localShape->shapeColor, ctx);
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
    return ((Geometry *)localShape);
}
#include "java/util/PriorityQueue.txx"
