#include "io/pov/LightSourceParser.h"
#include "io/pov/Parse.h"
#include "io/pov/ParseHelpers.h"
#include "io/pov/PrimitiveParser.h"
#include "io/pov/SceneConfigParser.h"
#include "app/PovApp.h"
#include "common/Vector.h"
#include "geom/Light.h"
#include "geom/Geometry.h"

extern TokenStruct globalToken;
extern Constant constants[MAX_CONSTANTS];

Geometry *
LightSourceParser::parseLightSource()
{
    Light *localShape = nullptr;
    Vector3D localVector;
    CONSTANT constantId;

    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.Token_Id) {
    case LEFT_ANGLE_TOKEN:
    Tokenizer::ungetToken();
    localShape = SceneFactory::getLightSourceShape();
    PrimitiveParser::parseVector(&(localShape->Center));
    localShape->Shape_Colour = SceneFactory::getColour();
    Color::makeColor(localShape->Shape_Colour, 1.0, 1.0, 1.0);
    localShape->Shape_Colour->Alpha = 0.0;
    ParseHelpers::getExpectedToken(COLOUR_TOKEN);
    PrimitiveParser::parseColour(localShape->Shape_Colour);
    Exit_Flag = TRUE; break;

    case IDENTIFIER_TOKEN: if ((constantId = SceneConfigParser::findConstant()) != -1)
    {
        if (constants[(int)constantId].Constant_Type == LIGHT_SOURCE_CONSTANT) {
            localShape = (Light *)GeometryOps::copy(
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

        default: ParseErrorReporter::parseError(LEFT_ANGLE_TOKEN);
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

        case TRANSLATE_TOKEN: PrimitiveParser::parseVector(&localVector);
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

    /* Point that the spot is pointed at */
    case POINT_AT_TOKEN:
    PrimitiveParser::parseVector(&(localShape->Points_At));
    break;

    case TIGHTNESS_TOKEN:
    localShape->Coeff = PrimitiveParser::parseFloat();
    break;

    case RADIUS_TOKEN:
    localShape->Radius = cos(PrimitiveParser::parseFloat() * M_PI / 180.0);
    break;

    case COLOUR_TOKEN:
    PrimitiveParser::parseColour(localShape->Shape_Colour);
    break;

    case FALLOFF_TOKEN:
    localShape->Falloff = cos(PrimitiveParser::parseFloat() * M_PI / 180.0);
    break;

    case SPOTLIGHT_TOKEN:
    localShape->Type = SPOT_LIGHT_TYPE;
    break;

    default:
    ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN);
    break;

    }
        }
    }

    return ((Geometry *)localShape);
}
