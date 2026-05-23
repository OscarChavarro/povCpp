#include "app/PovApp.h"
#include "common/FrameConfig.h"
#include "common/linealAlgebra/Transformation.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "io/image/DumpFormat.h"
#include "io/image/GifFormat.h"
#include "io/image/IffFormat.h"
#include "io/image/TargaFormat.h"
#include "io/pov/Parse.h"
#include "render/RenderEngine.h"

#include "environment/camera/Camera.h"
#include "environment/geometry/elements/Triangle.h"
#include "environment/geometry/surface/InfinitePlane.h"
#include "environment/geometry/surface/parametric/ParametricPatch.h"
#include "environment/geometry/volume/Blob.h"
#include "environment/geometry/volume/Box.h"
#include "environment/geometry/volume/HeightField.h"
#include "environment/geometry/volume/Quadric.h"
#include "environment/geometry/volume/Sphere.h"
#include "environment/geometry/volume/compound/CSG.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "environment/geometry/volume/polynomial/PolynomialShape.h"
#include "environment/light/Light.h"

extern ReservedWord globalReservedWords[];
extern double antialiasThreshold;
extern int termCounts[MAX_ORDER + 1];
extern TokenStruct globalToken;
extern double maxTraceLevel;
extern char verboseFormat;
extern unsigned int Options;
extern char statFileName[FILE_NAME_LENGTH];

extern RenderFrame *parsingFramePtr;
extern Constant constants[MAX_CONSTANTS];
extern int numberOfConstants;
extern int degenerateTriangles;

/* Parse a float.  Doesn't handle exponentiation. */
double
PrimitiveParser::parseFloat()
{
    double localFloat = 0.0;
    CONSTANT constantId;
    int negative;
    int signParsed;

    negative = FALSE;
    signParsed = FALSE;

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.Token_Id) {
            case IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant()) != -1) {
                    if (constants[(int)constantId].Constant_Type ==
                        FLOAT_CONSTANT) {
                        localFloat = *(
                            (double *)constants[(int)constantId].Constant_Data);
                        if (negative) {
                            localFloat *= -1.0;
                        }
                    } else {
                        ParseErrorReporter::typeError();
                    }
                } else {
                    ParseErrorReporter::Undeclared();
                }
                Exit_Flag = TRUE;
                break;

            case PLUS_TOKEN:
                if (signParsed) {
                    ParseErrorReporter::parseError(FLOAT_TOKEN);
                }
                signParsed = TRUE;
                break;

            case DASH_TOKEN:
                if (signParsed) {
                    ParseErrorReporter::parseError(FLOAT_TOKEN);
                }
                negative = TRUE;
                signParsed = TRUE;
                break;

            case FLOAT_TOKEN:
                localFloat = globalToken.Token_Float;
                if (negative) {
                    localFloat *= -1.0;
                }
                Exit_Flag = TRUE;
                break;

            default:
                ParseErrorReporter::parseError(FLOAT_TOKEN);
                break;
            }
        }
    }

    return (localFloat);
}

void
PrimitiveParser::parseVector(Vector3Dd *givenVector)
{
    CONSTANT constantId;

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.Token_Id) {
            case IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant()) != -1) {
                    if (constants[(int)constantId].Constant_Type ==
                        VECTOR_CONSTANT) {
                        *givenVector = *((Vector3Dd *)constants[(int)constantId]
                                .Constant_Data);
                    } else {
                        ParseErrorReporter::typeError();
                    }
                } else {
                    ParseErrorReporter::Undeclared();
                }
                Exit_Flag = TRUE;
                break;

            case LEFT_ANGLE_TOKEN:
                (givenVector->x) = PrimitiveParser::parseFloat();
                (givenVector->y) = PrimitiveParser::parseFloat();
                (givenVector->z) = PrimitiveParser::parseFloat();
                ParseHelpers::getExpectedToken(RIGHT_ANGLE_TOKEN);
                Exit_Flag = TRUE;
                break;

            default:
                ParseErrorReporter::parseError(LEFT_ANGLE_TOKEN);
                break;
            }
        }
    }
}

void
PrimitiveParser::parseCoeffs(int order, double *givenCoeffs)
{
    int i;

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.Token_Id) {
            case LEFT_ANGLE_TOKEN:
                for (i = 0; i < termCounts[order]; i++) {
                    givenCoeffs[i] = PrimitiveParser::parseFloat();
                }
                ParseHelpers::getExpectedToken(RIGHT_ANGLE_TOKEN);
                Exit_Flag = TRUE;
                break;

            default:
                ParseErrorReporter::parseError(LEFT_ANGLE_TOKEN);
                break;
            }
        }
    }
}

void
PrimitiveParser::parseColour(RGBAColor *givenColour)
{
    CONSTANT constantId;
    Color::makeColor(givenColour, 0.0, 0.0, 0.0);
    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.Token_Id) {
            case IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant()) != -1) {
                    if (constants[(int)constantId].Constant_Type ==
                        COLOUR_CONSTANT) {
                        *givenColour = *((RGBAColor *)constants[(int)constantId]
                                .Constant_Data);
                    } else {
                        ParseErrorReporter::typeError();
                    }
                } else {
                    ParseErrorReporter::Undeclared();
                }
                break;

            case RED_TOKEN:
                (givenColour->Red) = PrimitiveParser::parseFloat();
                break;

            case GREEN_TOKEN:
                (givenColour->Green) = PrimitiveParser::parseFloat();
                break;

            case BLUE_TOKEN:
                (givenColour->Blue) = PrimitiveParser::parseFloat();
                break;

            case ALPHA_TOKEN:
                (givenColour->Alpha) = PrimitiveParser::parseFloat();
                break;

            default:
                Tokenizer::ungetToken();
                Exit_Flag = TRUE;
                break;
            }
        }
    }
}
