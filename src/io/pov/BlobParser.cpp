#include "io/pov/BlobParser.h"
#include "io/pov/Parse.h"
#include "io/pov/ParseHelpers.h"
#include "io/pov/PrimitiveParser.h"
#include "io/pov/SceneConfigParser.h"
#include "io/pov/TextureParser.h"
#include "app/PovApp.h"
#include "common/Vector.h"
#include "geom/Blob.h"
#include "geom/Geometry.h"

extern TokenStruct globalToken;
extern Constant constants[MAX_CONSTANTS];
extern void MakeBlob(SimpleBody *obj, double threshold, BlobList *components, int npoints, int surfaceType);

Geometry *
BlobParser::parseBlob()
{
    Blob *localShape;
    CONSTANT constantId;
    Vector3D localVector;
    Texture *localTexture;
    Texture *tempTexture;
    double threshold;
    int npoints;
    BlobList *blobComponents;
    BlobList *blobComponent;

    localShape = nullptr;
    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.Token_Id) {
    case LEFT_CURLY_TOKEN:
    Exit_Flag = TRUE; break; default: ParseErrorReporter::parseError(LEFT_CURLY_TOKEN);
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
    case THRESHOLD_TOKEN:
    case COMPONENT_TOKEN:
    Tokenizer::ungetToken();
    localShape = SceneFactory::getBlobShape();
    blobComponents = nullptr;
    npoints = 0;
    threshold = 1.0;

    /* Here is where we get the blob coefficients */
    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.Token_Id) {
    case THRESHOLD_TOKEN:
    threshold = PrimitiveParser::parseFloat();
    break;

    case COMPONENT_TOKEN:
    blobComponent = new BlobList;
    if (blobComponent == nullptr) {
        ParseErrorReporter::Error("Out of Memory! Cannot allocate blob component");
    }
    blobComponent->elem.coeffs[2] = PrimitiveParser::parseFloat();
    blobComponent->elem.radius2 = PrimitiveParser::parseFloat();
    PrimitiveParser::parseVector(&blobComponent->elem.pos);
    blobComponent->next = blobComponents;
    blobComponents = blobComponent;
    npoints++;
    break;

    default:
    Tokenizer::ungetToken();
    Exit_Flag = TRUE; break; }
        }
    }

        /* Finally, process the information */
        MakeBlob(
            (SimpleBody *)localShape, threshold, blobComponents, npoints, 0);
    Exit_Flag = TRUE; break;

    case IDENTIFIER_TOKEN: if ((constantId = SceneConfigParser::findConstant()) != -1)
    {
        if (constants[(int)constantId].Constant_Type == BLOB_CONSTANT) {
            localShape = (Blob *)GeometryOps::copy(
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

        default: ParseErrorReporter::parseError(FLOAT_TOKEN);
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

        case STURM_TOKEN: localShape->Sturm_Flag = 1;
    break;

    case TRANSLATE_TOKEN:
    PrimitiveParser::parseVector(&localVector);
    GeometryOps::translate((SimpleBody *)localShape, &localVector);
    break;

    case ROTATE_TOKEN:
    PrimitiveParser::parseVector(&localVector);
    GeometryOps::rotate((SimpleBody *)localShape, &localVector);
    break;

    case SCALE_TOKEN:
    PrimitiveParser::parseVector(&localVector);
    GeometryOps::scale((SimpleBody *)localShape, &localVector);
    break;

    case INVERSE_TOKEN:
    GeometryOps::invert((SimpleBody *)localShape);
    break;

    case TEXTURE_TOKEN:
    localTexture = TextureParser::parseTexture();
    if (localTexture->Constant_Flag) {
        localTexture = TextureParser::copyTexture(localTexture);
    }
    {
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

    return ((Geometry *)localShape);
}
