#include "io/pov/HeightFieldParser.h"
#include "io/pov/Parse.h"
#include "io/pov/ParseHelpers.h"
#include "io/pov/PrimitiveParser.h"
#include "io/pov/SceneConfigParser.h"
#include "io/pov/TextureParser.h"
#include "app/PovApp.h"
#include "common/Vector3D.h"
#include "common/VectorOps.h"
#include "io/GifFormat.h"
#include "io/IffFormat.h"
#include "io/TargaFormat.h"
#include "geom/HeightField.h"
#include "geom/GeometryOperations.h"

extern TokenStruct globalToken;
extern Constant constants[MAX_CONSTANTS];

Geometry *
HeightFieldParser::parseHeightField()
{
    HeightField *localShape;
    CONSTANT constantId;
    Vector3D localVector;
    Texture *localTexture;
    RGBAImage *image = nullptr;
    int imageType = 0;

    localShape = nullptr;

    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.Token_Id) { /* This should be modified to include other image types - CdW */
        case GIF_TOKEN: imageType = GIF;
    localShape = SceneFactory::getHeightFieldShape();
    image = new RGBAImage;
    if (image == nullptr) {
        ParseErrorReporter::Error("Out of memory. Cannot allocate space for Height Field (1st "
              "message).");
    }
    ParseHelpers::getExpectedToken(STRING_TOKEN);
    GifFormat::readGifImage(image, globalToken.Token_String);
    localShape->bounding_box->bounds[0].x = 1.0;
    localShape->bounding_box->bounds[0].y = 0.0;
    localShape->bounding_box->bounds[0].z = 1.0;
    localShape->bounding_box->bounds[1].x = image->width - 2.0;
    localShape->bounding_box->bounds[1].y = 256.0;
    localShape->bounding_box->bounds[1].z = image->height - 2.0;
    VectorOps::makeVector(
        &localVector, 1.0 / (image->width), 1.0 / 256.0, 1.0 / (image->height));
    Transformation::getScalingTransformation(localShape->transformation, &localVector);
    Exit_Flag = TRUE; break;

        case POT_TOKEN: imageType = POT;
    localShape = SceneFactory::getHeightFieldShape();
    image = new RGBAImage;
    if (image == nullptr) {
        ParseErrorReporter::Error("Out of memory. Cannot allocate space for Height Field (1st "
              "message).");
    }
    ParseHelpers::getExpectedToken(STRING_TOKEN);
    GifFormat::readGifImage(image, globalToken.Token_String);
    localShape->bounding_box->bounds[0].x = 1.0;
    localShape->bounding_box->bounds[0].y = 0.0;
    localShape->bounding_box->bounds[0].z = 1.0;
    localShape->bounding_box->bounds[1].x = image->width / 2.0 - 2.0;
    localShape->bounding_box->bounds[1].y = 256.0;
    localShape->bounding_box->bounds[1].z = image->height - 2.0;
    VectorOps::makeVector(
        &localVector, 2.0 / image->width, 1.0 / 256.0, 1.0 / image->height);
    Transformation::getScalingTransformation(localShape->transformation, &localVector);
    Exit_Flag = TRUE; break;

        case TGA_TOKEN: imageType = TGA;
    localShape = SceneFactory::getHeightFieldShape();
    image = new RGBAImage;
    if (image == nullptr) {
        ParseErrorReporter::Error("Cannot allocate space for Height Field (1st message).");
    }
    ParseHelpers::getExpectedToken(STRING_TOKEN);
    TargaFormat::readTargaImage(image, globalToken.Token_String);
    localShape->bounding_box->bounds[0].x = 1.0;
    localShape->bounding_box->bounds[0].y = 0.0;
    localShape->bounding_box->bounds[0].z = 1.0;
    localShape->bounding_box->bounds[1].x = image->width - 2.0;
    localShape->bounding_box->bounds[1].y = 256.0;
    localShape->bounding_box->bounds[1].z = image->height - 2.0;
    VectorOps::makeVector(
        &localVector, 1.0 / image->width, 1.0 / 256.0, 1.0 / image->height);
    Transformation::getScalingTransformation(localShape->transformation, &localVector);
    Exit_Flag = TRUE; break;

    case IDENTIFIER_TOKEN: if ((constantId = SceneConfigParser::findConstant()) != -1)
    {
        if (constants[(int)constantId].Constant_Type == HEIGHT_FIELD_CONSTANT) {
            localShape = (HeightField *)GeometryOperations::copy(
                (SimpleBody *)constants[(int)constantId].Constant_Data);
        } else {
            ParseErrorReporter::typeError();
        }
    }
    else
    {
        ParseErrorReporter::Undeclared();
    }
    Exit_Flag = TRUE; break;

        default: ParseErrorReporter::parseError(GIF_TOKEN);
    break;
    }
        }
    }

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.Token_Id) {
    case RIGHT_CURLY_TOKEN:
    Exit_Flag = TRUE; break;

        case WATER_LEVEL_TOKEN: localShape->bounding_box->bounds[0]
            .y = PrimitiveParser::parseFloat();
    break;

    case TRANSLATE_TOKEN:
    PrimitiveParser::parseVector(&localVector);
    GeometryOperations::translate((SimpleBody *)localShape, &localVector);
    break;

    case ROTATE_TOKEN:
    PrimitiveParser::parseVector(&localVector);
    GeometryOperations::rotate((SimpleBody *)localShape, &localVector);
    break;

    case SCALE_TOKEN:
    PrimitiveParser::parseVector(&localVector);
    GeometryOperations::scale((SimpleBody *)localShape, &localVector);
    break;

    case INVERSE_TOKEN:
    GeometryOperations::invert((SimpleBody *)localShape);
    break;

    case TEXTURE_TOKEN:
    localTexture = TextureParser::parseTexture();
    if (localTexture->Constant_Flag) {
        localTexture = TextureParser::copyTexture(localTexture);
    }

    {
        Texture *tempTexture;

        for (tempTexture = localTexture; tempTexture->Next_Texture != nullptr;
             tempTexture = tempTexture->Next_Texture) {
        }

        tempTexture->Next_Texture = localShape->Shape_Texture;
        localShape->Shape_Texture = localTexture;
    }
    break;

    case COLOUR_TOKEN:
    localShape->Shape_Colour = SceneFactory::getColour();
    PrimitiveParser::parseColour(localShape->Shape_Colour);
    break;

    default:
    ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN);
    break;
    }
        }
    }

    HeightField::findHfMinMax(localShape, image, imageType);
    return ((Geometry *)localShape);
}
