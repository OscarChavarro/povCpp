#include "render/bakedScene/BakedGeometryBaker.h"

void
BakedGeometryBaker::quadricToMatrix(const QuadricSurface &quadric, Matrix4x4d *matrix)
{
    *matrix = Matrix4x4d::identityMatrix().multiply(0.0);
    *matrix = matrix->withVal(0, 0, quadric.squareCoefficients().x());
    *matrix = matrix->withVal(1, 1, quadric.squareCoefficients().y());
    *matrix = matrix->withVal(2, 2, quadric.squareCoefficients().z());
    *matrix = matrix->withVal(0, 1, quadric.mixedCoefficients().x());
    *matrix = matrix->withVal(0, 2, quadric.mixedCoefficients().y());
    *matrix = matrix->withVal(0, 3, quadric.linearCoefficients().x());
    *matrix = matrix->withVal(1, 2, quadric.mixedCoefficients().z());
    *matrix = matrix->withVal(1, 3, quadric.linearCoefficients().y());
    *matrix = matrix->withVal(2, 3, quadric.linearCoefficients().z());
    *matrix = matrix->withVal(3, 3, quadric.constantCoefficient());
}

QuadricSurface
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
    return QuadricSurface(object2Terms, objectMixedTerms, objectTerms, objectConstant);
}

QuadricSurface
BakedGeometryBaker::transformQuadric(
    const QuadricSurface &shape, const Matrix4x4d &transformationInverse)
{
    Matrix4x4d quadricMatrix;
    quadricToMatrix(shape, &quadricMatrix);
    quadricMatrix = transformationInverse.multiply(quadricMatrix);
    Matrix4x4d transformTransposed = transformationInverse.transpose();
    quadricMatrix = quadricMatrix.multiply(transformTransposed);
    return matrixToQuadric(quadricMatrix);
}

QuadricSurface
BakedGeometryBaker::bakeQuadric(
    const QuadricSurface &original, const java::ArrayList<TransformStep> &steps)
{
    QuadricSurface baked(original);
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

PlanarHalfSpace
BakedGeometryBaker::bakePlane(
    const PlanarHalfSpace &original, const java::ArrayList<TransformStep> &steps)
{
    Vector3Dd normal = original.getBoundaryPlane().getCoefficients();
    double distance = original.getBoundaryPlane().getConstantCoefficient();
    for (long int i = 0; i < steps.size(); i++) {
        const TransformStep &step = steps[i];
        switch (step.getKind()) {
        case TransformStep::Kind::Translate:
        {
            Vector3Dd translation = normal.multiply(step.getVector());
            distance = distance - (translation.x() + translation.y() + translation.z());
            break;
        }
        case TransformStep::Kind::Rotate:
        {
            Matrix4x4d transformation;
            Matrix4x4d transformationInverse;
            Vector3Dd vector = step.getVector();
            transformation.axisRotationRodrigues(&transformationInverse, &vector);
            normal = transformation.transpose().multiply(normal);
            break;
        }
        case TransformStep::Kind::Scale:
        {
            Vector3Dd n = Vector3Dd(normal.x() / step.getVector().x(),
                normal.y() / step.getVector().y(), normal.z() / step.getVector().z());
            const double length = n.length();
            normal = n.multiply(1.0 / length);
            distance = distance / length;
            break;
        }
        case TransformStep::Kind::Invert:
            break;
        }
    }
    return PlanarHalfSpace(normal.x(), normal.y(), normal.z(), distance);
}
