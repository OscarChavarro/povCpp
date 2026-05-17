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

extern Frame *parsingFramePtr;
extern Constant constants[MAX_CONSTANTS];
extern int numberOfConstants;
extern int degenerateTriangles;



Geometry *
ShapeParser::parseLightSource()
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
    /*  linkShapes (Local_Shape, &(Local_Shape -> Next_Light_Source),
            &(Parsing_Frame_Ptr -> Light_Sources));
*/
    return ((Geometry *)localShape);
}

Geometry *
ShapeParser::parseSphere()
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

Geometry *
ShapeParser::parsePlane()
{
    InfinitePlane *localShape;
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
    localShape = SceneFactory::getPlaneShape();
    PrimitiveParser::parseVector(&(localShape->Normal_Vector));
    localShape->Distance = PrimitiveParser::parseFloat();
    localShape->Distance *= -1.0;
    Exit_Flag = TRUE; break;

    case IDENTIFIER_TOKEN: if ((constantId = SceneConfigParser::findConstant()) != -1)
    {
        if (constants[(int)constantId].Constant_Type == PLANE_CONSTANT) {
            localShape = (InfinitePlane *)GeometryOps::copy(
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

Geometry *
ShapeParser::parseHeightField()
{
    HeightField *localShape;
    CONSTANT constantId;
    Vector3D localVector;
    Texture *localTexture;
    RGBAImage *image = nullptr;
    int imageType = 0;

    localShape = nullptr;

    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.Token_Id) { /* This should be modified to include other image types - CdW */
        case GIF_TOKEN: imageType = GIF;
    localShape = SceneFactory::getHeightFieldShape();
    image = new RGBAImage;
    if (image == nullptr) {
        ParseErrorReporter::Error("Out of memory. Cannot allocate space for Height Field (1st "
              "message).");
    }
    ParseHelpers::getExpectedToken(STRING_TOKEN);
    GifFormat::readGifImage(image, globalToken.Token_String);
    localShape->bounding_box->bounds[0].x = 1.0;
    localShape->bounding_box->bounds[0].y = 0.0;
    localShape->bounding_box->bounds[0].z = 1.0;
    localShape->bounding_box->bounds[1].x = image->width - 2.0;
    localShape->bounding_box->bounds[1].y = 256.0;
    localShape->bounding_box->bounds[1].z = image->height - 2.0;
    VectorOps::makeVector(
        &localVector, 1.0 / (image->width), 1.0 / 256.0, 1.0 / (image->height));
    Transformation::getScalingTransformation(localShape->transformation, &localVector);
    Exit_Flag = TRUE; break;

        case POT_TOKEN: imageType = POT;
    localShape = SceneFactory::getHeightFieldShape();
    image = new RGBAImage;
    if (image == nullptr) {
        ParseErrorReporter::Error("Out of memory. Cannot allocate space for Height Field (1st "
              "message).");
    }
    ParseHelpers::getExpectedToken(STRING_TOKEN);
    GifFormat::readGifImage(image, globalToken.Token_String);
    localShape->bounding_box->bounds[0].x = 1.0;
    localShape->bounding_box->bounds[0].y = 0.0;
    localShape->bounding_box->bounds[0].z = 1.0;
    localShape->bounding_box->bounds[1].x = image->width / 2.0 - 2.0;
    localShape->bounding_box->bounds[1].y = 256.0;
    localShape->bounding_box->bounds[1].z = image->height - 2.0;
    VectorOps::makeVector(
        &localVector, 2.0 / image->width, 1.0 / 256.0, 1.0 / image->height);
    Transformation::getScalingTransformation(localShape->transformation, &localVector);
    Exit_Flag = TRUE; break;

        case TGA_TOKEN: imageType = TGA;
    localShape = SceneFactory::getHeightFieldShape();
    image = new RGBAImage;
    if (image == nullptr) {
        ParseErrorReporter::Error("Cannot allocate space for Height Field (1st message).");
    }
    ParseHelpers::getExpectedToken(STRING_TOKEN);
    TargaFormat::readTargaImage(image, globalToken.Token_String);
    localShape->bounding_box->bounds[0].x = 1.0;
    localShape->bounding_box->bounds[0].y = 0.0;
    localShape->bounding_box->bounds[0].z = 1.0;
    localShape->bounding_box->bounds[1].x = image->width - 2.0;
    localShape->bounding_box->bounds[1].y = 256.0;
    localShape->bounding_box->bounds[1].z = image->height - 2.0;
    VectorOps::makeVector(
        &localVector, 1.0 / image->width, 1.0 / 256.0, 1.0 / image->height);
    Transformation::getScalingTransformation(localShape->transformation, &localVector);
    Exit_Flag = TRUE; break;

    case IDENTIFIER_TOKEN: if ((constantId = SceneConfigParser::findConstant()) != -1)
    {
        if (constants[(int)constantId].Constant_Type == HEIGHT_FIELD_CONSTANT) {
            localShape = (HeightField *)GeometryOps::copy(
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

        default: ParseErrorReporter::parseError(GIF_TOKEN);
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

        case WATER_LEVEL_TOKEN: localShape->bounding_box->bounds[0]
            .y = PrimitiveParser::parseFloat();
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
        Texture *tempTexture;

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

    HeightField::findHfMinMax(localShape, image, imageType);
    return ((Geometry *)localShape);
}

Geometry *
ShapeParser::parseTriangle()
{
    Triangle *localShape;
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
    localShape = SceneFactory::getTriangleShape();
    PrimitiveParser::parseVector(&localShape->P1);
    PrimitiveParser::parseVector(&localShape->P2);
    PrimitiveParser::parseVector(&localShape->P3);
    if (!Triangle::computeTriangle(localShape)) {
        fprintf(stderr, "Degenerate triangle on line %d.  Please remove.\n",
            globalToken.Token_Line_No);
        degenerateTriangles = TRUE;
    }
    Exit_Flag = TRUE; break;

    case IDENTIFIER_TOKEN: if ((constantId = SceneConfigParser::findConstant()) != -1)
    {
        if (constants[(int)constantId].Constant_Type == TRIANGLE_CONSTANT) {
            localShape = (Triangle *)GeometryOps::copy(
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

Geometry *
ShapeParser::parseSmoothTriangle()
{
    SmoothTriangle *localShape;
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
    localShape = (SmoothTriangle *)SceneFactory::getSmoothTriangleShape();
    PrimitiveParser::parseVector(&localShape->P1);
    PrimitiveParser::parseVector(&localShape->N1);
    VectorOps::vNormalize(localShape->N1, localShape->N1);
    PrimitiveParser::parseVector(&localShape->P2);
    PrimitiveParser::parseVector(&localShape->N2);
    VectorOps::vNormalize(localShape->N2, localShape->N2);
    PrimitiveParser::parseVector(&localShape->P3);
    PrimitiveParser::parseVector(&localShape->N3);
    VectorOps::vNormalize(localShape->N3, localShape->N3);
    if (!Triangle::computeTriangle((Triangle *)localShape))
    {
        fprintf(stderr, "Degenerate triangle on line %d.  Please remove.\n",
            globalToken.Token_Line_No);
        degenerateTriangles = TRUE;
    }
    Exit_Flag = TRUE; break;

    case IDENTIFIER_TOKEN: if ((constantId = SceneConfigParser::findConstant()) != -1)
    {
        if (constants[(int)constantId].Constant_Type ==
            SMOOTH_TRIANGLE_CONSTANT) {
            localShape = (SmoothTriangle *)GeometryOps::copy(
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

Geometry *
ShapeParser::parseQuadric()
{
    Quadric *localShape;
    Vector3D localVector;
    CONSTANT constantId;
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
    localShape = SceneFactory::getQuadricShape();
    PrimitiveParser::parseVector(&(localShape->Object_2_Terms));
    PrimitiveParser::parseVector(&(localShape->Object_Mixed_Terms));
    PrimitiveParser::parseVector(&(localShape->Object_Terms));
    (localShape->Object_Constant) = PrimitiveParser::parseFloat();
    localShape->Non_Zero_Square_Term =
        !((localShape->Object_2_Terms.x == 0.0) &&
            (localShape->Object_2_Terms.y == 0.0) &&
            (localShape->Object_2_Terms.z == 0.0) &&
            (localShape->Object_Mixed_Terms.x == 0.0) &&
            (localShape->Object_Mixed_Terms.y == 0.0) &&
            (localShape->Object_Mixed_Terms.z == 0.0));
    Exit_Flag = TRUE; break;

    case IDENTIFIER_TOKEN: if ((constantId = SceneConfigParser::findConstant()) != -1)
    {
        if (constants[(int)constantId].Constant_Type == QUADRIC_CONSTANT) {
            localShape = (Quadric *)GeometryOps::copy(
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

Geometry *
ShapeParser::parsePoly(int knownOrder)
{
    Poly *localShape;
    Vector3D localVector;
    CONSTANT constantId;
    int order;
    Texture *localTexture;

    if (knownOrder > 0) {
        localShape = SceneFactory::getPolyShape(knownOrder);
    } else {
        localShape = nullptr;
    }

    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.Token_Id) {
    case DASH_TOKEN:
    case PLUS_TOKEN:
    case FLOAT_TOKEN:
    Tokenizer::ungetToken();
    if (localShape != nullptr) {
        ParseErrorReporter::Error("The order of a polynomial may not be specified twice");
    }
    order = (int)PrimitiveParser::parseFloat();
    if (order < 2 || order > MAX_ORDER) {
        ParseErrorReporter::Error("Order of Poly is out of range");
    }
    localShape = SceneFactory::getPolyShape(order);
    break;

    case LEFT_ANGLE_TOKEN:
    Tokenizer::ungetToken();
    if (localShape == nullptr) {
        printf("Need the order of the Poly");
    }
    PrimitiveParser::parseCoeffs(localShape->Order, &(localShape->Coeffs[0]));
    Exit_Flag = TRUE; break;

    case IDENTIFIER_TOKEN: if ((constantId = SceneConfigParser::findConstant()) != -1)
    {
        if (constants[(int)constantId].Constant_Type == POLY_CONSTANT) {
            localShape = (Poly *)GeometryOps::copy(
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

    ObjectUtils::link((SimpleBody *)localTexture, (SimpleBody **)&localTexture->Next_Texture,
        (SimpleBody **)&localShape->Shape_Texture);
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

Geometry *
ShapeParser::parseBicubicPatch()
{
    BicubicPatch *localShape = nullptr;
    Vector3D localVector;
    CONSTANT constantId;
    Texture *localTexture;
    int i;
    int j;

    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.Token_Id) {
    case DASH_TOKEN:
    case PLUS_TOKEN:
    case FLOAT_TOKEN:
    Tokenizer::ungetToken();
    localShape = SceneFactory::getBicubicPatchShape();
    localShape->Patch_Type = (int)PrimitiveParser::parseFloat();
    if (localShape->Patch_Type == 2 || localShape->Patch_Type == 3) {
        localShape->Flatness_Value = PrimitiveParser::parseFloat();
    } else {
        localShape->Flatness_Value = 0.1;
    }
    localShape->U_Steps = (int)PrimitiveParser::parseFloat();
    localShape->V_Steps = (int)PrimitiveParser::parseFloat();
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            PrimitiveParser::parseVector(&(localShape->Control_Points[i][j]));
        }
    }
    BicubicPatch::precomputePatchValues(localShape); /* interpolated mesh coords */
    Exit_Flag = TRUE; break;

    case IDENTIFIER_TOKEN: if ((constantId = SceneConfigParser::findConstant()) != -1)
    {
        if (constants[(int)constantId].Constant_Type ==
            BICUBIC_PATCH_CONSTANT) {
            localShape = (BicubicPatch *)GeometryOps::copy(
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

    ObjectUtils::link((SimpleBody *)localTexture, (SimpleBody **)&localTexture->Next_Texture,
        (SimpleBody **)&localShape->Shape_Texture);
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

Geometry *
ShapeParser::parseBox()
{
    Box *localShape;
    CONSTANT constantId;
    Vector3D localVector;
    Texture *localTexture;
    Texture *tempTexture;

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
    case LEFT_ANGLE_TOKEN:
    Tokenizer::ungetToken();
    localShape = SceneFactory::getBoxShape();
    PrimitiveParser::parseVector(&(localShape->bounds[0]));
    PrimitiveParser::parseVector(&(localShape->bounds[1]));
    Exit_Flag = TRUE; break;

    case IDENTIFIER_TOKEN: if ((constantId = SceneConfigParser::findConstant()) != -1)
    {
        if (constants[(int)constantId].Constant_Type == BOX_CONSTANT) {
            localShape = (Box *)GeometryOps::copy(
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

Geometry *
ShapeParser::parseBlob()
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

