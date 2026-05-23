#include "io/pov/geometryParser/BlobParser.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/volume/Blob.h"
#include "io/pov/Parse.h"
#include "io/pov/ParseHelpers.h"
#include "io/pov/PrimitiveParser.h"
#include "io/pov/SceneConfigParser.h"
#include "io/pov/mediaParser/TextureParser.h"

extern TokenStruct globalToken;
extern Constant constants[MAX_CONSTANTS];
extern void MakeBlob(SimpleBody *obj, double threshold, BlobList *components,
    int npoints, int surfaceType);

Geometry *
BlobParser::parseBlob()
{
    Blob *localShape;
    CONSTANT constantId;
    Vector3Dd localVector;
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
            switch (globalToken.tokenId) {
            case LEFT_CURLY_TOKEN:
                Exit_Flag = TRUE;
                break;
            default:
                ParseErrorReporter::parseError(LEFT_CURLY_TOKEN);
                break;
            }
        }
    }

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.tokenId) {
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
                        switch (globalToken.tokenId) {
                        case THRESHOLD_TOKEN:
                            threshold = PrimitiveParser::parseFloat();
                            break;

                        case COMPONENT_TOKEN:
                            blobComponent = new BlobList;
                            if (blobComponent == nullptr) {
                                ParseErrorReporter::Error(
                                    "Out of Memory! Cannot allocate blob "
                                    "component");
                            }
                            blobComponent->elem.coeffs[2] =
                                PrimitiveParser::parseFloat();
                            blobComponent->elem.radius2 =
                                PrimitiveParser::parseFloat();
                            PrimitiveParser::parseVector(
                                &blobComponent->elem.pos);
                            blobComponent->next = blobComponents;
                            blobComponents = blobComponent;
                            npoints++;
                            break;

                        default:
                            Tokenizer::ungetToken();
                            Exit_Flag = TRUE;
                            break;
                        }
                    }
                }

                /* Finally, process the information */
                MakeBlob((SimpleBody *)localShape, threshold, blobComponents,
                    npoints, 0);
                Exit_Flag = TRUE;
                break;

            case IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant()) != -1) {
                    if (constants[(int)constantId].constantType ==
                        BLOB_CONSTANT) {
                        localShape = (Blob *)GeometryOperations::copy(
                            (SimpleBody *)constants[(int)constantId]
                                .constantData);
                    } else {
                        ParseErrorReporter::typeError();
                    }
                } else {
                    ParseErrorReporter::Undeclared();
                }
                Exit_Flag = TRUE;
                break;

            default:
                ParseErrorReporter::parseError(FLOAT_TOKEN);
                break;
            }
        }
    }

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.tokenId) {
            case RIGHT_CURLY_TOKEN:
                Exit_Flag = TRUE;
                break;

            case STURM_TOKEN:
                localShape->sturmFlag = 1;
                break;

            case TRANSLATE_TOKEN:
                PrimitiveParser::parseVector(&localVector);
                GeometryOperations::translate(
                    (SimpleBody *)localShape, &localVector);
                break;

            case ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector);
                GeometryOperations::rotate(
                    (SimpleBody *)localShape, &localVector);
                break;

            case SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector);
                GeometryOperations::scale(
                    (SimpleBody *)localShape, &localVector);
                break;

            case INVERSE_TOKEN:
                GeometryOperations::invert((SimpleBody *)localShape);
                break;

            case TEXTURE_TOKEN:
                localTexture = TextureParser::parseTexture();
                if (localTexture->constantFlag) {
                    localTexture = TextureParser::copyTexture(localTexture);
                }
                {
                    for (tempTexture = localTexture;
                        tempTexture->Next_Texture != nullptr;
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
