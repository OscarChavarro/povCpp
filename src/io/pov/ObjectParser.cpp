#include "app/PovApp.h"
#include "common/FrameConfig.h"
#include "common/Transformation.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "io/DumpFormat.h"
#include "io/GifFormat.h"
#include "io/IffFormat.h"
#include "io/TargaFormat.h"
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

CSG *
ObjectParser::parseCsg(int type)
{
    CSG *container = nullptr;
    Geometry *localShape;
    Vector3Dd localVector;
    CONSTANT constantId;
    int firstShapeParsed = FALSE;

    if (type == CSG_UNION_TYPE) {
        container = SceneFactory::getCsgUnion();

    } else if ((type == CSG_INTERSECTION_TYPE) ||
               (type == CSG_DIFFERENCE_TYPE)) {
        container = SceneFactory::getCsgIntersection();
    }

    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.Token_Id) {
            case IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant()) != -1) {
                    if ((constants[(int)constantId].Constant_Type ==
                            CSG_INTERSECTION_CONSTANT) ||
                        (constants[(int)constantId].Constant_Type ==
                            CSG_UNION_CONSTANT) ||
                        (constants[(int)constantId].Constant_Type ==
                            CSG_DIFFERENCE_CONSTANT)) {
                        delete container;
                        container = (CSG *)GeometryOperations::copy(
                            (SimpleBody *)constants[(int)constantId]
                                .Constant_Data);
                    } else {
                        ParseErrorReporter::typeError();
                    }
                } else {
                    ParseErrorReporter::Undeclared();
                }
                break;

            case LIGHT_SOURCE_TOKEN:
                localShape = LightSourceParser::parseLightSource();
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->Next_Object),
                    (SimpleBody **)&(container->Shapes));
                break;

            case SPHERE_TOKEN:
                localShape = SphereParser::parseSphere();
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->Next_Object),
                    (SimpleBody **)&(container->Shapes));
                break;

            case PLANE_TOKEN:
                localShape = PlaneParser::parsePlane();
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->Next_Object),
                    (SimpleBody **)&(container->Shapes));
                break;

            case TRIANGLE_TOKEN:
                localShape = TriangleParser::parseTriangle();
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->Next_Object),
                    (SimpleBody **)&(container->Shapes));
                break;

            case SMOOTH_TRIANGLE_TOKEN:
                localShape = SmoothTriangleParser::parseSmoothTriangle();
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->Next_Object),
                    (SimpleBody **)&(container->Shapes));
                break;

            case QUADRIC_TOKEN:
                localShape = QuadricParser::parseQuadric();
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->Next_Object),
                    (SimpleBody **)&(container->Shapes));
                break;

            case HEIGHT_FIELD_TOKEN:
                localShape = HeightFieldParser::parseHeightField();
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->Next_Object),
                    (SimpleBody **)&(container->Shapes));
                break;

            case CUBIC_TOKEN:
                localShape = PolyParser::parsePoly(3);
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->Next_Object),
                    (SimpleBody **)&(container->Shapes));
                break;

            case QUARTIC_TOKEN:
                localShape = PolyParser::parsePoly(4);
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->Next_Object),
                    (SimpleBody **)&(container->Shapes));
                break;

            case POLY_TOKEN:
                localShape = PolyParser::parsePoly(0);
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->Next_Object),
                    (SimpleBody **)&(container->Shapes));
                break;

            case BOX_TOKEN:
                localShape = BoxParser::parseBox();
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->Next_Object),
                    (SimpleBody **)&(container->Shapes));
                break;

            case BLOB_TOKEN:
                localShape = BlobParser::parseBlob();
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->Next_Object),
                    (SimpleBody **)&(container->Shapes));
                break;

            case BICUBIC_PATCH_TOKEN:
                localShape = BicubicPatchParser::parseBicubicPatch();
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->Next_Object),
                    (SimpleBody **)&(container->Shapes));
                break;

            case UNION_TOKEN:
                localShape =
                    (Geometry *)ObjectParser::parseCsg(CSG_UNION_TYPE);
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->Next_Object),
                    (SimpleBody **)&(container->Shapes));
                break;

            case INTERSECTION_TOKEN:
                localShape =
                    (Geometry *)ObjectParser::parseCsg(CSG_INTERSECTION_TYPE);
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->Next_Object),
                    (SimpleBody **)&(container->Shapes));
                break;

            case DIFFERENCE_TOKEN:
                localShape =
                    (Geometry *)ObjectParser::parseCsg(CSG_DIFFERENCE_TYPE);
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->Next_Object),
                    (SimpleBody **)&(container->Shapes));
                break;

            default:
                Tokenizer::ungetToken();
                Exit_Flag = TRUE;
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
                    (SimpleBody *)container, &localVector);
                break;

            case ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector);
                GeometryOperations::rotate(
                    (SimpleBody *)container, &localVector);
                break;

            case SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector);
                GeometryOperations::scale(
                    (SimpleBody *)container, &localVector);
                break;

            case INVERSE_TOKEN:
                GeometryOperations::invert((SimpleBody *)container);
                break;

            default:
                if (type == CSG_UNION_TYPE) {
                    ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN);
                } else if (type == CSG_INTERSECTION_TYPE) {
                    ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN);
                } else {
                    ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN);
                }
                break;
            }
        }
    }

    return ((CSG *)container);
}

Geometry *
ObjectParser::parseShape()
{
    Geometry *localShape = nullptr;

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.Token_Id) {
            case LIGHT_SOURCE_TOKEN:
                localShape = LightSourceParser::parseLightSource();
                Exit_Flag = TRUE;
                break;

            case SPHERE_TOKEN:
                localShape = SphereParser::parseSphere();
                Exit_Flag = TRUE;
                break;

            case PLANE_TOKEN:
                localShape = PlaneParser::parsePlane();
                Exit_Flag = TRUE;
                break;

            case TRIANGLE_TOKEN:
                localShape = TriangleParser::parseTriangle();
                Exit_Flag = TRUE;
                break;

            case SMOOTH_TRIANGLE_TOKEN:
                localShape = SmoothTriangleParser::parseSmoothTriangle();
                Exit_Flag = TRUE;
                break;

            case QUADRIC_TOKEN:
                localShape = QuadricParser::parseQuadric();
                Exit_Flag = TRUE;
                break;

            case HEIGHT_FIELD_TOKEN:
                localShape = HeightFieldParser::parseHeightField();
                Exit_Flag = TRUE;
                break;

            case CUBIC_TOKEN:
                localShape = PolyParser::parsePoly(3);
                Exit_Flag = TRUE;
                break;

            case QUARTIC_TOKEN:
                localShape = PolyParser::parsePoly(4);
                Exit_Flag = TRUE;
                break;

            case POLY_TOKEN:
                localShape = PolyParser::parsePoly(0);
                Exit_Flag = TRUE;
                break;

            case BOX_TOKEN:
                localShape = BoxParser::parseBox();
                Exit_Flag = TRUE;
                break;

            case BLOB_TOKEN:
                localShape = BlobParser::parseBlob();
                Exit_Flag = TRUE;
                break;

            case BICUBIC_PATCH_TOKEN:
                localShape = BicubicPatchParser::parseBicubicPatch();
                Exit_Flag = TRUE;
                break;

            case UNION_TOKEN:
                localShape =
                    (Geometry *)ObjectParser::parseCsg(CSG_UNION_TYPE);
                Exit_Flag = TRUE;
                break;

            case INTERSECTION_TOKEN:
                localShape =
                    (Geometry *)ObjectParser::parseCsg(CSG_INTERSECTION_TYPE);
                Exit_Flag = TRUE;
                break;

            case DIFFERENCE_TOKEN:
                localShape =
                    (Geometry *)ObjectParser::parseCsg(CSG_DIFFERENCE_TYPE);
                Exit_Flag = TRUE;
                break;

            default:
                ParseErrorReporter::parseError(QUADRIC_TOKEN);
                break;
            }
        }
    }
    return (localShape);
}

SimpleBody *
ObjectParser::parseObject()
{
    SimpleBody *object;
    Geometry *localShape;
    Vector3Dd localVector;
    CONSTANT constantId;
    Texture *localTexture;
    Texture *tempTexture;

    object = nullptr;

    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.Token_Id) {
            case IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant()) != -1) {
                    if (constants[(int)constantId].Constant_Type ==
                        OBJECT_CONSTANT) {
                        object = (SimpleBody *)GeometryOperations::copy(
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

            case SPHERE_TOKEN:
            case QUADRIC_TOKEN:
            case QUARTIC_TOKEN:
            case UNION_TOKEN:
            case INTERSECTION_TOKEN:
            case DIFFERENCE_TOKEN:
            case TRIANGLE_TOKEN:
            case SMOOTH_TRIANGLE_TOKEN:
            case PLANE_TOKEN:
            case CUBIC_TOKEN:
            case POLY_TOKEN:
            case BICUBIC_PATCH_TOKEN:
            case HEIGHT_FIELD_TOKEN:
            case LIGHT_SOURCE_TOKEN:
            case BOX_TOKEN:
            case BLOB_TOKEN:
                Tokenizer::ungetToken();
                if (object == nullptr) {
                    object = ObjectUtils::getObject();
                }

                localShape = ObjectParser::parseShape();
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->Next_Object),
                    (SimpleBody **)&(object->Shape));
                Exit_Flag = TRUE;
                break;

            default:
                ParseErrorReporter::parseError(SHAPE_TOKEN);
                Exit_Flag = TRUE;
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
            case BOUNDED_TOKEN:

                ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN);

                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (globalToken.Token_Id) {
                        case RIGHT_CURLY_TOKEN:
                            Exit_Flag = TRUE;
                            break;

                        default:
                            Tokenizer::ungetToken();
                            localShape = ObjectParser::parseShape();
                            ObjectUtils::link((SimpleBody *)localShape,
                                (SimpleBody **)&(localShape->Next_Object),
                                (SimpleBody **)&(object->Bounding_Shapes));
                            break;
                        }
                    }
                }
                break;

            case CLIPPED_TOKEN:

                ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN);

                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (globalToken.Token_Id) {
                        case RIGHT_CURLY_TOKEN:
                            Exit_Flag = TRUE;
                            break;

                        default:
                            Tokenizer::ungetToken();
                            localShape = ObjectParser::parseShape();
                            ObjectUtils::link((SimpleBody *)localShape,
                                (SimpleBody **)&(localShape->Next_Object),
                                (SimpleBody **)&(object->Clipping_Shapes));
                            break;
                        }
                    }
                }
                break;

            case COLOUR_TOKEN:
                object->Object_Colour = SceneFactory::getColour();
                PrimitiveParser::parseColour(object->Object_Colour);
                break;

            case TEXTURE_TOKEN:
                localTexture = TextureParser::parseTexture();
                if (localTexture->Constant_Flag) {
                    localTexture = TextureParser::copyTexture(localTexture);
                }

                if (object->Object_Texture == Default_Texture) {
                    object->Object_Texture = localTexture;
                } else {
                    for (tempTexture = localTexture;
                        tempTexture->Next_Texture != nullptr;
                        tempTexture = tempTexture->Next_Texture) {
                    }

                    tempTexture->Next_Texture = object->Object_Texture;
                    object->Object_Texture = localTexture;
                }
                break;

            case NO_SHADOW_TOKEN:
                object->No_Shadow_Flag = TRUE;
                break;

            case LIGHT_SOURCE_TOKEN:
                ParseErrorReporter::Error(
                    "Light source must be defined using new syntax");
                break;

            case TRANSLATE_TOKEN:
                PrimitiveParser::parseVector(&localVector);
                GeometryOperations::translate(object, &localVector);
                break;

            case ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector);
                GeometryOperations::rotate(object, &localVector);
                break;

            case SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector);
                GeometryOperations::scale(object, &localVector);
                break;

            case INVERSE_TOKEN:
                GeometryOperations::invert(object);
                break;

            case RIGHT_CURLY_TOKEN:
                Exit_Flag = TRUE;
                break;

            default:
                ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN);
                break;
            }
        }
    }

    return (object);
}

SimpleBody *
ObjectParser::parseComposite()
{
    Composite *localComposite;
    SimpleBody *localObject;
    Geometry *localShape;
    CONSTANT constantId;
    Vector3Dd localVector;

    localComposite = nullptr;

    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.Token_Id) {
            case IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant()) != -1) {
                    if (constants[(int)constantId].Constant_Type ==
                        COMPOSITE_CONSTANT) {
                        localComposite = (Composite *)GeometryOperations::copy(
                            (SimpleBody *)constants[(int)constantId]
                                .Constant_Data);
                    } else {
                        ParseErrorReporter::typeError();
                    }
                } else {
                    ParseErrorReporter::Undeclared();
                }
                break;

            case COMPOSITE_TOKEN:
                if (localComposite == nullptr) {
                    localComposite = SceneFactory::getCompositeObject();
                }

                localObject = ObjectParser::parseComposite();
                ObjectUtils::link((SimpleBody *)localObject,
                    (SimpleBody **)&(localObject->Next_Object),
                    (SimpleBody **)&(localComposite->Objects));
                break;

            case OBJECT_TOKEN:
                if (localComposite == nullptr) {
                    localComposite = SceneFactory::getCompositeObject();
                }
                localObject = ObjectParser::parseObject();
                ObjectUtils::link(localObject, &(localObject->Next_Object),
                    &(localComposite->Objects));
                break;

            case RIGHT_CURLY_TOKEN:
                Tokenizer::ungetToken();
                if (localComposite == nullptr) {
                    localComposite = SceneFactory::getCompositeObject();
                }
                Exit_Flag = TRUE;
                break;

            default:
                Tokenizer::ungetToken();
                Exit_Flag = TRUE;
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

            case BOUNDED_TOKEN:

                ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN);

                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (globalToken.Token_Id) {
                        case RIGHT_CURLY_TOKEN:
                            Exit_Flag = TRUE;
                            break;

                        default:
                            Tokenizer::ungetToken();
                            localShape = ObjectParser::parseShape();
                            ObjectUtils::link((SimpleBody *)localShape,
                                (SimpleBody **)&(localShape->Next_Object),
                                (SimpleBody **)&(
                                    localComposite->Bounding_Shapes));
                            break;
                        }
                    }
                }
                break;

            case CLIPPED_TOKEN:

                ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN);

                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (globalToken.Token_Id) {
                        case RIGHT_CURLY_TOKEN:
                            Exit_Flag = TRUE;
                            break;

                        default:
                            Tokenizer::ungetToken();
                            localShape = ObjectParser::parseShape();
                            ObjectUtils::link((SimpleBody *)localShape,
                                (SimpleBody **)&(localShape->Next_Object),
                                (SimpleBody **)&(
                                    localComposite->Clipping_Shapes));
                            break;
                        }
                    }
                }
                break;

            case TRANSLATE_TOKEN:
                PrimitiveParser::parseVector(&localVector);
                GeometryOperations::translate(
                    (SimpleBody *)localComposite, &localVector);
                break;

            case ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector);
                GeometryOperations::rotate(
                    (SimpleBody *)localComposite, &localVector);
                break;

            case SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector);
                GeometryOperations::scale(
                    (SimpleBody *)localComposite, &localVector);
                break;

            case INVERSE_TOKEN:
                GeometryOperations::invert((SimpleBody *)localComposite);
                break;

            default:
                ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN);
                break;
            }
        }
    }

    return ((SimpleBody *)localComposite);
}
