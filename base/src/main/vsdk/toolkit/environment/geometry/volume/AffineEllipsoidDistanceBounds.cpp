#include "vsdk/toolkit/environment/geometry/volume/AffineEllipsoidDistanceBounds.h"
#include "vsdk/toolkit/environment/geometry/volume/QuadraticInteriorBound.h"
#include "vsdk/toolkit/common/numerics/FloatingInterval.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "java/lang/Double.h"
#include "java/lang/Math.h"

GeometryDistanceStatus AffineEllipsoidDistanceBounds::query(const Vector3Dd& point,
    double radius, const Matrix4x4d& forward, const Vector3Dd& localDirection,
    GeometryContainment& side, double& lower, double& upper, bool refineInterior)
{
    if (!java::Double::isFinite(radius) || !(radius>0) ||
        !java::Double::isFinite(point.x()) || !java::Double::isFinite(point.y()) ||
        !java::Double::isFinite(point.z()) || !java::Double::isFinite(localDirection.x()) ||
        !java::Double::isFinite(localDirection.y()) || !java::Double::isFinite(localDirection.z()))
        return GeometryDistanceStatus::InvalidInput;
    for (int i=0;i<4;++i) for (int j=0;j<4;++j)
        if (!java::Double::isFinite(forward.get(i,j))) return GeometryDistanceStatus::InvalidInput;
    if (forward.get(3,0)!=0 || forward.get(3,1)!=0 || forward.get(3,2)!=0 || forward.get(3,3)!=1 ||
        (localDirection.x()==0 && localDirection.y()==0 && localDirection.z()==0))
        return GeometryDistanceStatus::InvalidInput;
    if (!FloatingInterval::isSupportedEnvironment()) return GeometryDistanceStatus::Unsupported;
    for (int i=0;i<3;++i) {
        if ((forward.get(i,0)==0 && forward.get(i,1)==0 && forward.get(i,2)==0) ||
            (forward.get(0,i)==0 && forward.get(1,i)==0 && forward.get(2,i)==0))
            return GeometryDistanceStatus::InvalidInput;
    }
    const FloatingInterval a=FloatingInterval::exact(forward.get(0,0));
    const FloatingInterval b=FloatingInterval::exact(forward.get(0,1));
    const FloatingInterval c=FloatingInterval::exact(forward.get(0,2));
    const FloatingInterval d0=FloatingInterval::exact(forward.get(1,0));
    const FloatingInterval e=FloatingInterval::exact(forward.get(1,1));
    const FloatingInterval f=FloatingInterval::exact(forward.get(1,2));
    const FloatingInterval g=FloatingInterval::exact(forward.get(2,0));
    const FloatingInterval h=FloatingInterval::exact(forward.get(2,1));
    const FloatingInterval i=FloatingInterval::exact(forward.get(2,2));
    const FloatingInterval determinant=a.multiply(e.multiply(i).subtract(f.multiply(h)))
        .subtract(b.multiply(d0.multiply(i).subtract(f.multiply(g))))
        .add(c.multiply(d0.multiply(h).subtract(e.multiply(g))));
    if (!determinant.isValid() || (determinant.getLower()<=0 && determinant.getUpper()>=0))
        return GeometryDistanceStatus::Indeterminate;
    const double p[3]={point.x(),point.y(),point.z()};
    const double d[3]={localDirection.x(),localDirection.y(),localDirection.z()};
    FloatingInterval lengthSquared=FloatingInterval::exact(0);
    for (int i=0;i<3;++i) lengthSquared=lengthSquared.add(FloatingInterval::exact(d[i]).square());
    const FloatingInterval length=lengthSquared.squareRoot();
    if (!length.isValid() || !(length.getLower()>0)) return GeometryDistanceStatus::Indeterminate;
    FloatingInterval unit[3];
    for (int i=0;i<3;++i) unit[i]=FloatingInterval::exact(d[i]).divide(length)
        .multiply(FloatingInterval::exact(radius));
    FloatingInterval distanceSquared=FloatingInterval::exact(0);
    for (int i=0;i<3;++i) {
        FloatingInterval coordinate=FloatingInterval::exact(forward.get(i,3));
        for (int j=0;j<3;++j) coordinate=coordinate.add(FloatingInterval::exact(forward.get(i,j)).multiply(unit[j]));
        const FloatingInterval delta=coordinate.subtract(FloatingInterval::exact(p[i]));
        distanceSquared=distanceSquared.add(delta.square());
    }
    const FloatingInterval distance=distanceSquared.squareRoot();
    if (!distance.isValid()) return GeometryDistanceStatus::Indeterminate;
    // Any finite nonzero vector n defines a supporting plane. Its distance
    // from the query is a lower bound whenever its signed gap is positive.
    double normals[2][3]={{0,0,0},{0,0,0}};
    bool usable[2]={false,false};
    for (int i=0;i<3;++i) {
        normals[0][i]=p[i]-forward.get(i,3);
        if (!java::Double::isFinite(normals[0][i])) return GeometryDistanceStatus::Indeterminate;
        if (normals[0][i]!=0) usable[0]=true;
    }
    double lowerBound=0;
    GeometryContainment certifiedSide=GeometryContainment::Indeterminate;
    // Enclose A^-1 from cofactors of the stored forward matrix itself; the
    // independently stored approximate inverse is not treated as exact.
    FloatingInterval inverse[3][3];
    inverse[0][0]=e.multiply(i).subtract(f.multiply(h)).divide(determinant);
    inverse[0][1]=c.multiply(h).subtract(b.multiply(i)).divide(determinant);
    inverse[0][2]=b.multiply(f).subtract(c.multiply(e)).divide(determinant);
    inverse[1][0]=f.multiply(g).subtract(d0.multiply(i)).divide(determinant);
    inverse[1][1]=a.multiply(i).subtract(c.multiply(g)).divide(determinant);
    inverse[1][2]=c.multiply(d0).subtract(a.multiply(f)).divide(determinant);
    inverse[2][0]=d0.multiply(h).subtract(e.multiply(g)).divide(determinant);
    inverse[2][1]=b.multiply(g).subtract(a.multiply(h)).divide(determinant);
    inverse[2][2]=a.multiply(e).subtract(b.multiply(d0)).divide(determinant);
    bool inverseCandidateValid=true;
    for (int row=0;row<3;++row) for (int col=0;col<3;++col) {
        if (!inverse[row][col].isValid()) { inverseCandidateValid=false; continue; }
        // This rounded midpoint is only a choice of plane direction: any
        // finite direction is valid for the interval-verified support test.
        const double midpoint=0.5*inverse[row][col].getLower()+0.5*inverse[row][col].getUpper();
        normals[1][col]+=midpoint*d[row];
    }
    for (int j=0;j<3;++j) {
        if (!java::Double::isFinite(normals[1][j])) inverseCandidateValid=false;
        if (normals[1][j]!=0) usable[1]=true;
    }
    usable[1]=usable[1] && inverseCandidateValid;
    FloatingInterval localSquared=FloatingInterval::exact(0);
    FloatingInterval inverseSquared=FloatingInterval::exact(0);
    FloatingInterval local[3];
    for (int row=0;row<3;++row) {
        FloatingInterval component=FloatingInterval::exact(0);
        for (int col=0;col<3;++col) {
            component=component.add(inverse[row][col].multiply(FloatingInterval::exact(p[col])
                .subtract(FloatingInterval::exact(forward.get(col,3)))));
            inverseSquared=inverseSquared.add(inverse[row][col].square());
        }
        local[row]=component;
        localSquared=localSquared.add(component.square());
    }
    const FloatingInterval localLength=localSquared.squareRoot();
    const FloatingInterval inverseNorm=inverseSquared.squareRoot();
    if (localLength.isValid() && inverseNorm.isValid() && inverseNorm.getLower()>0) {
        if (localLength.getUpper()<radius) {
            certifiedSide=GeometryContainment::Inside;
            const FloatingInterval margin=FloatingInterval::exact(radius).subtract(localLength).divide(inverseNorm);
            if (margin.isValid() && margin.getLower()>0) lowerBound=margin.getLower();
            // For Q=A^-T A^-1 and g=A^-T x, the exact quadratic obeys
            // q(p+d)<=q(p)+2||g||||d||+||Q||_2||d||^2. Enclose g and Q,
            // bound the symmetric spectral norm by its largest absolute row
            // sum, and solve the scalar quadratic with outward rounding.
            FloatingInterval gradientSquared=FloatingInterval::exact(0);
            FloatingInterval gradientComponents[3];
            FloatingInterval quadratic[3][3];
            double maxRow=0;
            bool quadraticValid=true;
            for (int row=0;row<3;++row) {
                FloatingInterval gradient=FloatingInterval::exact(0);
                FloatingInterval rowSum=FloatingInterval::exact(0);
                for (int k=0;k<3;++k) gradient=gradient.add(inverse[k][row].multiply(local[k]));
                gradientComponents[row]=gradient;
                gradientSquared=gradientSquared.add(gradient.square());
                for (int col=0;col<3;++col) {
                    FloatingInterval element=FloatingInterval::exact(0);
                    for (int k=0;k<3;++k) element=element.add(inverse[k][row].multiply(inverse[k][col]));
                    quadratic[row][col]=element;
                    if (!element.isValid()) { quadraticValid=false; break; }
                    const double maxAbs=java::Math::max(java::Math::abs(element.getLower()),
                        java::Math::abs(element.getUpper()));
                    rowSum=rowSum.add(FloatingInterval::exact(maxAbs));
                }
                if (!rowSum.isValid()) quadraticValid=false;
                else if (rowSum.getUpper()>maxRow) maxRow=rowSum.getUpper();
            }
            const FloatingInterval gradientLength=gradientSquared.squareRoot();
            const FloatingInterval gapSquared=FloatingInterval::exact(radius).square().subtract(localSquared);
            if (quadraticValid && gradientLength.isValid() && gapSquared.isValid() && gapSquared.getLower()>0) {
                const FloatingInterval gap=FloatingInterval::exact(gapSquared.getLower());
                const FloatingInterval grad=FloatingInterval::exact(gradientLength.getUpper());
                const FloatingInterval curvature=FloatingInterval::exact(maxRow);
                const FloatingInterval denominator=grad.square().add(curvature.multiply(gap)).squareRoot().add(grad);
                const FloatingInterval root=gap.divide(denominator);
                if (root.isValid() && root.getLower()>lowerBound) lowerBound=root.getLower();
                if (refineInterior) {
                    const double dual=QuadraticInteriorBound::improve(radius,localSquared,
                        gradientComponents,quadratic,maxRow);
                    if (dual>lowerBound) lowerBound=dual;
                }
            }
        } else if (localLength.getLower()>radius) certifiedSide=GeometryContainment::Outside;
    }
    for (int candidateIndex=0;candidateIndex<2;++candidateIndex) {
        if (!usable[candidateIndex]) continue;
        const double* n=normals[candidateIndex];
        FloatingInterval projection=FloatingInterval::exact(0);
        FloatingInterval normalSquared=FloatingInterval::exact(0);
        FloatingInterval supportSquared=FloatingInterval::exact(0);
        for (int i=0;i<3;++i) {
            const FloatingInterval ni=FloatingInterval::exact(n[i]);
            normalSquared=normalSquared.add(ni.square());
            projection=projection.add(ni.multiply(FloatingInterval::exact(p[i])
                .subtract(FloatingInterval::exact(forward.get(i,3)))));
            FloatingInterval component=FloatingInterval::exact(0);
            for (int j=0;j<3;++j) component=component.add(FloatingInterval::exact(n[j])
                .multiply(FloatingInterval::exact(forward.get(j,i))));
            supportSquared=supportSquared.add(component.square());
        }
        const FloatingInterval support=supportSquared.squareRoot().multiply(FloatingInterval::exact(radius));
        const FloatingInterval gap=projection.subtract(support);
        const FloatingInterval normalLength=normalSquared.squareRoot();
        if (!gap.isValid() || !normalLength.isValid() || !(normalLength.getLower()>0))
            return GeometryDistanceStatus::Indeterminate;
        if (gap.getLower()>0) {
            const FloatingInterval gapDistance=gap.divide(normalLength);
            if (!gapDistance.isValid()) return GeometryDistanceStatus::Indeterminate;
            if (certifiedSide==GeometryContainment::Inside) return GeometryDistanceStatus::Indeterminate;
            certifiedSide=GeometryContainment::Outside;
            if (gapDistance.getLower()>lowerBound) lowerBound=gapDistance.getLower();
        }
    }
    const double upperBound=distance.getUpper();
    if (!java::Double::isFinite(upperBound) || lowerBound>upperBound) return GeometryDistanceStatus::Indeterminate;
    side=certifiedSide;
    lower=lowerBound;
    upper=upperBound;
    return GeometryDistanceStatus::Valid;
}
