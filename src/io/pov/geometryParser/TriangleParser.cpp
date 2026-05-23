#include "io/pov/geometryParser/TriangleParser.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/elements/Triangle.h"
#include "io/pov/Parse.h"
#include "io/pov/ParseHelpers.h"
#include "io/pov/PrimitiveParser.h"
#include "io/pov/SceneConfigParser.h"
#include "io/pov/mediaParser/TextureParser.h"

extern TokenStruct globalToken;
extern Constant constants[MAX_CONSTANTS];
extern int degenerateTriangles;

Geometry *
TriangleParser::parseTriangle()
{
    Triangle *localShape;
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
            switch (globalToken.tokenId) {
            case LEFT_ANGLE_TOKEN:
                Tokenizer::ungetToken();
                localShape = SceneFactory::getTriangleShape();
                PrimitiveParser::parseVector(&localShape->P1);
                PrimitiveParser::parseVector(&localShape->P2);
                PrimitiveParser::parseVector(&localShape->P3);
                if (!Triangle::computeTriangle(localShape)) {
                    Logger::error(
                        "Degenerate triangle on line %d.  Please remove.\n",
                        globalToken.tokenLineNo);
                    degenerateTriangles = TRUE;
                }
                Exit_Flag = TRUE;
                break;

            case IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant()) != -1) {
                    if (constants[(int)constantId].constantType ==
                        TRIANGLE_CONSTANT) {
                        localShape = (Triangle *)GeometryOperations::copy(
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
            switch (globalToken.tokenId) {
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
