#include "io/pov/ViewPointParser.h"
#include "io/pov/Parse.h"
#include "app/PovApp.h"
#include "common/Vector3Dd.h"
#include "common/Vector3Dd.h"
#include "geom/Viewpoint.h"

extern TokenStruct globalToken;
extern Constant constants[MAX_CONSTANTS];
extern int numberOfConstants;

static CONSTANT
findConstant()
{
    register int i;

    for (i = 1; i <= numberOfConstants; i++) {
        if (constants[i].Identifier_Number == globalToken.Identifier_Number) {
            return (i);
        }
    }

    return (-1);
}

void
ViewPointParser::parseViewpoint(Viewpoint *givenVp)
{
    CONSTANT constantId;
    Vector3Dd localVector;
    Vector3Dd tempVector;
    double directionLength, upLength, rightLength, handedness;

    givenVp->initializeDefaults();

    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.Token_Id) {
    case IDENTIFIER_TOKEN:
    if ((constantId = findConstant()) != -1) {
        if (constants[(int)constantId].Constant_Type == VIEW_POINT_CONSTANT) {
            *givenVp = *((Viewpoint *)constants[(int)constantId].Constant_Data);
        } else {
            ParseErrorReporter::typeError();
        }
    } else {
        ParseErrorReporter::Undeclared();
    }
    break;

    case LOCATION_TOKEN:
    PrimitiveParser::parseVector(&(givenVp->Location));
    break;

    case DIRECTION_TOKEN:
    PrimitiveParser::parseVector(&(givenVp->Direction));
    break;

    case UP_TOKEN:
    PrimitiveParser::parseVector(&(givenVp->Up));
    break;

    case RIGHT_TOKEN:
    PrimitiveParser::parseVector(&(givenVp->Right));
    break;

    case SKY_TOKEN:
    PrimitiveParser::parseVector(&(givenVp->Sky));
    break;

    case LOOK_AT_TOKEN:
    VectorOps::vLength(directionLength, givenVp->Direction);
    VectorOps::vLength(upLength, givenVp->Up);
    VectorOps::vLength(rightLength, givenVp->Right);
    VectorOps::vCross(tempVector, givenVp->Direction, givenVp->Up);
    VectorOps::vDot(handedness, tempVector, givenVp->Right);
    PrimitiveParser::parseVector(&givenVp->Direction);

    VectorOps::vSub(givenVp->Direction, givenVp->Direction, givenVp->Location);
    VectorOps::vNormalize(givenVp->Direction, givenVp->Direction);
    VectorOps::vCross(givenVp->Right, givenVp->Direction, givenVp->Sky);
    VectorOps::vNormalize(givenVp->Right, givenVp->Right);
    VectorOps::vCross(givenVp->Up, givenVp->Right, givenVp->Direction);
    VectorOps::vScale(givenVp->Direction, givenVp->Direction, directionLength);
    if (handedness >= 0.0) {
        VectorOps::vScale(givenVp->Right, givenVp->Right, rightLength);
    } else {
        VectorOps::vScale(givenVp->Right, givenVp->Right, -rightLength);
    }

    VectorOps::vScale(givenVp->Up, givenVp->Up, upLength);
    break;

    case TRANSLATE_TOKEN:
    PrimitiveParser::parseVector(&localVector);
    GeometryOperations::translate((SimpleBody *)givenVp, &localVector);
    break;

    case ROTATE_TOKEN:
    PrimitiveParser::parseVector(&localVector);
    GeometryOperations::rotate((SimpleBody *)givenVp, &localVector);
    break;

    case SCALE_TOKEN:
    PrimitiveParser::parseVector(&localVector);
    GeometryOperations::scale((SimpleBody *)givenVp, &localVector);
    break;

    case RIGHT_CURLY_TOKEN:
    Exit_Flag = TRUE; break;

        default: ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN);
    break;
    }
        }
    }
}
