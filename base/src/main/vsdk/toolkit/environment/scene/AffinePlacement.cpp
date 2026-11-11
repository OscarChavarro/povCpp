#include "java/lang/Double.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/environment/scene/AffinePlacement.h"

AffinePlacement::AffinePlacement() : forward(), inverse(), status(AffinePlacementStatus::Valid) {}
AffinePlacementStatus AffinePlacement::assign(const Matrix4x4d& value, const Matrix4x4d& inverseValue)
{
    forward = value;
    inverse = inverseValue;
    status = validate(forward, inverse);
    return status;
}

AffinePlacementStatus AffinePlacement::validate(const Matrix4x4d& forward, const Matrix4x4d& inverse)
{
    for (int row=0; row<4; ++row) for (int column=0; column<4; ++column) {
        if (!java::Double::isFinite(forward.get(row,column))) return AffinePlacementStatus::Invalid;
        if (!java::Double::isFinite(inverse.get(row,column))) return AffinePlacementStatus::Indeterminate;
    }
    if (forward.get(3,0)!=0 || forward.get(3,1)!=0 || forward.get(3,2)!=0 || forward.get(3,3)!=1 ||
        inverse.get(3,0)!=0 || inverse.get(3,1)!=0 || inverse.get(3,2)!=0 || inverse.get(3,3)!=1)
        return AffinePlacementStatus::Invalid;
    const double a=forward.get(0,0), b=forward.get(0,1), c=forward.get(0,2);
    const double d=forward.get(1,0), e=forward.get(1,1), f=forward.get(1,2);
    const double g=forward.get(2,0), h=forward.get(2,1), i=forward.get(2,2);
    const double determinant=a*(e*i-f*h)-b*(d*i-f*g)+c*(d*h-e*g);
    if (!java::Double::isFinite(determinant)) return AffinePlacementStatus::Indeterminate;
    if (determinant==0) {
        // A visibly zero row or column proves singularity. Otherwise the
        // determinant may have underflowed or suffered cancellation.
        for (int k=0; k<3; ++k) {
            if ((forward.get(k,0)==0 && forward.get(k,1)==0 && forward.get(k,2)==0) ||
                (forward.get(0,k)==0 && forward.get(1,k)==0 && forward.get(2,k)==0))
                return AffinePlacementStatus::Invalid;
        }
        return AffinePlacementStatus::Indeterminate;
    }
    const Matrix4x4d products[] = {forward.multiply(inverse), inverse.multiply(forward)};
    for (int k=0; k<2; ++k) for (int row=0; row<4; ++row) for (int column=0; column<4; ++column) {
        const double actual=products[k].get(row,column), expected=row==column ? 1.0 : 0.0;
        if (!java::Double::isFinite(actual) || java::Math::abs(actual-expected)>1e-8)
            return AffinePlacementStatus::Indeterminate;
    }
    return AffinePlacementStatus::Valid;
}
