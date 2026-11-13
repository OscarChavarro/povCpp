#include "java/lang/Double.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/common/numerics/FloatingInterval.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/scene/AffineToleranceMap.h"

AffinePlacementStatus AffineToleranceMap::map(const Matrix4x4d& forward, const Vector3Dd& point,
    double tolerance, Vector3Dd& childPoint, double& childTolerance, double* childInnerTolerance,
    Vector3Dd* childHalfWidths)
{
    if (!java::Double::isFinite(point.x()) || !java::Double::isFinite(point.y()) ||
        !java::Double::isFinite(point.z()) || !java::Double::isFinite(tolerance) || tolerance < 0)
        return AffinePlacementStatus::Invalid;
    if (forward.get(3,0)!=0 || forward.get(3,1)!=0 || forward.get(3,2)!=0 || forward.get(3,3)!=1)
        return AffinePlacementStatus::Invalid;
    if (!FloatingInterval::isSupportedEnvironment()) return AffinePlacementStatus::Unsupported;
    FloatingInterval matrix[3][3], cofactor[3][3], inverse[3][3];
    for (int i=0;i<3;++i) for (int j=0;j<3;++j) {
        matrix[i][j]=FloatingInterval::exact(forward.get(i,j));
        if (!matrix[i][j].isValid()) return AffinePlacementStatus::Invalid;
    }
    for (int i=0;i<3;++i) for (int j=0;j<3;++j) {
        const int a=(i+1)%3, b=(i+2)%3, c=(j+1)%3, d=(j+2)%3;
        cofactor[i][j]=matrix[a][c].multiply(matrix[b][d]).subtract(
            matrix[a][d].multiply(matrix[b][c]));
    }
    FloatingInterval determinant=FloatingInterval::exact(0);
    for (int j=0;j<3;++j) determinant=determinant.add(matrix[0][j].multiply(cofactor[0][j]));
    if (!determinant.isValid() || (determinant.getLower()<=0 && determinant.getUpper()>=0))
        return AffinePlacementStatus::Indeterminate;
    for (int i=0;i<3;++i) for (int j=0;j<3;++j) {
        inverse[i][j]=cofactor[j][i].divide(determinant);
        if (!inverse[i][j].isValid()) return AffinePlacementStatus::Indeterminate;
    }
    const double coordinates[3]={point.x(),point.y(),point.z()};
    FloatingInterval delta[3];
    for (int i=0;i<3;++i) {
        delta[i]=FloatingInterval::exact(coordinates[i]).subtract(FloatingInterval::exact(forward.get(i,3)));
        if (!delta[i].isValid()) return AffinePlacementStatus::Indeterminate;
    }
    double center[3], halfWidth[3];
    FloatingInterval total=FloatingInterval::exact(0);
    for (int i=0;i<3;++i) {
        FloatingInterval mapped=FloatingInterval::exact(0), radius=FloatingInterval::exact(0);
        for (int j=0;j<3;++j) {
            mapped=mapped.add(inverse[i][j].multiply(delta[j]));
            const double magnitude=java::Math::max(java::Math::abs(inverse[i][j].getLower()),
                java::Math::abs(inverse[i][j].getUpper()));
            radius=radius.add(FloatingInterval::exact(magnitude).multiply(FloatingInterval::exact(tolerance)));
        }
        if (!mapped.isValid() || !radius.isValid()) return AffinePlacementStatus::Indeterminate;
        center[i]=mapped.getLower()*0.5+mapped.getUpper()*0.5;
        if (!java::Double::isFinite(center[i])) return AffinePlacementStatus::Indeterminate;
        const FloatingInterval error=mapped.subtract(FloatingInterval::exact(center[i]));
        if (!error.isValid()) return AffinePlacementStatus::Indeterminate;
        const double errorBound=java::Math::max(java::Math::abs(error.getLower()),java::Math::abs(error.getUpper()));
        radius=radius.add(FloatingInterval::exact(errorBound));
        total=total.add(radius.square());
        // Along axis i the image of the unit ball under the inverse extends by
        // the Euclidean norm of row i, bounded here using the row's magnitudes.
        FloatingInterval rowSquared=FloatingInterval::exact(0);
        for (int j=0;j<3;++j) {
            const double magnitude=java::Math::max(java::Math::abs(inverse[i][j].getLower()),
                java::Math::abs(inverse[i][j].getUpper()));
            rowSquared=rowSquared.add(FloatingInterval::exact(magnitude).square());
        }
        const FloatingInterval width=rowSquared.squareRoot().multiply(FloatingInterval::exact(tolerance))
            .add(FloatingInterval::exact(errorBound));
        if (!width.isValid()) return AffinePlacementStatus::Indeterminate;
        halfWidth[i]=width.getUpper();
    }
    const FloatingInterval bound=total.squareRoot();
    if (!bound.isValid()) return AffinePlacementStatus::Indeterminate;
    double inner=-1;
    if (childInnerTolerance && tolerance>0) {
        // A child ball of radius r maps inside the world ball when
        // ||A center + translation - point|| + ||A||_2 r <= tolerance.
        FloatingInterval errorSquared=FloatingInterval::exact(0);
        FloatingInterval normSquared=FloatingInterval::exact(0);
        for (int i=0;i<3;++i) {
            FloatingInterval mapped=FloatingInterval::exact(forward.get(i,3));
            for (int j=0;j<3;++j) {
                mapped=mapped.add(matrix[i][j].multiply(FloatingInterval::exact(center[j])));
                normSquared=normSquared.add(matrix[i][j].square());
            }
            mapped=mapped.subtract(FloatingInterval::exact(coordinates[i]));
            errorSquared=errorSquared.add(mapped.square());
        }
        const FloatingInterval error=errorSquared.squareRoot();
        // Any upper bound of the spectral norm works: use the smaller of the
        // Frobenius norm and sqrt(||A||_1 ||A||_inf).
        FloatingInterval norm=normSquared.squareRoot();
        FloatingInterval rowMaximum=FloatingInterval::exact(0), columnMaximum=FloatingInterval::exact(0);
        for (int i=0;i<3;++i) {
            FloatingInterval row=FloatingInterval::exact(0), column=FloatingInterval::exact(0);
            for (int j=0;j<3;++j) {
                row=row.add(FloatingInterval::exact(java::Math::abs(forward.get(i,j))));
                column=column.add(FloatingInterval::exact(java::Math::abs(forward.get(j,i))));
            }
            if (row.isValid() && rowMaximum.isValid() && row.getUpper()>rowMaximum.getUpper()) rowMaximum=row;
            if (column.isValid() && columnMaximum.isValid() && column.getUpper()>columnMaximum.getUpper()) columnMaximum=column;
            if (!row.isValid()) rowMaximum=FloatingInterval();
            if (!column.isValid()) columnMaximum=FloatingInterval();
        }
        const FloatingInterval mixed=rowMaximum.multiply(columnMaximum).squareRoot();
        if (mixed.isValid() && norm.isValid() && mixed.getUpper()<norm.getUpper() && mixed.getLower()>0) norm=mixed;
        if (error.isValid() && norm.isValid() && norm.getLower()>0) {
            const FloatingInterval remaining=FloatingInterval::exact(tolerance).subtract(error);
            if (remaining.isValid() && remaining.getLower()>0) {
                const FloatingInterval ratio=remaining.divide(norm);
                if (ratio.isValid() && ratio.getLower()>0) inner=ratio.getLower();
            }
        }
    }
    childPoint=Vector3Dd(center[0],center[1],center[2]);
    childTolerance=bound.getUpper();
    if (childInnerTolerance) *childInnerTolerance=inner;
    if (childHalfWidths) *childHalfWidths=Vector3Dd(halfWidth[0],halfWidth[1],halfWidth[2]);
    return AffinePlacementStatus::Valid;
}
