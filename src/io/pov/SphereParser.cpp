#include "io/pov/SphereParser.h"
#include "io/pov/Parse.h"
#include "io/pov/ParseHelpers.h"
#include "io/pov/PrimitiveParser.h"
#include "io/pov/SceneConfigParser.h"
#include "io/pov/TextureParser.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "geom/Spheres.h"
#include "geom/Geometry.h"

extern TokenStruct globalToken;
extern Constant constants[MAX_CONSTANTS];

Geometry *
SphereParser::parseSphere()
{
    Sphere *localShape;
    CONSTANT constantId;
    Vector3D localVector;
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
    localShape = SceneFactory::getSphereShape();
    PrimitiveParser::parseVector(&(localShape->Center));
    localShape->Radius = PrimitiveParser::parseFloat();
    localShape->Radius_Squared = localShape->Radius * localShape->Radius;
    localShape->Inverse_Radius = 1.0 / localShape->Radius;
    Exit_Flag = TRUE; break;

    case IDENTIFIER_TOKEN: if ((constantId = SceneConfigParser::findConstant()) != -1)
    {
        if (constants[(int)constantId].Constant_Type == SPHERE_CONSTANT) {
            localShape = (Sphere *)GeometryOps::copy(
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
