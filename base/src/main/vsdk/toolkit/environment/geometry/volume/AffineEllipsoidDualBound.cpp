#include "vsdk/toolkit/environment/geometry/volume/AffineEllipsoidDualBound.h"
#include "vsdk/toolkit/common/numerics/FloatingInterval.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "java/lang/Double.h"
#include "java/lang/Math.h"

double AffineEllipsoidDualBound::lowerDistance(const Vector3Dd& point, double radius,
    const Matrix4x4d& forward, const Vector3Dd& localDirection)
{
    if (!FloatingInterval::isSupportedEnvironment() || !java::Double::isFinite(radius) || !(radius>0)) return 0;
    const double seedLength=localDirection.length();
    if (!(seedLength>0) || !java::Double::isFinite(seedLength)) return 0;
    const Vector3Dd unit=localDirection.multiply(1.0/seedLength);
    const double z[3]={unit.x(),unit.y(),unit.z()};
    const double p[3]={point.x(),point.y(),point.z()};
    FloatingInterval b[3][3],difference[3],h[3][3],g[3];
    FloatingInterval distanceSquared=FloatingInterval::exact(0);
    double scale=0;
    for (int row=0;row<3;++row) {
        if (!java::Double::isFinite(z[row]) || !java::Double::isFinite(p[row])) return 0;
        difference[row]=FloatingInterval::exact(p[row]).subtract(FloatingInterval::exact(forward.get(row,3)));
        distanceSquared=distanceSquared.add(difference[row].square());
        for (int col=0;col<3;++col)
            b[row][col]=FloatingInterval::exact(forward.get(row,col)).multiply(FloatingInterval::exact(radius));
    }
    for (int i=0;i<3;++i) {
        g[i]=FloatingInterval::exact(0);
        for (int k=0;k<3;++k) g[i]=g[i].add(b[k][i].multiply(difference[k]));
        if (!g[i].isValid()) return 0;
        for (int j=0;j<3;++j) {
            h[i][j]=FloatingInterval::exact(0);
            for (int k=0;k<3;++k) h[i][j]=h[i][j].add(b[k][i].multiply(b[k][j]));
            if (!h[i][j].isValid()) return 0;
            const double magnitude=java::Math::max(java::Math::abs(h[i][j].getLower()),java::Math::abs(h[i][j].getUpper()));
            if (magnitude>scale) scale=magnitude;
        }
    }
    if (!distanceSquared.isValid() || !(scale>0) || !java::Double::isFinite(scale)) return 0;
    double estimate=0;
    for (int i=0;i<3;++i) {
        const double gi=0.5*g[i].getLower()+0.5*g[i].getUpper();
        estimate+=z[i]*gi;
        for (int j=0;j<3;++j) estimate-=z[i]*z[j]*(0.5*h[i][j].getLower()+0.5*h[i][j].getUpper());
    }
    if (!java::Double::isFinite(estimate)) return 0;
    double shift=1e-13*(1+scale+java::Math::abs(estimate));
    double best=0;
    for (int attempt=0;attempt<20;++attempt,shift*=8) {
        const double lambda=estimate+shift;
        if (!java::Double::isFinite(lambda) || !java::Double::isFinite(shift)) break;
        FloatingInterval matrix[3][3];
        double normUpper=0;
        bool ok=true;
        for (int i=0;i<3;++i) {
            FloatingInterval rowSum=FloatingInterval::exact(0);
            for (int j=0;j<3;++j) {
                matrix[i][j]=h[i][j].add(FloatingInterval::exact(i==j?lambda:0));
                if (!matrix[i][j].isValid()) { ok=false; break; }
                const double maxAbs=java::Math::max(java::Math::abs(matrix[i][j].getLower()),java::Math::abs(matrix[i][j].getUpper()));
                rowSum=rowSum.add(FloatingInterval::exact(maxAbs));
            }
            if (!rowSum.isValid()) ok=false;
            else if (rowSum.getUpper()>normUpper) normUpper=rowSum.getUpper();
        }
        if (!ok || !(normUpper>0)) continue;
        const FloatingInterval minor2=matrix[0][0].multiply(matrix[1][1]).subtract(matrix[0][1].multiply(matrix[1][0]));
        const FloatingInterval det=matrix[0][0].multiply(matrix[1][1].multiply(matrix[2][2]).subtract(matrix[1][2].multiply(matrix[2][1])))
            .subtract(matrix[0][1].multiply(matrix[1][0].multiply(matrix[2][2]).subtract(matrix[1][2].multiply(matrix[2][0]))))
            .add(matrix[0][2].multiply(matrix[1][0].multiply(matrix[2][1]).subtract(matrix[1][1].multiply(matrix[2][0]))));
        if (!minor2.isValid() || !det.isValid() || !(matrix[0][0].getLower()>0) ||
            !(minor2.getLower()>0) || !(det.getLower()>0)) continue;
        // Sylvester proves positive definiteness. det/||P||^2 is a lower
        // bound for its smallest eigenvalue because both other eigenvalues
        // are at most the symmetric maximum absolute row sum.
        const FloatingInterval eigenLower=det.divide(FloatingInterval::exact(normUpper).square());
        if (!eigenLower.isValid() || !(eigenLower.getLower()>0)) continue;
        FloatingInterval dot=FloatingInterval::exact(0),quadratic=FloatingInterval::exact(0);
        FloatingInterval residualSquared=FloatingInterval::exact(0);
        for (int i=0;i<3;++i) {
            const FloatingInterval zi=FloatingInterval::exact(z[i]);
            dot=dot.add(g[i].multiply(zi));
            FloatingInterval residual=g[i];
            for (int j=0;j<3;++j) {
                const FloatingInterval pz=matrix[i][j].multiply(FloatingInterval::exact(z[j]));
                residual=residual.subtract(pz);
                quadratic=quadratic.add(zi.multiply(pz));
            }
            residualSquared=residualSquared.add(residual.square());
        }
        const FloatingInterval correction=dot.multiply(FloatingInterval::exact(2)).subtract(quadratic)
            .add(residualSquared.divide(FloatingInterval::exact(eigenLower.getLower())));
        const FloatingInterval lowerSquared=distanceSquared.subtract(FloatingInterval::exact(lambda)).subtract(correction);
        if (!lowerSquared.isValid() || !(lowerSquared.getLower()>0)) continue;
        const FloatingInterval result=FloatingInterval::exact(lowerSquared.getLower()).squareRoot();
        if (result.isValid() && result.getLower()>best) best=result.getLower();
    }
    return best;
}
