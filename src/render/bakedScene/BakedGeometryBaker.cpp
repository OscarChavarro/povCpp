#include "render/bakedScene/BakedGeometryBaker.h"

void
BakedGeometryBaker::quadricToMatrix(const Quadric &quadric, Matrix4x4d *matrix)
{
    *matrix = Matrix4x4d::identityMatrix().multiply(0.0);
    *matrix = matrix->withVal(0, 0, quadric.getObject2Terms().x());
    *matrix = matrix->withVal(1, 1, quadric.getObject2Terms().y());
    *matrix = matrix->withVal(2, 2, quadric.getObject2Terms().z());
    *matrix = matrix->withVal(0, 1, quadric.getObjectMixedTerms().x());
    *matrix = matrix->withVal(0, 2, quadric.getObjectMixedTerms().y());
    *matrix = matrix->withVal(0, 3, quadric.getObjectTerms().x());
    *matrix = matrix->withVal(1, 2, quadric.getObjectMixedTerms().z());
    *matrix = matrix->withVal(1, 3, quadric.getObjectTerms().y());
    *matrix = matrix->withVal(2, 3, quadric.getObjectTerms().z());
    *matrix = matrix->withVal(3, 3, quadric.getObjectConstant());
}

Quadric
BakedGeometryBaker::matrixToQuadric(const Matrix4x4d &matrix)
{
    Vector3Dd object2Terms(matrix.get(0, 0), matrix.get(1, 1), matrix.get(2, 2));
    Vector3Dd objectMixedTerms(
        matrix.get(0, 1) + matrix.get(1, 0),
        matrix.get(0, 2) + matrix.get(2, 0),
        matrix.get(1, 2) + matrix.get(2, 1));
    Vector3Dd objectTerms(
        matrix.get(0, 3) + matrix.get(3, 0),
        matrix.get(1, 3) + matrix.get(3, 1),
        matrix.get(2, 3) + matrix.get(3, 2));
    double objectConstant = matrix.get(3, 3);
    // The 4-arg constructor calls updateSquareTermFlag() itself.
    return Quadric(object2Terms, objectMixedTerms, objectTerms, objectConstant);
}

Quadric
BakedGeometryBaker::transformQuadric(
    const Quadric &shape, const Matrix4x4d &transformationInverse)
{
    Matrix4x4d quadricMatrix;
    quadricToMatrix(shape, &quadricMatrix);
    quadricMatrix = transformationInverse.multiply(quadricMatrix);
    Matrix4x4d transformTransposed = transformationInverse.transpose();
    quadricMatrix = quadricMatrix.multiply(transformTransposed);
    return matrixToQuadric(quadricMatrix);
}

Quadric
BakedGeometryBaker::bakeQuadric(
    const Quadric &original, const java::ArrayList<TransformStep> &steps)
{
    Quadric baked(original);
    for (long int i = 0; i < steps.size(); i++) {
        const TransformStep &step = steps[i];
        switch (step.getKind()) {
        case TransformStep::Kind::Translate:
        {
            Matrix4x4d transformationInverse = Matrix4x4d().translation(
                0.0 - step.getVector().x(), 0.0 - step.getVector().y(),
                0.0 - step.getVector().z()).transpose();
            baked = transformQuadric(baked, transformationInverse);
            break;
        }
        case TransformStep::Kind::Rotate:
        {
            Matrix4x4d transformation;
            Matrix4x4d transformationInverse;
            Vector3Dd vector = step.getVector();
            transformation.axisRotationRodrigues(&transformationInverse, &vector);
            baked = transformQuadric(baked, transformationInverse);
            break;
        }
        case TransformStep::Kind::Scale:
        {
            Matrix4x4d transformationInverse = Matrix4x4d().scale(
                1.0 / step.getVector().x(), 1.0 / step.getVector().y(), 1.0 / step.getVector().z());
            baked = transformQuadric(baked, transformationInverse);
            break;
        }
        case TransformStep::Kind::Invert:
            break;
        }
    }
    return baked;
}

InfinitePlane
BakedGeometryBaker::bakePlane(
    const InfinitePlane &original, const java::ArrayList<TransformStep> &steps)
{
    InfinitePlane baked(original);
    for (long int i = 0; i < steps.size(); i++) {
        const TransformStep &step = steps[i];
        switch (step.getKind()) {
        case TransformStep::Kind::Translate:
        {
            Vector3Dd translation = baked.getNormalVector().multiply(step.getVector());
            baked.setDistance(
                baked.getDistance() - (translation.x() + translation.y() + translation.z()));
            break;
        }
        case TransformStep::Kind::Rotate:
        {
            Matrix4x4d transformation;
            Matrix4x4d transformationInverse;
            Vector3Dd vector = step.getVector();
            transformation.axisRotationRodrigues(&transformationInverse, &vector);
            baked.getNormalVector() =
                transformation.transpose().multiply(baked.getNormalVector());
            break;
        }
        case TransformStep::Kind::Scale:
        {
            Vector3Dd &n = baked.getNormalVector();
            n = Vector3Dd(n.x() / step.getVector().x(), n.y() / step.getVector().y(),
                n.z() / step.getVector().z());
            const double length = n.length();
            n = n.multiply(1.0 / length);
            baked.setDistance(baked.getDistance() / length);
            break;
        }
        case TransformStep::Kind::Invert:
            break;
        }
    }
    return baked;
}
