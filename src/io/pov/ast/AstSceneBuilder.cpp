#include "io/pov/ast/AstSceneBuilder.h"

#include <cmath>

#include "common/color/Color.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/volume/Sphere.h"
#include "environment/geometry/volume/compound/CSG.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "environment/light/Light.h"
#include "environment/scene/ModelBuilder.h"
#include "environment/scene/SceneFrame.h"
#include "environment/scene/SimpleBodyFactory.h"
#include "io/pov/ParseErrorReporter.h"
#include "io/pov/ParserConstants.h"
#include "io/pov/ParserContext.h"
#include "io/pov/ast/AstNodes.h"

struct AstDeclTable {
    int count;
    int ids[MAX_CONSTANTS];
    const AstNode *nodes[MAX_CONSTANTS];
};

static Vector3Dd
asVector(const AstVector3 &v)
{
    return Vector3Dd(v.x, v.y, v.z);
}

static void
applyTransforms(Geometry *shape, const AstTransform *transforms, int count)
{
    for (int i = 0; i < count; ++i) {
        const AstTransform &t = transforms[i];
        Vector3Dd v = asVector(t.vectorValue);
        switch (t.kind) {
        case AST_TRANSLATE:
            GeometryOperations::translate((SimpleBody *)shape, &v);
            break;
        case AST_ROTATE:
            GeometryOperations::rotate((SimpleBody *)shape, &v);
            break;
        case AST_SCALE:
            GeometryOperations::scale((SimpleBody *)shape, &v);
            break;
        case AST_INVERSE:
            GeometryOperations::invert((SimpleBody *)shape);
            break;
        default:
            break;
        }
    }
}

static const AstNode *
findDecl(const AstDeclTable &decls, int identifierNumber)
{
    for (int i = 0; i < decls.count; ++i) {
        if (decls.ids[i] == identifierNumber) {
            return decls.nodes[i];
        }
    }
    return nullptr;
}

static Geometry *buildGeometryNode(
    const AstNode &node, ParserContext &ctx, const AstDeclTable &decls);
static SimpleBody *buildSimpleBodyNode(
    const AstNode &node, ParserContext &ctx, const AstDeclTable &decls);

static Sphere *
buildSphere(const AstSphereNode &node, ParserContext &ctx, const AstDeclTable &decls)
{
    Sphere *shape = nullptr;

    if (node.hasReference) {
        const AstNode *declNode = findDecl(decls, node.referenceConstantId);
        if (declNode == nullptr || declNode->kind != AST_SPHERE_NODE) {
            ParseErrorReporter::Error("Invalid sphere reference in AST", ctx);
        }
        Sphere *declShape = buildSphere((const AstSphereNode &)*declNode, ctx, decls);
        shape = (Sphere *)GeometryOperations::copy((SimpleBody *)declShape);
        delete declShape;
    } else {
        shape = ModelBuilder::getSphereShape();
        if (node.hasInlineData) {
            shape->Center = asVector(node.center);
            shape->Radius = node.radius;
            shape->radiusSquared = shape->Radius * shape->Radius;
            shape->inverseRadius = 1.0 / shape->Radius;
        }
    }

    if (node.hasColour) {
        shape->Shape_Colour = ModelBuilder::getColour();
        shape->Shape_Colour->Red = node.colour.r;
        shape->Shape_Colour->Green = node.colour.g;
        shape->Shape_Colour->Blue = node.colour.b;
        shape->Shape_Colour->Alpha = node.colour.a;
    }

    applyTransforms((Geometry *)shape, node.transforms, node.transformCount);
    return shape;
}

static Light *
buildLight(
    const AstLightSourceNode &node, ParserContext &ctx, const AstDeclTable &decls)
{
    Light *shape = nullptr;

    if (node.hasReference) {
        const AstNode *declNode = findDecl(decls, node.referenceConstantId);
        if (declNode == nullptr || declNode->kind != AST_LIGHT_SOURCE_NODE) {
            ParseErrorReporter::Error("Invalid light reference in AST", ctx);
        }
        Light *declShape =
            buildLight((const AstLightSourceNode &)*declNode, ctx, decls);
        shape = (Light *)GeometryOperations::copy((SimpleBody *)declShape);
        delete declShape;
    } else {
        shape = ModelBuilder::getLightSourceShape();
        if (node.hasInlineData) {
            shape->Center = asVector(node.center);
            if (shape->Shape_Colour == nullptr) {
                shape->Shape_Colour = ModelBuilder::getColour();
            }
            Color::makeColor(
                shape->Shape_Colour, node.colour.r, node.colour.g, node.colour.b);
            shape->Shape_Colour->Alpha = node.colour.a;
        }
    }

    shape->pointsAt = asVector(node.pointsAt);
    shape->Coeff = node.tightness;
    if (node.radiusDegrees > 0.0) {
        shape->Radius = cos(node.radiusDegrees * M_PI / 180.0);
    }
    if (node.falloffDegrees > 0.0) {
        shape->Falloff = cos(node.falloffDegrees * M_PI / 180.0);
    }
    if (node.hasSpotlight) {
        shape->Type = SPOT_LIGHT_TYPE;
    }

    applyTransforms((Geometry *)shape, node.transforms, node.transformCount);
    return shape;
}

static CSG *
buildCsg(const AstCsgNode &node, ParserContext &ctx, const AstDeclTable &decls)
{
    CSG *container = nullptr;
    if (node.op == AST_CSG_UNION) {
        container = ModelBuilder::getCsgUnion();
    } else {
        container = ModelBuilder::getCsgIntersection();
    }

    if (node.hasReference) {
        const AstNode *declNode = findDecl(decls, node.referenceConstantId);
        if (declNode == nullptr || declNode->kind != AST_CSG_NODE) {
            ParseErrorReporter::Error("Invalid CSG reference in AST", ctx);
        }
        CSG *declShape = buildCsg((const AstCsgNode &)*declNode, ctx, decls);
        delete container;
        container = (CSG *)GeometryOperations::copy((SimpleBody *)declShape);
        delete declShape;
    } else {
        int firstShapeParsed = FALSE;
        for (int i = 0; i < node.childCount; ++i) {
            Geometry *child = buildGeometryNode(*node.children[i], ctx, decls);
            if (node.op == AST_CSG_DIFFERENCE && firstShapeParsed) {
                GeometryOperations::invert((SimpleBody *)child);
            }
            firstShapeParsed = TRUE;
            SimpleBodyFactory::link((SimpleBody *)child,
                (SimpleBody **)&(child->nextObject),
                (SimpleBody **)&(container->Shapes));
        }
    }

    applyTransforms((Geometry *)container, node.transforms, node.transformCount);
    return container;
}

static Geometry *
buildGeometryNode(const AstNode &node, ParserContext &ctx, const AstDeclTable &decls)
{
    if (node.kind == AST_SPHERE_NODE) {
        return (Geometry *)buildSphere((const AstSphereNode &)node, ctx, decls);
    }
    if (node.kind == AST_LIGHT_SOURCE_NODE) {
        return (Geometry *)buildLight((const AstLightSourceNode &)node, ctx, decls);
    }
    if (node.kind == AST_CSG_NODE) {
        return (Geometry *)buildCsg((const AstCsgNode &)node, ctx, decls);
    }

    ParseErrorReporter::Error("Unsupported AST geometry node", ctx);
    return nullptr;
}

static SimpleBody *
buildObject(const AstObjectNode &node, ParserContext &ctx, const AstDeclTable &decls)
{
    SimpleBody *object = nullptr;
    if (node.hasReference) {
        const AstNode *declNode = findDecl(decls, node.referenceConstantId);
        if (declNode == nullptr ||
            (declNode->kind != AST_OBJECT_NODE && declNode->kind != AST_COMPOSITE_NODE)) {
            ParseErrorReporter::Error("Invalid object reference in AST", ctx);
        }
        object =
            (SimpleBody *)GeometryOperations::copy(buildSimpleBodyNode(*declNode, ctx, decls));
    } else {
        object = SimpleBodyFactory::getObject();
        if (node.shape != nullptr) {
            Geometry *shape = buildGeometryNode(*node.shape, ctx, decls);
            SimpleBodyFactory::link((SimpleBody *)shape,
                (SimpleBody **)&(shape->nextObject), (SimpleBody **)&(object->Shape));
        }
    }

    for (int i = 0; i < node.boundedByCount; ++i) {
        Geometry *shape = buildGeometryNode(*node.boundedBy[i], ctx, decls);
        SimpleBodyFactory::link((SimpleBody *)shape,
            (SimpleBody **)&(shape->nextObject), (SimpleBody **)&(object->boundingShapes));
    }
    for (int i = 0; i < node.clippedByCount; ++i) {
        Geometry *shape = buildGeometryNode(*node.clippedBy[i], ctx, decls);
        SimpleBodyFactory::link((SimpleBody *)shape,
            (SimpleBody **)&(shape->nextObject), (SimpleBody **)&(object->clippingShapes));
    }
    applyTransforms((Geometry *)object, node.transforms, node.transformCount);
    if (node.noShadow) {
        object->noShadowFlag = TRUE;
    }
    return object;
}

static Composite *
buildComposite(
    const AstCompositeNode &node, ParserContext &ctx, const AstDeclTable &decls)
{
    Composite *composite = nullptr;
    if (node.hasReference) {
        const AstNode *declNode = findDecl(decls, node.referenceConstantId);
        if (declNode == nullptr || declNode->kind != AST_COMPOSITE_NODE) {
            ParseErrorReporter::Error("Invalid composite reference in AST", ctx);
        }
        composite =
            (Composite *)GeometryOperations::copy(buildSimpleBodyNode(*declNode, ctx, decls));
    } else {
        composite = ModelBuilder::getCompositeObject();
        for (int i = 0; i < node.childCount; ++i) {
            SimpleBody *child = buildSimpleBodyNode(*node.children[i], ctx, decls);
            SimpleBodyFactory::link(
                child, &(child->nextObject), (SimpleBody **)&(composite->Objects));
        }
    }

    for (int i = 0; i < node.boundedByCount; ++i) {
        Geometry *shape = buildGeometryNode(*node.boundedBy[i], ctx, decls);
        SimpleBodyFactory::link((SimpleBody *)shape, (SimpleBody **)&(shape->nextObject),
            (SimpleBody **)&(composite->boundingShapes));
    }
    for (int i = 0; i < node.clippedByCount; ++i) {
        Geometry *shape = buildGeometryNode(*node.clippedBy[i], ctx, decls);
        SimpleBodyFactory::link((SimpleBody *)shape, (SimpleBody **)&(shape->nextObject),
            (SimpleBody **)&(composite->clippingShapes));
    }
    applyTransforms((Geometry *)composite, node.transforms, node.transformCount);
    return composite;
}

static SimpleBody *
buildSimpleBodyNode(const AstNode &node, ParserContext &ctx, const AstDeclTable &decls)
{
    if (node.kind == AST_OBJECT_NODE) {
        return buildObject((const AstObjectNode &)node, ctx, decls);
    }
    if (node.kind == AST_COMPOSITE_NODE) {
        return (SimpleBody *)buildComposite((const AstCompositeNode &)node, ctx, decls);
    }
    if (node.kind == AST_SPHERE_NODE || node.kind == AST_LIGHT_SOURCE_NODE ||
        node.kind == AST_CSG_NODE) {
        return (SimpleBody *)buildGeometryNode(node, ctx, decls);
    }
    ParseErrorReporter::Error("Unsupported AST simple body node", ctx);
    return nullptr;
}

void
AstSceneBuilder::build(const AstScene &scene, RenderFrame *framePtr, ParserContext &ctx)
{
    AstDeclTable declarations;
    declarations.count = 0;

    for (int i = 0; i < scene.nodeCount; ++i) {
        const AstNode *node = scene.nodes[i];
        if (node->kind == AST_DECLARE_NODE) {
            const AstDeclareNode *decl = (const AstDeclareNode *)node;
            if (declarations.count < MAX_CONSTANTS) {
                declarations.ids[declarations.count] = decl->identifierNumber;
                declarations.nodes[declarations.count] = decl->value;
                declarations.count++;
            }
            continue;
        }
        if (node->kind == AST_SPHERE_NODE) {
            Sphere *sphere = buildSphere(*(const AstSphereNode *)node, ctx, declarations);
            SimpleBodyFactory::link((SimpleBody *)sphere,
                (SimpleBody **)&(sphere->nextObject),
                (SimpleBody **)&(framePtr->Objects));
        } else if (node->kind == AST_LIGHT_SOURCE_NODE) {
            Light *light = buildLight(*(const AstLightSourceNode *)node, ctx, declarations);
            light->Next_Light_Source = framePtr->Light_Sources;
            framePtr->Light_Sources = light;
        } else if (node->kind == AST_CSG_NODE) {
            CSG *csg = buildCsg(*(const AstCsgNode *)node, ctx, declarations);
            SimpleBodyFactory::link((SimpleBody *)csg,
                (SimpleBody **)&(csg->nextObject),
                (SimpleBody **)&(framePtr->Objects));
        } else if (node->kind == AST_OBJECT_NODE || node->kind == AST_COMPOSITE_NODE) {
            SimpleBody *obj = buildSimpleBodyNode(*node, ctx, declarations);
            SimpleBodyFactory::link(
                obj, &(obj->nextObject), (SimpleBody **)&(framePtr->Objects));
        }
    }
}
