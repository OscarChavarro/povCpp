#include "environment/geometry/GeometryTransformMutator.h"

#include "environment/geometry/Geometry.h"
#include "environment/geometry/surface/parametric/ParametricBiCubicPatch.h"
#include "environment/geometry/volume/polynomial/PolynomialShape.h"

bool
GeometryTransformMutator::translateIfSupported(Geometry *geometry, Vector3Dd *vector)
{
    if (PolynomialShape *polynomial = dynamic_cast<PolynomialShape *>(geometry)) {
        polynomial->translateGeometry(vector);
        return true;
    }
    if (ParametricBiCubicPatch *patch = dynamic_cast<ParametricBiCubicPatch *>(geometry)) {
        patch->translateGeometry(vector);
        return true;
    }
    return false;
}

bool
GeometryTransformMutator::rotateIfSupported(Geometry *geometry, Vector3Dd *vector)
{
    if (PolynomialShape *polynomial = dynamic_cast<PolynomialShape *>(geometry)) {
        polynomial->rotateGeometry(vector);
        return true;
    }
    if (ParametricBiCubicPatch *patch = dynamic_cast<ParametricBiCubicPatch *>(geometry)) {
        patch->rotateGeometry(vector);
        return true;
    }
    return false;
}

bool
GeometryTransformMutator::scaleIfSupported(Geometry *geometry, Vector3Dd *vector)
{
    if (PolynomialShape *polynomial = dynamic_cast<PolynomialShape *>(geometry)) {
        polynomial->scaleGeometry(vector);
        return true;
    }
    if (ParametricBiCubicPatch *patch = dynamic_cast<ParametricBiCubicPatch *>(geometry)) {
        patch->scaleGeometry(vector);
        return true;
    }
    return false;
}
