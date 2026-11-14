#include "io/pov/geometry/SimpleBodyBuilder.h"
#include "environment/material/povray/PovRayMaterial.h"
#include "environment/scene/constructiveSolidGeometry/ConstructiveSolidGeometry.h"
#include "environment/scene/constructiveSolidGeometry/CsgOperand.h"
#include "environment/scene/SurfaceTraceKernel.h"
#include "environment/scene/ProjectedTriangleTraversal.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMesh.h"
#include "java/lang/IllegalStateException.h"
#include "vsdk/toolkit/environment/light/Light.h"
#include "java/util/ArrayList.txx"

SimpleBodyBuilder::SimpleBodyBuilder(
    Geometry *geometry, Material *material, ColorRgba *shapeColor) :
    geometry(geometry),
    material(material),
    shapeColor(shapeColor)
{
}

SimpleBodyBuilder::SimpleBodyBuilder(ConstructiveSolidGeometry *csg) :
    csg(csg)
{
}

SimpleBodyBuilder::SimpleBodyBuilder(Light *light) :
    light(light)
{
}

SimpleBodyBuilder::~SimpleBodyBuilder()
{
    delete triangleTraversal;
    delete geometry;
    delete csg;
    delete light;
    delete material;
    delete shapeColor;
    delete transformation;
    delete transformationInverse;
}

void
SimpleBodyBuilder::ensureMatrices()
{
    if (transformation == nullptr) {
        transformation = new Matrix4x4d(Matrix4x4d::identityMatrix());
        transformationInverse = new Matrix4x4d(Matrix4x4d::identityMatrix());
    }
}

ColorRgba *
SimpleBodyBuilder::ensureShapeColor()
{
    if (shapeColor == nullptr) {
        shapeColor = new ColorRgba(0.0, 0.0, 0.0, 0.0);
    }
    return shapeColor;
}

Material *
SimpleBodyBuilder::releaseMaterial()
{
    Material *released = material;
    material = nullptr;
    return released;
}

void
SimpleBodyBuilder::prependMaterialLayers(Material *newHead)
{
    if (newHead != nullptr) {
        material = static_cast<PovRayMaterial *>(newHead)->prependMaterialLayers(
            static_cast<PovRayMaterial *>(material));
    }
}

SimpleBodyBuilder::SimpleBodyBuilder(const SimpleBodyBuilder &other) :
    geometry(other.geometry != nullptr ? other.geometry->copyGeometry() : nullptr),
    csg(other.csg != nullptr ? other.csg->copy() : nullptr),
    light(other.light != nullptr ? other.light->copy() : nullptr),
    material(other.material != nullptr ?
        new PovRayMaterial(*static_cast<PovRayMaterial *>(other.material)) : nullptr),
    shapeColor(other.shapeColor != nullptr ? new ColorRgba(*other.shapeColor) : nullptr),
    transformation(other.transformation != nullptr ?
        new Matrix4x4d(*other.transformation) : nullptr),
    transformationInverse(other.transformationInverse != nullptr ?
        new Matrix4x4d(*other.transformationInverse) : nullptr),
    steps(other.steps),
    surfaceInverted(other.surfaceInverted)
{
    if (other.triangleTraversal != nullptr) {
        triangleTraversal = new ProjectedTriangleTraversal(*other.triangleTraversal,
            static_cast<TriangleMesh *>(geometry));
    }
}

ProjectedTriangleTraversal *
SimpleBodyBuilder::enableProjectedTriangleTraversal()
{
    TriangleMesh *mesh = dynamic_cast<TriangleMesh *>(geometry);
    if (mesh == nullptr || triangleTraversal != nullptr)
        throw java::IllegalStateException("Projected traversal needs a valid triangle mesh");
    triangleTraversal = new ProjectedTriangleTraversal(mesh);
    return triangleTraversal;
}

void
SimpleBodyBuilder::translate(Vector3Dd *vector)
{
    ensureMatrices();
    Matrix4x4d delta = Matrix4x4d().translation(
        vector->x(), vector->y(), vector->z()).transpose();
    Matrix4x4d deltaInverse = Matrix4x4d().translation(
        0.0 - vector->x(), 0.0 - vector->y(), 0.0 - vector->z()).transpose();
    *transformation = transformation->multiply(delta);
    *transformationInverse = deltaInverse.multiply(*transformationInverse);
    steps.add(TransformStep(TransformStep::Kind::Translate, *vector));
    if (getMaterial() != nullptr) {
        material = getMaterial()->translate(vector);
    }
}

void
SimpleBodyBuilder::translateOwnerOnly(Vector3Dd *vector)
{
    ensureMatrices();
    Matrix4x4d delta = Matrix4x4d().translation(
        vector->x(), vector->y(), vector->z()).transpose();
    Matrix4x4d deltaInverse = Matrix4x4d().translation(
        0.0 - vector->x(), 0.0 - vector->y(), 0.0 - vector->z()).transpose();
    *transformation = transformation->multiply(delta);
    *transformationInverse = deltaInverse.multiply(*transformationInverse);
    steps.add(TransformStep(TransformStep::Kind::Translate, *vector));
}

void
SimpleBodyBuilder::rotate(Vector3Dd *vector)
{
    ensureMatrices();
    Matrix4x4d delta;
    Matrix4x4d deltaInverse;
    delta.axisRotationRodrigues(&deltaInverse, vector);
    *transformation = transformation->multiply(delta);
    *transformationInverse = deltaInverse.multiply(*transformationInverse);
    steps.add(TransformStep(TransformStep::Kind::Rotate, *vector));
    if (getMaterial() != nullptr) {
        material = getMaterial()->rotate(vector);
    }
}

void
SimpleBodyBuilder::rotateOwnerOnly(Vector3Dd *vector)
{
    ensureMatrices();
    Matrix4x4d delta;
    Matrix4x4d deltaInverse;
    delta.axisRotationRodrigues(&deltaInverse, vector);
    *transformation = transformation->multiply(delta);
    *transformationInverse = deltaInverse.multiply(*transformationInverse);
    steps.add(TransformStep(TransformStep::Kind::Rotate, *vector));
}

void
SimpleBodyBuilder::scale(Vector3Dd *vector)
{
    ensureMatrices();
    Matrix4x4d delta = Matrix4x4d().scale(
        vector->x(), vector->y(), vector->z()).transpose();
    Matrix4x4d deltaInverse = Matrix4x4d().scale(
        1.0 / vector->x(), 1.0 / vector->y(), 1.0 / vector->z()).transpose();
    *transformation = transformation->multiply(delta);
    *transformationInverse = deltaInverse.multiply(*transformationInverse);
    steps.add(TransformStep(TransformStep::Kind::Scale, *vector));
    if (getMaterial() != nullptr) {
        material = getMaterial()->scale(vector);
    }
}

void
SimpleBodyBuilder::scaleOwnerOnly(Vector3Dd *vector)
{
    ensureMatrices();
    Matrix4x4d delta = Matrix4x4d().scale(
        vector->x(), vector->y(), vector->z()).transpose();
    Matrix4x4d deltaInverse = Matrix4x4d().scale(
        1.0 / vector->x(), 1.0 / vector->y(), 1.0 / vector->z()).transpose();
    *transformation = transformation->multiply(delta);
    *transformationInverse = deltaInverse.multiply(*transformationInverse);
    steps.add(TransformStep(TransformStep::Kind::Scale, *vector));
}

void
SimpleBodyBuilder::invert()
{
    // A light has nothing to invert.
    if (csg != nullptr) {
        csg->invert();
    } else if (triangleTraversal != nullptr) {
        triangleTraversal->invert();
    } else if (geometry != nullptr) {
        const SurfaceDetailFamily family = SurfaceTraceKernel::familyOf(geometry);
        if (SurfaceTraceKernel::invertsByState(family)) {
            surfaceInverted = !surfaceInverted;
        } else {
            SurfaceTraceKernel::invertGeometry(geometry, family);
        }
    }
    steps.add(TransformStep(TransformStep::Kind::Invert, Vector3Dd(0.0, 0.0, 0.0)));
}

PovrayBody *
SimpleBodyBuilder::buildBody(Material *objectTexture, PovrayBody::MaterialReleaser releaser)
{
    java::ArrayList<PovrayBody*> noRegions(0);
    java::ArrayList<TransformStep> noSteps;
    PovrayBody *body;
    if (csg != nullptr) {
        body = new PovrayBody(
            nullptr, material, objectTexture, nullptr, false, noRegions, noRegions,
            transformation, transformationInverse, nullptr, nullptr,
            steps, noSteps, releaser);
        body->setCsg(csg);
        csg = nullptr;
    } else {
        body = new PovrayBody(
            geometry, material, objectTexture, nullptr, false, noRegions, noRegions,
            nullptr, nullptr, transformation, transformationInverse,
            noSteps, steps, releaser);
    }
    if (light != nullptr) body->setLight(light);
    light = nullptr;
    if (triangleTraversal != nullptr) body->setTriangleTraversal(triangleTraversal);
    body->setSurfaceInverted(surfaceInverted);
    triangleTraversal = nullptr;
    surfaceInverted = false;
    geometry = nullptr;
    material = nullptr;
    transformation = nullptr;
    transformationInverse = nullptr;
    steps.clear();
    return body;
}

PovrayBody *
SimpleBodyBuilder::buildRegion()
{
    java::ArrayList<PovrayBody*> noRegions(0);
    PovrayBody *region = new PovrayBody(
        geometry, nullptr, nullptr, nullptr, false, noRegions, noRegions,
        nullptr, nullptr, transformation, transformationInverse);
    if (csg != nullptr) region->setCsg(csg);
    csg = nullptr;
    if (light != nullptr) region->setLight(light);
    light = nullptr;
    if (triangleTraversal != nullptr) region->setTriangleTraversal(triangleTraversal);
    region->setSurfaceInverted(surfaceInverted);
    triangleTraversal = nullptr;
    surfaceInverted = false;
    geometry = nullptr;
    transformation = nullptr;
    transformationInverse = nullptr;
    steps.clear();
    return region;
}

void
SimpleBodyBuilder::addToCsg(ConstructiveSolidGeometry *container)
{
    CsgOperand *operand =
        new CsgOperand(geometry, material, transformation, transformationInverse, steps);
    if (csg != nullptr) operand->setCsg(csg);
    csg = nullptr;
    if (light != nullptr) operand->setLight(light);
    light = nullptr;
    if (triangleTraversal != nullptr) operand->setTriangleTraversal(triangleTraversal);
    operand->setSurfaceInverted(surfaceInverted);
    triangleTraversal = nullptr;
    surfaceInverted = false;
    container->addOperand(operand);
    geometry = nullptr;
    material = nullptr;
    transformation = nullptr;
    transformationInverse = nullptr;
    steps.clear();
}
