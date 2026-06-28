#include "environment/geometry/volume/constructiveSolidGeometry/CsgOperand.h"
#include "environment/geometry/GeometryTransformMutator.h"
#include "environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometry.h"
#include "environment/geometry/volume/polynomial/PolynomialShape.h"

void
CsgOperand::translate(Vector3Dd *vector)
{
    if (dynamic_cast<PolynomialShape *>(geometry) != nullptr ||
        dynamic_cast<ConstructiveSolidGeometry *>(geometry) != nullptr) {
        ensureMatrices();
        Matrix4x4d delta = Matrix4x4d().translation(
            vector->x(), vector->y(), vector->z()).transpose();
        Matrix4x4d deltaInverse = Matrix4x4d().translation(
            0.0 - vector->x(), 0.0 - vector->y(), 0.0 - vector->z()).transpose();
        *transformation = transformation->multiply(delta);
        *transformationInverse = deltaInverse.multiply(*transformationInverse);
    } else if (GeometryTransformMutator::translateIfSupported(geometry, vector)) {
    } else {
        ensureMatrices();
        Matrix4x4d delta = Matrix4x4d().translation(
            vector->x(), vector->y(), vector->z()).transpose();
        Matrix4x4d deltaInverse = Matrix4x4d().translation(
            0.0 - vector->x(), 0.0 - vector->y(), 0.0 - vector->z()).transpose();
        *transformation = transformation->multiply(delta);
        *transformationInverse = deltaInverse.multiply(*transformationInverse);
    }
    if (material != nullptr) {
        material = material->translate(vector);
    }
}

void
CsgOperand::rotate(Vector3Dd *vector)
{
    if (dynamic_cast<PolynomialShape *>(geometry) != nullptr ||
        dynamic_cast<ConstructiveSolidGeometry *>(geometry) != nullptr) {
        ensureMatrices();
        Matrix4x4d delta;
        Matrix4x4d deltaInverse;
        delta.axisRotationRodrigues(&deltaInverse, vector);
        *transformation = transformation->multiply(delta);
        *transformationInverse = deltaInverse.multiply(*transformationInverse);
    } else if (GeometryTransformMutator::rotateIfSupported(geometry, vector)) {
    } else {
        ensureMatrices();
        Matrix4x4d delta;
        Matrix4x4d deltaInverse;
        delta.axisRotationRodrigues(&deltaInverse, vector);
        *transformation = transformation->multiply(delta);
        *transformationInverse = deltaInverse.multiply(*transformationInverse);
    }
    if (material != nullptr) {
        material = material->rotate(vector);
    }
}

void
CsgOperand::scale(Vector3Dd *vector)
{
    if (dynamic_cast<PolynomialShape *>(geometry) != nullptr ||
        dynamic_cast<ConstructiveSolidGeometry *>(geometry) != nullptr) {
        ensureMatrices();
        Matrix4x4d delta = Matrix4x4d().scale(vector->x(), vector->y(), vector->z());
        Matrix4x4d deltaInverse = Matrix4x4d().scale(
            1.0 / vector->x(), 1.0 / vector->y(), 1.0 / vector->z());
        *transformation = transformation->multiply(delta);
        *transformationInverse = deltaInverse.multiply(*transformationInverse);
    } else if (GeometryTransformMutator::scaleIfSupported(geometry, vector)) {
    } else {
        ensureMatrices();
        Matrix4x4d delta = Matrix4x4d().scale(vector->x(), vector->y(), vector->z());
        Matrix4x4d deltaInverse = Matrix4x4d().scale(
            1.0 / vector->x(), 1.0 / vector->y(), 1.0 / vector->z());
        *transformation = transformation->multiply(delta);
        *transformationInverse = deltaInverse.multiply(*transformationInverse);
    }
    if (material != nullptr) {
        material = material->scale(vector);
    }
}
