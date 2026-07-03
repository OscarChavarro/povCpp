#include "render/bakedScene/BakedGeometryBaker.h"

// Every formula below is transcribed verbatim from the baseline commit
// 4af1a75 (Quadric::transformQuadric/quadricToMatrix/matrixToQuadric/
// translateGeometry/rotateGeometry/scaleGeometry/invertGeometry and
// InfinitePlane::translateGeometry/rotateGeometry/scaleGeometry/
// invertGeometry) - see doc/performanceReviewPlan5.md Phase 0 appendix.
// Replaying elementary steps one at a time (rather than composing one
// Matrix4x4d and applying a single congruence) is required: the composed-
// matrix variant is a recorded dead end that breaks byte-exact rendering.

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
        switch (step.kind) {
        case TransformStep::Kind::Translate:
        {
            Matrix4x4d transformationInverse = Matrix4x4d().translation(
                0.0 - step.vector.x(), 0.0 - step.vector.y(),
                0.0 - step.vector.z()).transpose();
            baked = transformQuadric(baked, transformationInverse);
            break;
        }
        case TransformStep::Kind::Rotate:
        {
            Matrix4x4d transformation;
            Matrix4x4d transformationInverse;
            Vector3Dd vector = step.vector;
            transformation.axisRotationRodrigues(&transformationInverse, &vector);
            baked = transformQuadric(baked, transformationInverse);
            break;
        }
        case TransformStep::Kind::Scale:
        {
            Matrix4x4d transformationInverse = Matrix4x4d().scale(
                1.0 / step.vector.x(), 1.0 / step.vector.y(), 1.0 / step.vector.z());
            baked = transformQuadric(baked, transformationInverse);
            break;
        }
        case TransformStep::Kind::Invert:
            // Unlike Translate/Rotate/Scale (deferred into a matrix,
            // leaving the parsed Geometry's own coefficients untouched
            // until this replay), `inverse` is applied destructively to
            // the Geometry's coefficients immediately at parse time
            // (SimpleBodyBuilder::invert()/CsgOperand::invert() both call
            // geometry->invertGeometry() directly - see the Phase 0
            // appendix). `original` here already reflects it, so replaying
            // it again would invert twice (a no-op on the sign, silently
            // discarding the actual invert). Skip it.
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
        switch (step.kind) {
        case TransformStep::Kind::Translate:
        {
            Vector3Dd translation = baked.getNormalVector().multiply(step.vector);
            baked.setDistance(
                baked.getDistance() - (translation.x() + translation.y() + translation.z()));
            break;
        }
        case TransformStep::Kind::Rotate:
        {
            Matrix4x4d transformation;
            Matrix4x4d transformationInverse;
            Vector3Dd vector = step.vector;
            transformation.axisRotationRodrigues(&transformationInverse, &vector);
            baked.getNormalVector() =
                transformation.transpose().multiply(baked.getNormalVector());
            break;
        }
        case TransformStep::Kind::Scale:
        {
            Vector3Dd &n = baked.getNormalVector();
            n = Vector3Dd(n.x() / step.vector.x(), n.y() / step.vector.y(),
                n.z() / step.vector.z());
            const double length = n.length();
            n = n.multiply(1.0 / length);
            baked.setDistance(baked.getDistance() / length);
            break;
        }
        case TransformStep::Kind::Invert:
            // Unlike Translate/Rotate/Scale (deferred into a matrix,
            // leaving the parsed Geometry's own coefficients untouched
            // until this replay), `inverse` is applied destructively to
            // the Geometry's coefficients immediately at parse time
            // (SimpleBodyBuilder::invert()/CsgOperand::invert() both call
            // geometry->invertGeometry() directly - see the Phase 0
            // appendix). `original` here already reflects it, so replaying
            // it again would invert twice (a no-op on the sign, silently
            // discarding the actual invert). Skip it.
            break;
        }
    }
    return baked;
}
