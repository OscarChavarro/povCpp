#include "vsdk/toolkit/environment/geometry/volume/QuadraticInteriorBound.h"
#include "java/lang/Double.h"
#include "java/lang/Math.h"

double QuadraticInteriorBound::improve(double radius, const FloatingInterval& localSquared,
    const FloatingInterval gradient[3], const FloatingInterval quadratic[3][3], double spectralUpper)
{
    if (!(spectralUpper>0) || !java::Double::isFinite(spectralUpper)) return 0;
    const FloatingInterval radiusSquared=FloatingInterval::exact(radius).square();
    double q[3][3],g[3];
    for (int i=0;i<3;++i) {
        if (!gradient[i].isValid()) return 0;
        g[i]=0.5*gradient[i].getLower()+0.5*gradient[i].getUpper();
        if (!java::Double::isFinite(g[i])) return 0;
        for (int j=0;j<3;++j) {
            if (!quadratic[i][j].isValid()) return 0;
            q[i][j]=0.5*quadratic[i][j].getLower()+0.5*quadratic[i][j].getUpper();
            if (!java::Double::isFinite(q[i][j])) return 0;
        }
    }
    double best=0;
    double extra=spectralUpper*0.005;
    if (!(extra>0) || !java::Double::isFinite(extra)) return 0;
    for (int candidate=0;candidate<16;++candidate,extra*=2) {
        const double lambda=spectralUpper+extra;
        if (!java::Double::isFinite(lambda) || !(lambda>spectralUpper)) break;
        // Fixed-point iteration only proposes z. Correctness does not depend
        // on convergence: the interval residual below accounts for its error.
        double z[3]={0,0,0};
        bool finite=true;
        for (int iteration=0;iteration<24;++iteration) {
            double next[3]={0,0,0};
            for (int i=0;i<3;++i) {
                next[i]=g[i];
                for (int j=0;j<3;++j) next[i]+=q[i][j]*z[j];
                next[i]/=lambda;
                if (!java::Double::isFinite(next[i])) finite=false;
            }
            if (!finite) break;
            for (int i=0;i<3;++i) z[i]=next[i];
        }
        if (!finite) continue;
        const FloatingInterval delta=FloatingInterval::exact(lambda).subtract(FloatingInterval::exact(spectralUpper));
        if (!delta.isValid() || !(delta.getLower()>0)) continue;
        FloatingInterval dot=FloatingInterval::exact(0);
        FloatingInterval quadraticTerm=FloatingInterval::exact(0);
        FloatingInterval residualSquared=FloatingInterval::exact(0);
        for (int i=0;i<3;++i) {
            const FloatingInterval zi=FloatingInterval::exact(z[i]);
            dot=dot.add(gradient[i].multiply(zi));
            FloatingInterval residual=gradient[i];
            for (int j=0;j<3;++j) {
                const FloatingInterval p=FloatingInterval::exact(i==j ? lambda : 0).subtract(quadratic[i][j]);
                const FloatingInterval pz=p.multiply(FloatingInterval::exact(z[j]));
                residual=residual.subtract(pz);
                quadraticTerm=quadraticTerm.add(zi.multiply(pz));
            }
            residualSquared=residualSquared.add(residual.square());
        }
        // P=lambda I-Q is positive definite and lambda_min(P)>=delta.
        // Completing the square at arbitrary z bounds the exact maximum by
        // 2g.z-z.P.z + ||g-Pz||^2/delta.
        const FloatingInterval correction=dot.multiply(FloatingInterval::exact(2))
            .subtract(quadraticTerm).add(residualSquared.divide(delta));
        const FloatingInterval gap=radiusSquared.subtract(localSquared).subtract(correction);
        if (!gap.isValid() || !(gap.getLower()>0)) continue;
        const FloatingInterval root=gap.divide(FloatingInterval::exact(lambda)).squareRoot();
        if (root.isValid() && root.getLower()>best) best=root.getLower();
    }
    return best;
}
