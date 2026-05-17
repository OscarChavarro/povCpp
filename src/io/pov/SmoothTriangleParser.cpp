#include "io/pov/SmoothTriangleParser.h"
#include "app/PovApp.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/elements/Triangle.h"
#include "io/pov/Parse.h"
#include "io/pov/ParseHelpers.h"
#include "io/pov/PrimitiveParser.h"
#include "io/pov/SceneConfigParser.h"
#include "io/pov/TextureParser.h"

extern TokenStruct globalToken;
extern Constant constants[MAX_CONSTANTS];
extern int degenerateTriangles;

Geometry *
SmoothTriangleParser::parseSmoothTriangle()
{
    SmoothTriangle *localShape;
    CONSTANT constantId;
    Vector3Dd localVector;
    Texture *localTexture;
    Texture *tempTexture;

    localShape = nullptr;

    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.Token_Id) {
            case LEFT_ANGLE_TOKEN:
                Tokenizer::ungetToken();
                localShape =
                    (SmoothTriangle *)SceneFactory::getSmoothTriangleShape();
                PrimitiveParser::parseVector(&localShape->P1);
                PrimitiveParser::parseVector(&localShape->N1);
                localShape->N1.normalize();
                PrimitiveParser::parseVector(&localShape->P2);
                PrimitiveParser::parseVector(&localShape->N2);
                localShape->N2.normalize();
                PrimitiveParser::parseVector(&localShape->P3);
                PrimitiveParser::parseVector(&localShape->N3);
                localShape->N3.normalize();
                if (!Triangle::computeTriangle((Triangle *)localShape)) {
                    fprintf(stderr,
                        "Degenerate triangle on line %d.  Please remove.\n",
                        globalToken.Token_Line_No);
                    degenerateTriangles = TRUE;
                }
                Exit_Flag = TRUE;
                break;

            case IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant()) != -1) {
                    if (constants[(int)constantId].Constant_Type ==
                        SMOOTH_TRIANGLE_CONSTANT) {
                        localShape = (SmoothTriangle *)GeometryOperations::copy(
                            (SimpleBody *)constants[(int)constantId]
                                .Constant_Data);
                    } else {
                        ParseErrorReporter::typeError();
                    }
                } else {
                    ParseErrorReporter::Undeclared();
                }
                Exit_Flag = TRUE;
                break;

            default:
                ParseErrorReporter::parseError(LEFT_ANGLE_TOKEN);
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
                Exit_Flag = TRUE;
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
                if (localTexture->Constant_Flag) {
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
