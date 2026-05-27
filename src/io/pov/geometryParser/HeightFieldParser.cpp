#include "io/pov/ParserContext.h"
#include "io/pov/geometryParser/HeightFieldParser.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/volume/HeightField.h"
#include "io/base/image/GifFormat.h"
#include "io/base/image/TargaFormat.h"
#include "io/pov/Parse.h"
#include "io/pov/ParseHelpers.h"
#include "io/pov/PrimitiveParser.h"
#include "io/pov/SceneConfigParser.h"
#include "io/pov/mediaParser/TextureParser.h"


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
    CONSTANT constantId;
    Vector3Dd localVector;
    Texture *localTexture;
    RGBAImage *image = nullptr;
    int imageType = 0;

    localShape = nullptr;

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        int Exit_Flag;
        Exit_Flag = LegacyBoolean::FALSE_VALUE;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (
                ctx.token().tokenId) { /* This should be modified to include
                                           other image types - CdW */
            case Tokenizer::GIF_TOKEN:
                imageType = HeightField::GIF;
                localShape = ModelBuilder::getHeightFieldShape();
                image = new RGBAImage;
                if (image == nullptr) {
                    ParseErrorReporter::Error("Out of memory. Cannot allocate "
                                              "space for Height Field (1st "
                                              "message, ctx).");
                }
                ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                GifFormat::readGifImage(image, ctx.token().Token_String);
                localShape->bounding_box->bounds[0].x = 1.0;
                localShape->bounding_box->bounds[0].y = 0.0;
                localShape->bounding_box->bounds[0].z = 1.0;
                localShape->bounding_box->bounds[1].x = image->width - 2.0;
                localShape->bounding_box->bounds[1].y = 256.0;
                localShape->bounding_box->bounds[1].z = image->height - 2.0;
                VectorOps::makeVector(&localVector, 1.0 / (image->width),
                    1.0 / 256.0, 1.0 / (image->height));
                Transformation::getScalingTransformation(
                    localShape->transformation, &localVector);
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::POT_TOKEN:
                imageType = HeightField::POT;
                localShape = ModelBuilder::getHeightFieldShape();
                image = new RGBAImage;
                if (image == nullptr) {
                    ParseErrorReporter::Error("Out of memory. Cannot allocate "
                                              "space for Height Field (1st "
                                              "message, ctx).");
                }
                ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                GifFormat::readGifImage(image, ctx.token().Token_String);
                localShape->bounding_box->bounds[0].x = 1.0;
                localShape->bounding_box->bounds[0].y = 0.0;
                localShape->bounding_box->bounds[0].z = 1.0;
                localShape->bounding_box->bounds[1].x =
                    image->width / 2.0 - 2.0;
                localShape->bounding_box->bounds[1].y = 256.0;
                localShape->bounding_box->bounds[1].z = image->height - 2.0;
                VectorOps::makeVector(&localVector, 2.0 / image->width,
                    1.0 / 256.0, 1.0 / image->height);
                Transformation::getScalingTransformation(
                    localShape->transformation, &localVector);
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::TGA_TOKEN:
                imageType = HeightField::TGA;
                localShape = ModelBuilder::getHeightFieldShape();
                image = new RGBAImage;
                if (image == nullptr) {
                    ParseErrorReporter::Error("Cannot allocate space for "
                                              "Height Field (1st message, ctx).");
                }
                ParseHelpers::getExpectedToken(Tokenizer::STRING_TOKEN, ctx);
                TargaFormat::readTargaImage(image, ctx.token().Token_String);
                localShape->bounding_box->bounds[0].x = 1.0;
                localShape->bounding_box->bounds[0].y = 0.0;
                localShape->bounding_box->bounds[0].z = 1.0;
                localShape->bounding_box->bounds[1].x = image->width - 2.0;
                localShape->bounding_box->bounds[1].y = 256.0;
                localShape->bounding_box->bounds[1].z = image->height - 2.0;
                VectorOps::makeVector(&localVector, 1.0 / image->width,
                    1.0 / 256.0, 1.0 / image->height);
                Transformation::getScalingTransformation(
                    localShape->transformation, &localVector);
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant(ctx)) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        ParseGlobals::HEIGHT_FIELD_CONSTANT) {
                        localShape = (HeightField *)GeometryOperations::copy(
                            (SimpleBody *)ctx.constants()[(int)constantId]
                                .constantData);
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::Undeclared(ctx);
                }
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
            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::WATER_LEVEL_TOKEN:
                localShape->bounding_box->bounds[0].y =
                    PrimitiveParser::parseFloat(ctx);
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

                {
                    Texture *tempTexture;

                    for (tempTexture = localTexture;
                        tempTexture->Next_Texture != nullptr;
                        tempTexture = tempTexture->Next_Texture) {
                    }

                    tempTexture->Next_Texture = localShape->Shape_Texture;
                    localShape->Shape_Texture = localTexture;
                }
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

    HeightField::findHfMinMax(localShape, image, imageType);
    return ((Geometry *)localShape);
}
