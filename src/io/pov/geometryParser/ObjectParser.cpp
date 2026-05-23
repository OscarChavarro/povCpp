#include "io/pov/ParserContext.h"
#include "common/LegacyBoolean.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "io/pov/Parse.h"

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


CSG *
ObjectParser::parseCsg(int type)
{
    ParserContext ctx;
    return ObjectParser::parseCsg(type, ctx);
}

Geometry *
ObjectParser::parseShape()
{
    ParserContext ctx;
    return ObjectParser::parseShape(ctx);
}

SimpleBody *
ObjectParser::parseObject()
{
    ParserContext ctx;
    return ObjectParser::parseObject(ctx);
}

SimpleBody *
ObjectParser::parseComposite()
{
    ParserContext ctx;
    return ObjectParser::parseComposite(ctx);
}

CSG *
ObjectParser::parseCsg(int type, ParserContext &ctx)
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

    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (ctx.token().tokenId) {
            case IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant(ctx)) != -1) {
                    if ((ctx.constants()[(int)constantId].constantType ==
                            CSG_INTERSECTION_CONSTANT) ||
                        (ctx.constants()[(int)constantId].constantType ==
                            CSG_UNION_CONSTANT) ||
                        (ctx.constants()[(int)constantId].constantType ==
                            CSG_DIFFERENCE_CONSTANT)) {
                        delete container;
                        container = (CSG *)GeometryOperations::copy(
                            (SimpleBody *)ctx.constants()[(int)constantId]
                                .constantData);
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::Undeclared(ctx);
                }
                break;

            case LIGHT_SOURCE_TOKEN:
                localShape = LightSourceParser::parseLightSource(ctx);
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case SPHERE_TOKEN:
                localShape = SphereParser::parseSphere(ctx);
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case PLANE_TOKEN:
                localShape = PlaneParser::parsePlane(ctx);
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case TRIANGLE_TOKEN:
                localShape = TriangleParser::parseTriangle(ctx);
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case SMOOTH_TRIANGLE_TOKEN:
                localShape = SmoothTriangleParser::parseSmoothTriangle(ctx);
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case QUADRIC_TOKEN:
                localShape = QuadricParser::parseQuadric(ctx);
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case HEIGHT_FIELD_TOKEN:
                localShape = HeightFieldParser::parseHeightField(ctx);
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case CUBIC_TOKEN:
                localShape = PolyParser::parsePoly(3, ctx);
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case QUARTIC_TOKEN:
                localShape = PolyParser::parsePoly(4, ctx);
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case POLY_TOKEN:
                localShape = PolyParser::parsePoly(0, ctx);
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case BOX_TOKEN:
                localShape = BoxParser::parseBox(ctx);
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case BLOB_TOKEN:
                localShape = BlobParser::parseBlob(ctx);
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case BICUBIC_PATCH_TOKEN:
                localShape = BicubicPatchParser::parseBicubicPatch(ctx);
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case UNION_TOKEN:
                localShape =
                    (Geometry *)ObjectParser::parseCsg(CSG_UNION_TYPE, ctx);
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case INTERSECTION_TOKEN:
                localShape =
                    (Geometry *)ObjectParser::parseCsg(CSG_INTERSECTION_TYPE, ctx);
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(container->Shapes));
                break;

            case DIFFERENCE_TOKEN:
                localShape =
                    (Geometry *)ObjectParser::parseCsg(CSG_DIFFERENCE_TYPE, ctx);
                if ((type == CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert((SimpleBody *)localShape);
                }
                firstShapeParsed = TRUE;
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
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
            switch (ctx.token().tokenId) {
            case RIGHT_CURLY_TOKEN:
                Exit_Flag = TRUE;
                break;

            case TRANSLATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::translate(
                    (SimpleBody *)container, &localVector);
                break;

            case ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::rotate(
                    (SimpleBody *)container, &localVector);
                break;

            case SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::scale(
                    (SimpleBody *)container, &localVector);
                break;

            case INVERSE_TOKEN:
                GeometryOperations::invert((SimpleBody *)container);
                break;

            default:
                if (type == CSG_UNION_TYPE) {
                    ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN, ctx);
                } else if (type == CSG_INTERSECTION_TYPE) {
                    ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN, ctx);
                } else {
                    ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN, ctx);
                }
                break;
            }
        }
    }

    return ((CSG *)container);
}

Geometry *
ObjectParser::parseShape(ParserContext &ctx)
{
    Geometry *localShape = nullptr;

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (ctx.token().tokenId) {
            case LIGHT_SOURCE_TOKEN:
                localShape = LightSourceParser::parseLightSource(ctx);
                Exit_Flag = TRUE;
                break;

            case SPHERE_TOKEN:
                localShape = SphereParser::parseSphere(ctx);
                Exit_Flag = TRUE;
                break;

            case PLANE_TOKEN:
                localShape = PlaneParser::parsePlane(ctx);
                Exit_Flag = TRUE;
                break;

            case TRIANGLE_TOKEN:
                localShape = TriangleParser::parseTriangle(ctx);
                Exit_Flag = TRUE;
                break;

            case SMOOTH_TRIANGLE_TOKEN:
                localShape = SmoothTriangleParser::parseSmoothTriangle(ctx);
                Exit_Flag = TRUE;
                break;

            case QUADRIC_TOKEN:
                localShape = QuadricParser::parseQuadric(ctx);
                Exit_Flag = TRUE;
                break;

            case HEIGHT_FIELD_TOKEN:
                localShape = HeightFieldParser::parseHeightField(ctx);
                Exit_Flag = TRUE;
                break;

            case CUBIC_TOKEN:
                localShape = PolyParser::parsePoly(3, ctx);
                Exit_Flag = TRUE;
                break;

            case QUARTIC_TOKEN:
                localShape = PolyParser::parsePoly(4, ctx);
                Exit_Flag = TRUE;
                break;

            case POLY_TOKEN:
                localShape = PolyParser::parsePoly(0, ctx);
                Exit_Flag = TRUE;
                break;

            case BOX_TOKEN:
                localShape = BoxParser::parseBox(ctx);
                Exit_Flag = TRUE;
                break;

            case BLOB_TOKEN:
                localShape = BlobParser::parseBlob(ctx);
                Exit_Flag = TRUE;
                break;

            case BICUBIC_PATCH_TOKEN:
                localShape = BicubicPatchParser::parseBicubicPatch(ctx);
                Exit_Flag = TRUE;
                break;

            case UNION_TOKEN:
                localShape =
                    (Geometry *)ObjectParser::parseCsg(CSG_UNION_TYPE, ctx);
                Exit_Flag = TRUE;
                break;

            case INTERSECTION_TOKEN:
                localShape =
                    (Geometry *)ObjectParser::parseCsg(CSG_INTERSECTION_TYPE, ctx);
                Exit_Flag = TRUE;
                break;

            case DIFFERENCE_TOKEN:
                localShape =
                    (Geometry *)ObjectParser::parseCsg(CSG_DIFFERENCE_TYPE, ctx);
                Exit_Flag = TRUE;
                break;

            default:
                ParseErrorReporter::parseError(QUADRIC_TOKEN, ctx);
                break;
            }
        }
    }
    return (localShape);
}

SimpleBody *
ObjectParser::parseObject(ParserContext &ctx)
{
    SimpleBody *object;
    Geometry *localShape;
    Vector3Dd localVector;
    CONSTANT constantId;
    Texture *localTexture;
    Texture *tempTexture;

    object = nullptr;

    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (ctx.token().tokenId) {
            case IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant(ctx)) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        OBJECT_CONSTANT) {
                        object = (SimpleBody *)GeometryOperations::copy(
                            (SimpleBody *)ctx.constants()[(int)constantId]
                                .constantData);
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::Undeclared(ctx);
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

                localShape = ObjectParser::parseShape(ctx);
                ObjectUtils::link((SimpleBody *)localShape,
                    (SimpleBody **)&(localShape->nextObject),
                    (SimpleBody **)&(object->Shape));
                Exit_Flag = TRUE;
                break;

            default:
                ParseErrorReporter::parseError(SHAPE_TOKEN, ctx);
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
            switch (ctx.token().tokenId) {
            case BOUNDED_TOKEN:

                ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);

                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (ctx.token().tokenId) {
                        case RIGHT_CURLY_TOKEN:
                            Exit_Flag = TRUE;
                            break;

                        default:
                            Tokenizer::ungetToken();
                            localShape = ObjectParser::parseShape(ctx);
                            ObjectUtils::link((SimpleBody *)localShape,
                                (SimpleBody **)&(localShape->nextObject),
                                (SimpleBody **)&(object->boundingShapes));
                            break;
                        }
                    }
                }
                break;

            case CLIPPED_TOKEN:

                ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);

                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (ctx.token().tokenId) {
                        case RIGHT_CURLY_TOKEN:
                            Exit_Flag = TRUE;
                            break;

                        default:
                            Tokenizer::ungetToken();
                            localShape = ObjectParser::parseShape(ctx);
                            ObjectUtils::link((SimpleBody *)localShape,
                                (SimpleBody **)&(localShape->nextObject),
                                (SimpleBody **)&(object->clippingShapes));
                            break;
                        }
                    }
                }
                break;

            case COLOUR_TOKEN:
                object->objectColour = SceneFactory::getColour();
                PrimitiveParser::parseColour(object->objectColour, ctx);
                break;

            case TEXTURE_TOKEN:
                localTexture = TextureParser::parseTexture(ctx);
                if (localTexture->constantFlag) {
                    localTexture = TextureParser::copyTexture(localTexture);
                }

                if (object->objectTexture == Default_Texture) {
                    object->objectTexture = localTexture;
                } else {
                    for (tempTexture = localTexture;
                        tempTexture->Next_Texture != nullptr;
                        tempTexture = tempTexture->Next_Texture) {
                    }

                    tempTexture->Next_Texture = object->objectTexture;
                    object->objectTexture = localTexture;
                }
                break;

            case NO_SHADOW_TOKEN:
                object->noShadowFlag = TRUE;
                break;

            case LIGHT_SOURCE_TOKEN:
                ParseErrorReporter::Error(
                    "Light source must be defined using new syntax", ctx);
                break;

            case TRANSLATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::translate(object, &localVector);
                break;

            case ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::rotate(object, &localVector);
                break;

            case SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::scale(object, &localVector);
                break;

            case INVERSE_TOKEN:
                GeometryOperations::invert(object);
                break;

            case RIGHT_CURLY_TOKEN:
                Exit_Flag = TRUE;
                break;

            default:
                ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    return (object);
}

SimpleBody *
ObjectParser::parseComposite(ParserContext &ctx)
{
    Composite *localComposite;
    SimpleBody *localObject;
    Geometry *localShape;
    CONSTANT constantId;
    Vector3Dd localVector;

    localComposite = nullptr;

    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (ctx.token().tokenId) {
            case IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant(ctx)) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        COMPOSITE_CONSTANT) {
                        localComposite = (Composite *)GeometryOperations::copy(
                            (SimpleBody *)ctx.constants()[(int)constantId]
                                .constantData);
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::Undeclared(ctx);
                }
                break;

            case COMPOSITE_TOKEN:
                if (localComposite == nullptr) {
                    localComposite = SceneFactory::getCompositeObject();
                }

                localObject = ObjectParser::parseComposite(ctx);
                ObjectUtils::link((SimpleBody *)localObject,
                    (SimpleBody **)&(localObject->nextObject),
                    (SimpleBody **)&(localComposite->Objects));
                break;

            case OBJECT_TOKEN:
                if (localComposite == nullptr) {
                    localComposite = SceneFactory::getCompositeObject();
                }
                localObject = ObjectParser::parseObject(ctx);
                ObjectUtils::link(localObject, &(localObject->nextObject),
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
            switch (ctx.token().tokenId) {
            case RIGHT_CURLY_TOKEN:
                Exit_Flag = TRUE;
                break;

            case BOUNDED_TOKEN:

                ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);

                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (ctx.token().tokenId) {
                        case RIGHT_CURLY_TOKEN:
                            Exit_Flag = TRUE;
                            break;

                        default:
                            Tokenizer::ungetToken();
                            localShape = ObjectParser::parseShape(ctx);
                            ObjectUtils::link((SimpleBody *)localShape,
                                (SimpleBody **)&(localShape->nextObject),
                                (SimpleBody **)&(
                                    localComposite->boundingShapes));
                            break;
                        }
                    }
                }
                break;

            case CLIPPED_TOKEN:

                ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);

                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (ctx.token().tokenId) {
                        case RIGHT_CURLY_TOKEN:
                            Exit_Flag = TRUE;
                            break;

                        default:
                            Tokenizer::ungetToken();
                            localShape = ObjectParser::parseShape(ctx);
                            ObjectUtils::link((SimpleBody *)localShape,
                                (SimpleBody **)&(localShape->nextObject),
                                (SimpleBody **)&(
                                    localComposite->clippingShapes));
                            break;
                        }
                    }
                }
                break;

            case TRANSLATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::translate(
                    (SimpleBody *)localComposite, &localVector);
                break;

            case ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::rotate(
                    (SimpleBody *)localComposite, &localVector);
                break;

            case SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::scale(
                    (SimpleBody *)localComposite, &localVector);
                break;

            case INVERSE_TOKEN:
                GeometryOperations::invert((SimpleBody *)localComposite);
                break;

            default:
                ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    return ((SimpleBody *)localComposite);
}
