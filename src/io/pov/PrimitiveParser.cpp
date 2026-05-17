#include "Parse.h"
#include "common/Frame.h"
#include "common/Matrices.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "io/Gif.h"
#include "io/Iff.h"
#include "io/Targa.h"
#include "io/Dump.h"
#include "render/Render.h"

#include "geom/Bezier.h"
#include "geom/Blob.h"
#include "geom/Boxes.h"
#include "geom/Csg.h"
#include "geom/HField.h"
#include "geom/Light.h"
#include "geom/Objects.h"
#include "geom/Planes.h"
#include "geom/Poly.h"
#include "geom/Quadrics.h"
#include "geom/Spheres.h"
#include "geom/Triangle.h"
#include "geom/ViewPnt.h"

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
    register int negative;
    register int signParsed;

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
        if (constants[(int)constantId].Constant_Type == FLOAT_CONSTANT) {
            localFloat = *((double *)constants[(int)constantId].Constant_Data);
            if (negative) {
                localFloat *= -1.0;
            }
        } else {
            ParseErrorReporter::typeError();
        }
    } else {
        ParseErrorReporter::Undeclared();
    }
    Exit_Flag = TRUE; break;

    case PLUS_TOKEN: if (signParsed)
    {
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
    Exit_Flag = TRUE; break;

        default: ParseErrorReporter::parseError(FLOAT_TOKEN);
    break;
    }
        }
    }

    return (localFloat);
}

void
PrimitiveParser::parseVector(Vector3D *givenVector)
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
        if (constants[(int)constantId].Constant_Type == VECTOR_CONSTANT) {
            *givenVector =
                *((Vector3D *)constants[(int)constantId].Constant_Data);
        } else {
            ParseErrorReporter::typeError();
        }
    } else {
        ParseErrorReporter::Undeclared();
    }
    Exit_Flag = TRUE; break;

        case LEFT_ANGLE_TOKEN:(givenVector->x) = PrimitiveParser::parseFloat();
    (givenVector->y) = PrimitiveParser::parseFloat();
    (givenVector->z) = PrimitiveParser::parseFloat();
    ParseHelpers::getExpectedToken(RIGHT_ANGLE_TOKEN);
    Exit_Flag = TRUE; break;

        default: ParseErrorReporter::parseError(LEFT_ANGLE_TOKEN);
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
    Exit_Flag = TRUE; break;

        default: ParseErrorReporter::parseError(LEFT_ANGLE_TOKEN);
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
        if (constants[(int)constantId].Constant_Type == COLOUR_CONSTANT) {
            *givenColour =
                *((RGBAColor *)constants[(int)constantId].Constant_Data);
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
    Exit_Flag = TRUE; break; }
        }
    }
}
