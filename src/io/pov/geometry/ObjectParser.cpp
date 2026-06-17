#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

#include "environment/camera/Camera.h"
#include "environment/geometry/elements/Triangle.h"
#include "environment/geometry/surface/parametric/ParametricPatch.h"
#include "environment/geometry/volume/Blob.h"
#include "environment/geometry/volume/Box.h"
#include "environment/geometry/volume/HeightField.h"
#include "environment/geometry/volume/Quadric.h"
#include "environment/geometry/volume/Sphere.h"
#include "environment/geometry/volume/compound/CSG.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "environment/light/Light.h"
#include "environment/material/MaterialUtils.h"
#include "environment/scene/ModelBuilder.h"
#include "environment/scene/BoundedGeometryFactory.h"
#include "environment/geometry/SimpleBody.h"

#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/geometry/BicubicPatchParser.h"
#include "io/pov/geometry/BlobParser.h"
#include "io/pov/geometry/BoxParser.h"
#include "io/pov/geometry/HeightFieldParser.h"
#include "io/pov/geometry/ObjectParser.h"
#include "io/pov/geometry/PlaneParser.h"
#include "io/pov/geometry/PolyParser.h"
#include "io/pov/geometry/QuadricParser.h"
#include "io/pov/geometry/SmoothTriangleParser.h"
#include "io/pov/geometry/SphereParser.h"
#include "io/pov/geometry/TriangleParser.h"
#include "io/pov/light/LightSourceParser.h"
#include "io/pov/material/TextureParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"

CSG *
ObjectParser::parseCsg(GeometryTypes type)
{
    ParserContext ctx;
    return ObjectParser::parseCsg(type, ctx);
}

SimpleBody *
ObjectParser::parseShape()
{
    ParserContext ctx;
    return ObjectParser::parseShape(ctx);
}

BoundedGeometry *
ObjectParser::parseObject()
{
    ParserContext ctx;
    return ObjectParser::parseObject(ctx);
}

BoundedGeometry *
ObjectParser::parseComposite()
{
    ParserContext ctx;
    return ObjectParser::parseComposite(ctx);
}

CSG *
ObjectParser::parseCsg(GeometryTypes type, ParserContext &ctx)
{
    CSG *container = nullptr;
    SimpleBody *localShape;
    Vector3Dd localVector;
    int constantId;
    bool firstShapeParsed = false;

    if (type == GeometryTypes::CSG_UNION_TYPE) {
        container = ModelBuilder::getCsgUnion();

    } else if ((type == GeometryTypes::CSG_INTERSECTION_TYPE) ||
               (type == GeometryTypes::CSG_DIFFERENCE_TYPE)) {
        container = ModelBuilder::getCsgIntersection();
    }

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = ctx.findConstant()) != -1) {
                    if ((ctx.constants()[(int)constantId].constantType ==
                            ParseGlobals::CSG_INTERSECTION_CONSTANT) ||
                        (ctx.constants()[(int)constantId].constantType ==
                            ParseGlobals::CSG_UNION_CONSTANT) ||
                        (ctx.constants()[(int)constantId].constantType ==
                            ParseGlobals::CSG_DIFFERENCE_CONSTANT)) {
                        delete container;
                        container = (CSG *)GeometryOperations::copy(
                            (TransformableElement *)ctx.constants()[(int)constantId]
                                .constantData);
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::reportUndeclared(ctx);
                }
                break;

            case Tokenizer::LIGHT_SOURCE_TOKEN:
                localShape = LightSourceParser::parseLightSource(ctx);
                if ((type == GeometryTypes::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert(localShape);
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::SPHERE_TOKEN:
                localShape = SphereParser::parseSphere(ctx);
                if ((type == GeometryTypes::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert(localShape);
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::PLANE_TOKEN:
                localShape = PlaneParser::parsePlane(ctx);
                if ((type == GeometryTypes::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert(localShape);
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::TRIANGLE_TOKEN:
                localShape = TriangleParser::parseTriangle(ctx);
                if ((type == GeometryTypes::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert(localShape);
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::SMOOTH_TRIANGLE_TOKEN:
                localShape = SmoothTriangleParser::parseSmoothTriangle(ctx);
                if ((type == GeometryTypes::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert(localShape);
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::QUADRIC_TOKEN:
                localShape = QuadricParser::parseQuadric(ctx);
                if ((type == GeometryTypes::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert(localShape);
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::HEIGHT_FIELD_TOKEN:
                localShape = HeightFieldParser::parseHeightField(ctx);
                if ((type == GeometryTypes::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert(localShape);
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::CUBIC_TOKEN:
                localShape = PolyParser::parsePoly(3, ctx);
                if ((type == GeometryTypes::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert(localShape);
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::QUARTIC_TOKEN:
                localShape = PolyParser::parsePoly(4, ctx);
                if ((type == GeometryTypes::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert(localShape);
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::POLY_TOKEN:
                localShape = PolyParser::parsePoly(0, ctx);
                if ((type == GeometryTypes::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert(localShape);
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::BOX_TOKEN:
                localShape = BoxParser::parseBox(ctx);
                if ((type == GeometryTypes::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert(localShape);
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::BLOB_TOKEN:
                localShape = BlobParser::parseBlob(ctx);
                if ((type == GeometryTypes::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert(localShape);
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::BICUBIC_PATCH_TOKEN:
                localShape = BicubicPatchParser::parseBicubicPatch(ctx);
                if ((type == GeometryTypes::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert(localShape);
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::UNION_TOKEN:
                localShape =
                    ModelBuilder::wrap(ObjectParser::parseCsg(GeometryTypes::CSG_UNION_TYPE, ctx));
                if ((type == GeometryTypes::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert(localShape);
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::INTERSECTION_TOKEN:
                localShape =
                    ModelBuilder::wrap(ObjectParser::parseCsg(GeometryTypes::CSG_INTERSECTION_TYPE, ctx));
                if ((type == GeometryTypes::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert(localShape);
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            case Tokenizer::DIFFERENCE_TOKEN:
                localShape =
                    ModelBuilder::wrap(ObjectParser::parseCsg(GeometryTypes::CSG_DIFFERENCE_TYPE, ctx));
                if ((type == GeometryTypes::CSG_DIFFERENCE_TYPE) && firstShapeParsed) {
                    GeometryOperations::invert(localShape);
                }
                firstShapeParsed = true;
                container->getShapes().add(localShape);
                break;

            default:
                ctx.tokenStream().ungetToken();
                Exit_Flag = true;
                break;
            }
        }
    }

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = true;
                break;

            case Tokenizer::TRANSLATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::translate(
                    container, &localVector);
                break;

            case Tokenizer::ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::rotate(
                    container, &localVector);
                break;

            case Tokenizer::SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::scale(
                    container, &localVector);
                break;

            case Tokenizer::INVERSE_TOKEN:
                GeometryOperations::invert(container);
                break;

            default:
                if (type == GeometryTypes::CSG_UNION_TYPE) {
                    ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                } else if (type == GeometryTypes::CSG_INTERSECTION_TYPE) {
                    ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                } else {
                    ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                }
                break;
            }
        }
    }

    return ((CSG *)container);
}

SimpleBody *
ObjectParser::parseShape(ParserContext &ctx)
{
    SimpleBody *localShape = nullptr;

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::LIGHT_SOURCE_TOKEN:
                localShape = LightSourceParser::parseLightSource(ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::SPHERE_TOKEN:
                localShape = SphereParser::parseSphere(ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::PLANE_TOKEN:
                localShape = PlaneParser::parsePlane(ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::TRIANGLE_TOKEN:
                localShape = TriangleParser::parseTriangle(ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::SMOOTH_TRIANGLE_TOKEN:
                localShape = SmoothTriangleParser::parseSmoothTriangle(ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::QUADRIC_TOKEN:
                localShape = QuadricParser::parseQuadric(ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::HEIGHT_FIELD_TOKEN:
                localShape = HeightFieldParser::parseHeightField(ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::CUBIC_TOKEN:
                localShape = PolyParser::parsePoly(3, ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::QUARTIC_TOKEN:
                localShape = PolyParser::parsePoly(4, ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::POLY_TOKEN:
                localShape = PolyParser::parsePoly(0, ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::BOX_TOKEN:
                localShape = BoxParser::parseBox(ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::BLOB_TOKEN:
                localShape = BlobParser::parseBlob(ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::BICUBIC_PATCH_TOKEN:
                localShape = BicubicPatchParser::parseBicubicPatch(ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::UNION_TOKEN:
                localShape =
                    ModelBuilder::wrap(ObjectParser::parseCsg(GeometryTypes::CSG_UNION_TYPE, ctx));
                Exit_Flag = true;
                break;

            case Tokenizer::INTERSECTION_TOKEN:
                localShape =
                    ModelBuilder::wrap(ObjectParser::parseCsg(GeometryTypes::CSG_INTERSECTION_TYPE, ctx));
                Exit_Flag = true;
                break;

            case Tokenizer::DIFFERENCE_TOKEN:
                localShape =
                    ModelBuilder::wrap(ObjectParser::parseCsg(GeometryTypes::CSG_DIFFERENCE_TYPE, ctx));
                Exit_Flag = true;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::QUADRIC_TOKEN, ctx);
                break;
            }
        }
    }
    return (localShape);
}

BoundedGeometry *
ObjectParser::parseObject(ParserContext &ctx)
{
    BoundedGeometry *object;
    SimpleBody *localShape;
    Vector3Dd localVector;
    int constantId;
    PovrayMaterial *localTexture;

    object = nullptr;

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        ParseGlobals::OBJECT_CONSTANT) {
                        object = (BoundedGeometry *)GeometryOperations::copy(
                            (TransformableElement *)ctx.constants()[(int)constantId]
                                .constantData);
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::reportUndeclared(ctx);
                }
                Exit_Flag = true;
                break;

            case Tokenizer::SPHERE_TOKEN:
            case Tokenizer::QUADRIC_TOKEN:
            case Tokenizer::QUARTIC_TOKEN:
            case Tokenizer::UNION_TOKEN:
            case Tokenizer::INTERSECTION_TOKEN:
            case Tokenizer::DIFFERENCE_TOKEN:
            case Tokenizer::TRIANGLE_TOKEN:
            case Tokenizer::SMOOTH_TRIANGLE_TOKEN:
            case Tokenizer::PLANE_TOKEN:
            case Tokenizer::CUBIC_TOKEN:
            case Tokenizer::POLY_TOKEN:
            case Tokenizer::BICUBIC_PATCH_TOKEN:
            case Tokenizer::HEIGHT_FIELD_TOKEN:
            case Tokenizer::LIGHT_SOURCE_TOKEN:
            case Tokenizer::BOX_TOKEN:
            case Tokenizer::BLOB_TOKEN:
                ctx.tokenStream().ungetToken();
                if (object == nullptr) {
                    object = BoundedGeometryFactory::getObject();
                }

                localShape = ObjectParser::parseShape(ctx);
                object->geometry = localShape;
                Exit_Flag = true;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::SHAPE_TOKEN, ctx);
                Exit_Flag = true;
                break;
            }
        }
    }

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::BOUNDED_TOKEN:

                ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

                {
                    bool Exit_Flag;
                    Exit_Flag = false;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().tokenId) {
                        case Tokenizer::RIGHT_CURLY_TOKEN:
                            Exit_Flag = true;
                            break;

                        default:
                            ctx.tokenStream().ungetToken();
                            localShape = ObjectParser::parseShape(ctx);
                            object->boundingShapes.add(localShape);
                            break;
                        }
                    }
                }
                break;

            case Tokenizer::CLIPPED_TOKEN:

                ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

                {
                    bool Exit_Flag;
                    Exit_Flag = false;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().tokenId) {
                        case Tokenizer::RIGHT_CURLY_TOKEN:
                            Exit_Flag = true;
                            break;

                        default:
                            ctx.tokenStream().ungetToken();
                            localShape = ObjectParser::parseShape(ctx);
                            object->clippingShapes.add(localShape);
                            break;
                        }
                    }
                }
                break;

            case Tokenizer::COLOUR_TOKEN:
                object->objectColor = ModelBuilder::getColor();
                PrimitiveParser::parseColor(object->objectColor, ctx);
                break;

            case Tokenizer::TEXTURE_TOKEN:
                localTexture = TextureParser::parseTexture(ctx);
                if (localTexture->isConstant()) {
                    localTexture = TextureParser::copyTexture(localTexture);
                }

                if (object->objectTexture == MaterialUtils::instance().defaultTexture()) {
                    object->objectTexture = localTexture;
                } else {
                    TextureParser::prependTextureLayers(localTexture, object->objectTexture);
                }
                break;

            case Tokenizer::NO_SHADOW_TOKEN:
                object->noShadowFlag = true;
                break;

            case Tokenizer::LIGHT_SOURCE_TOKEN:
                ParseErrorReporter::reportError(
                    "Light source must be defined using new syntax", ctx);
                break;

            case Tokenizer::TRANSLATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::translate(object, &localVector);
                break;

            case Tokenizer::ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::rotate(object, &localVector);
                break;

            case Tokenizer::SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::scale(object, &localVector);
                break;

            case Tokenizer::INVERSE_TOKEN:
                GeometryOperations::invert(object);
                break;

            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = true;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    return (object);
}

BoundedGeometry *
ObjectParser::parseComposite(ParserContext &ctx)
{
    Composite *localComposite;
    BoundedGeometry *localObject;
    SimpleBody *localShape;
    int constantId;
    Vector3Dd localVector;

    localComposite = nullptr;

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        ParseGlobals::COMPOSITE_CONSTANT) {
                        localComposite = (Composite *)GeometryOperations::copy(
                            (TransformableElement *)ctx.constants()[(int)constantId]
                                .constantData);
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::reportUndeclared(ctx);
                }
                break;

            case Tokenizer::COMPOSITE_TOKEN:
                if (localComposite == nullptr) {
                    localComposite = ModelBuilder::getCompositeObject();
                }

                localObject = ObjectParser::parseComposite(ctx);
                localComposite->simpleBodies.add(localObject);
                break;

            case Tokenizer::OBJECT_TOKEN:
                if (localComposite == nullptr) {
                    localComposite = ModelBuilder::getCompositeObject();
                }
                localObject = ObjectParser::parseObject(ctx);
                localComposite->simpleBodies.add(localObject);
                break;

            case Tokenizer::RIGHT_CURLY_TOKEN:
                ctx.tokenStream().ungetToken();
                if (localComposite == nullptr) {
                    localComposite = ModelBuilder::getCompositeObject();
                }
                Exit_Flag = true;
                break;

            default:
                ctx.tokenStream().ungetToken();
                Exit_Flag = true;
                break;
            }
        }
    }

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = true;
                break;

            case Tokenizer::BOUNDED_TOKEN:

                ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

                {
                    bool Exit_Flag;
                    Exit_Flag = false;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().tokenId) {
                        case Tokenizer::RIGHT_CURLY_TOKEN:
                            Exit_Flag = true;
                            break;

                        default:
                            ctx.tokenStream().ungetToken();
                            localShape = ObjectParser::parseShape(ctx);
                            localComposite->boundingShapes.add(localShape);
                            break;
                        }
                    }
                }
                break;

            case Tokenizer::CLIPPED_TOKEN:

                ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

                {
                    bool Exit_Flag;
                    Exit_Flag = false;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().tokenId) {
                        case Tokenizer::RIGHT_CURLY_TOKEN:
                            Exit_Flag = true;
                            break;

                        default:
                            ctx.tokenStream().ungetToken();
                            localShape = ObjectParser::parseShape(ctx);
                            localComposite->clippingShapes.add(localShape);
                            break;
                        }
                    }
                }
                break;

            case Tokenizer::TRANSLATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::translate(
                    localComposite, &localVector);
                break;

            case Tokenizer::ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::rotate(
                    localComposite, &localVector);
                break;

            case Tokenizer::SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::scale(
                    localComposite, &localVector);
                break;

            case Tokenizer::INVERSE_TOKEN:
                GeometryOperations::invert(localComposite);
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    return ((BoundedGeometry *)localComposite);
}
