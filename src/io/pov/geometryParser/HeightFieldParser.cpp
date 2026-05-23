#include "io/pov/ParserContext.h"
#include "io/pov/geometryParser/HeightFieldParser.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/volume/HeightField.h"
#include "io/image/GifFormat.h"
#include "io/image/TargaFormat.h"
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

    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (
                ctx.token().tokenId) { /* This should be modified to include
                                           other image types - CdW */
            case GIF_TOKEN:
                imageType = GIF;
                localShape = ModelBuilder::getHeightFieldShape();
                image = new RGBAImage;
                if (image == nullptr) {
                    ParseErrorReporter::Error("Out of memory. Cannot allocate "
                                              "space for Height Field (1st "
                                              "message, ctx).");
                }
                ParseHelpers::getExpectedToken(STRING_TOKEN, ctx);
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
                Exit_Flag = TRUE;
                break;

            case POT_TOKEN:
                imageType = POT;
                localShape = ModelBuilder::getHeightFieldShape();
                image = new RGBAImage;
                if (image == nullptr) {
                    ParseErrorReporter::Error("Out of memory. Cannot allocate "
                                              "space for Height Field (1st "
                                              "message, ctx).");
                }
                ParseHelpers::getExpectedToken(STRING_TOKEN, ctx);
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
                Exit_Flag = TRUE;
                break;

            case TGA_TOKEN:
                imageType = TGA;
                localShape = ModelBuilder::getHeightFieldShape();
                image = new RGBAImage;
                if (image == nullptr) {
                    ParseErrorReporter::Error("Cannot allocate space for "
                                              "Height Field (1st message, ctx).");
                }
                ParseHelpers::getExpectedToken(STRING_TOKEN, ctx);
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
                Exit_Flag = TRUE;
                break;

            case IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant(ctx)) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        HEIGHT_FIELD_CONSTANT) {
                        localShape = (HeightField *)GeometryOperations::copy(
                            (SimpleBody *)ctx.constants()[(int)constantId]
                                .constantData);
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::Undeclared(ctx);
                }
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
            case RIGHT_CURLY_TOKEN:
                Exit_Flag = TRUE;
                break;

            case WATER_LEVEL_TOKEN:
                localShape->bounding_box->bounds[0].y =
                    PrimitiveParser::parseFloat(ctx);
                break;

            case TRANSLATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::translate(
                    (SimpleBody *)localShape, &localVector);
                break;

            case ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::rotate(
                    (SimpleBody *)localShape, &localVector);
                break;

            case SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::scale(
                    (SimpleBody *)localShape, &localVector);
                break;

            case INVERSE_TOKEN:
                GeometryOperations::invert((SimpleBody *)localShape);
                break;

            case TEXTURE_TOKEN:
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

            case COLOUR_TOKEN:
                localShape->Shape_Colour = ModelBuilder::getColour();
                PrimitiveParser::parseColour(localShape->Shape_Colour, ctx);
                break;

            default:
                ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    HeightField::findHfMinMax(localShape, image, imageType);
    return ((Geometry *)localShape);
}
