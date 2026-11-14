

#include "io/pov/geometry/SceneBuilder.h"

#include "io/image/GifFormat.h"
#include "io/image/TargaFormat.h"
#include "io/pov/geometry/HeightFieldParser.h"
#include "environment/material/povray/PovRayMaterialConstancy.h"
#include "io/pov/material/TextureParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"
#include "java/util/PriorityQueue.txx"

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
    HeightFieldSolid *localShape = nullptr;
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
                imageType = GIF;
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
                // The body scales the grid to the unit square.
                localVector = Vector3Dd(1.0 / indexedImage->getXSize(),
                    1.0 / 256.0, 1.0 / indexedImage->getYSize());
                localShape = new HeightFieldSolid(indexedImage->getXSize() + 1,
                    indexedImage->getYSize() + 1, minBounds, maxBounds);
                body = SceneBuilder::wrap(localShape);
                body->scaleOwnerOnly(&localVector);
                Exit_Flag = true;
                break;
            }

            case Tokenizer::POT_TOKEN:
            {
                imageType = POT;
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
                // A POT image holds the high and low height bytes side by side.
                localVector = Vector3Dd(2.0 / indexedImage->getXSize(),
                    1.0 / 256.0, 1.0 / indexedImage->getYSize());
                localShape = new HeightFieldSolid(indexedImage->getXSize() / 2 + 1,
                    indexedImage->getYSize() + 1, minBounds, maxBounds);
                body = SceneBuilder::wrap(localShape);
                body->scaleOwnerOnly(&localVector);
                Exit_Flag = true;
                break;
            }

            case Tokenizer::TGA_TOKEN:
            {
                imageType = TGA;
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
                localShape = new HeightFieldSolid(directImage->getXSize() + 1,
                    directImage->getYSize() + 1, minBounds, maxBounds);
                body = SceneBuilder::wrap(localShape);
                body->scaleOwnerOnly(&localVector);
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
                        localShape = static_cast<HeightFieldSolid *>(body->getGeometry());
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
                setWaterLevel(localShape, PrimitiveParser::parseFloat(ctx));
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
        findHfMinMax(localShape, indexedImage, imageType);
    } else if (directImage != nullptr) {
        findHfMinMax(localShape, directImage, imageType);
    }
    // The images are scratch buffers copied into the height grid above.
    delete indexedImage;
    delete directImage;
    return body;
}

void
HeightFieldParser::setWaterLevel(HeightFieldSolid *solid, double level)
{
    solid->setBounds(solid->getMinimum().withY(level), solid->getMaximum());
    solid->updateBlocks();
}

double
HeightFieldParser::imageHeight(const HeightFieldSolid *solid, double height)
{
    return height <= solid->getMinimum().y() ? HIDDEN_HEIGHT : height;
}

void
HeightFieldParser::findHfMinMax(HeightFieldSolid *solid,
    const IndexedColorImageHDRUncompressed *image, int imageType)
{
    int maxX = image->getXSize();
    if (imageType == POT) {
        maxX = maxX / 2;
    }
    const int maxZ = image->getYSize();
    for (int z = 0; z <= maxZ; z++) {
        for (int x = 0; x <= maxX; x++) {
            double height = HIDDEN_HEIGHT;
            if ((x > 1) && (x < maxX - 1) && (z > 1) && (z < maxZ - 1)) {
                const int temp1 = image->getPixel(x, maxZ - z - 1);
                if (imageType == POT) {
                    const int temp2 = image->getPixel(x + maxX, maxZ - z - 1);
                    height = imageHeight(solid, (double)temp1 + (double)temp2 / 256.0);
                } else {
                    height = imageHeight(solid, (double)temp1);
                }
            }
            solid->setHeight(x, z, height);
        }
    }
    solid->updateBlocks();
}

void
HeightFieldParser::findHfMinMax(HeightFieldSolid *solid,
    const RGBAImageHDRUncompressed *image, int imageType)
{
    (void)imageType;
    const int maxX = image->getXSize();
    const int maxZ = image->getYSize();
    for (int z = 0; z <= maxZ; z++) {
        for (int x = 0; x <= maxX; x++) {
            double height = HIDDEN_HEIGHT;
            if ((x > 1) && (x < maxX - 1) && (z > 1) && (z < maxZ - 1)) {
                RGBAPixelHDR pixel;
                image->getPixel(x, maxZ - z - 1, &pixel);
                height = imageHeight(solid, (double)pixel.r + (double)pixel.g / 256.0);
            }
            solid->setHeight(x, z, height);
        }
    }
    solid->updateBlocks();
}
