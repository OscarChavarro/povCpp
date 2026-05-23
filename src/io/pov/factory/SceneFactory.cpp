#include "io/pov/factory/SceneFactory.h"

#include "environment/scene/factory/ModelFactory.h"

Composite *
SceneFactory::getCompositeObject()
{
    return ModelFactory::getCompositeObject();
}

Sphere *
SceneFactory::getSphereShape()
{
    return ModelFactory::getSphereShape();
}

Light *
SceneFactory::getLightSourceShape()
{
    return ModelFactory::getLightSourceShape();
}

Quadric *
SceneFactory::getQuadricShape()
{
    return ModelFactory::getQuadricShape();
}

PolynomialShape *
SceneFactory::getPolyShape(int order)
{
    return ModelFactory::getPolyShape(order);
}

Box *
SceneFactory::getBoxShape()
{
    return ModelFactory::getBoxShape();
}

Blob *
SceneFactory::getBlobShape()
{
    return ModelFactory::getBlobShape();
}

HeightField *
SceneFactory::getHeightFieldShape()
{
    return ModelFactory::getHeightFieldShape();
}

InfinitePlane *
SceneFactory::getPlaneShape()
{
    return ModelFactory::getPlaneShape();
}

Triangle *
SceneFactory::getTriangleShape()
{
    return ModelFactory::getTriangleShape();
}

SmoothTriangle *
SceneFactory::getSmoothTriangleShape()
{
    return ModelFactory::getSmoothTriangleShape();
}

CSG *
SceneFactory::getCsgShape()
{
    return ModelFactory::getCsgShape();
}

CSG *
SceneFactory::getCsgUnion()
{
    return ModelFactory::getCsgUnion();
}

CSG *
SceneFactory::getCsgIntersection()
{
    return ModelFactory::getCsgIntersection();
}

Viewpoint *
SceneFactory::getViewpoint()
{
    return ModelFactory::getViewpoint();
}

RGBAColor *
SceneFactory::getColour()
{
    return ModelFactory::getColour();
}

Vector3Dd *
SceneFactory::getVector()
{
    return ModelFactory::getVector();
}

double *
SceneFactory::getFloat()
{
    return ModelFactory::getFloat();
}

ParametricBiCubicPatch *
SceneFactory::getBicubicPatchShape()
{
    return ModelFactory::getBicubicPatchShape();
}
