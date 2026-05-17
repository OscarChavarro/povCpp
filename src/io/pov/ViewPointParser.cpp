#include "io/pov/ViewPointParser.h"
#include "io/pov/Parse.h"
#include "app/PovApp.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/camera/Viewpoint.h"

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
    directionLength = givenVp->Direction.length();
    upLength = givenVp->Up.length();
    rightLength = givenVp->Right.length();
    tempVector = givenVp->Direction.crossProduct(givenVp->Up);
    handedness = tempVector.dotProduct(givenVp->Right);
    PrimitiveParser::parseVector(&givenVp->Direction);

    givenVp->Direction.sub(givenVp->Location);
    givenVp->Direction.normalize();
    givenVp->Right = givenVp->Direction.crossProduct(givenVp->Sky);
    givenVp->Right.normalize();
    givenVp->Up = givenVp->Right.crossProduct(givenVp->Direction);
    givenVp->Direction.scale(directionLength);
    if (handedness >= 0.0) {
        givenVp->Right.scale(rightLength);
    } else {
        givenVp->Right.scale(-rightLength);
    }

    givenVp->Up.scale(upLength);
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
