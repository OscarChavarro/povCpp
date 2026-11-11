#include "vsdk/toolkit/environment/geometry/volume/OrthogonalEllipsoidDistance.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "java/lang/Double.h"
#include "java/lang/Math.h"

GeometryDistanceStatus OrthogonalEllipsoidDistance::query(
    const Vector3Dd& point, double radius, const Matrix4x4d& forward,
    double& distance, double& estimatedError)
{
    Vector3Dd witness;
    return queryWithWitness(point,radius,forward,distance,estimatedError,witness);
}

GeometryDistanceStatus OrthogonalEllipsoidDistance::queryWithWitness(
    const Vector3Dd& point, double radius, const Matrix4x4d& forward,
    double& distance, double& estimatedError, Vector3Dd& witness)
{
    if (!java::Double::isFinite(radius) || !(radius > 0) ||
        !java::Double::isFinite(point.x()) || !java::Double::isFinite(point.y()) ||
        !java::Double::isFinite(point.z())) return GeometryDistanceStatus::InvalidInput;
    for (int i=0;i<4;++i) for (int j=0;j<4;++j)
        if (!java::Double::isFinite(forward.get(i,j))) return GeometryDistanceStatus::InvalidInput;
    if (forward.get(3,0)!=0 || forward.get(3,1)!=0 || forward.get(3,2)!=0 || forward.get(3,3)!=1)
        return GeometryDistanceStatus::InvalidInput;
    const double p[3]={point.x(),point.y(),point.z()};
    double axis[3][3], length[3], x[3], a[3];
    double maxAxis=0, minAxis=java::Double::MAX_VALUE, magnitude=1;
    for (int i=0;i<3;++i) {
        double sum=0;
        for (int j=0;j<3;++j) { axis[i][j]=forward.get(j,i); sum+=axis[i][j]*axis[i][j]; }
        length[i]=java::Math::sqrt(sum);
        if (!java::Double::isFinite(length[i])) return GeometryDistanceStatus::Indeterminate;
        if (!(length[i]>0)) {
            // A nonzero column can have a squared norm that underflows; that
            // is not proof of a singular transformation.
            if (axis[i][0]!=0 || axis[i][1]!=0 || axis[i][2]!=0)
                return GeometryDistanceStatus::Indeterminate;
            return GeometryDistanceStatus::InvalidInput;
        }
        a[i]=radius*length[i];
        if (!java::Double::isFinite(a[i]) || !(a[i]>0)) return GeometryDistanceStatus::Indeterminate;
        if (a[i]>maxAxis) maxAxis=a[i];
        if (a[i]<minAxis) minAxis=a[i];
        for (int j=0;j<3;++j) axis[i][j]/=length[i];
    }
    if (minAxis/maxAxis<1e-6) return GeometryDistanceStatus::Indeterminate;
    for (int i=0;i<3;++i) for (int j=0;j<i;++j) {
        double dot=0;
        for (int k=0;k<3;++k) dot+=axis[i][k]*axis[j][k];
        if (!java::Double::isFinite(dot)) return GeometryDistanceStatus::Indeterminate;
        if (java::Math::abs(dot)>1e-12) return GeometryDistanceStatus::Unsupported;
    }
    for (int i=0;i<3;++i) {
        x[i]=0;
        for (int j=0;j<3;++j) {
            const double delta=p[j]-forward.get(j,3);
            x[i]+=axis[i][j]*delta/maxAxis;
            magnitude+=java::Math::abs(p[j])+java::Math::abs(forward.get(j,3));
        }
        a[i]/=maxAxis;
        if (!java::Double::isFinite(x[i])) return GeometryDistanceStatus::Indeterminate;
    }
    double implicit=0;
    for (int i=0;i<3;++i) implicit+=(x[i]/a[i])*(x[i]/a[i]);
    if (!java::Double::isFinite(implicit)) return GeometryDistanceStatus::Indeterminate;
    double y[3]={0,0,0};
    double computedDistance=0;
    if (implicit==1) {
        computedDistance=0;
        for (int i=0;i<3;++i) y[i]=x[i];
    } else {
        const bool inside=implicit<1;
        double minimumSquared=1;
        for (int i=0;i<3;++i) if (a[i]*a[i]<minimumSquared) minimumSquared=a[i]*a[i];
        bool hard=inside;
        double hardSum=0;
        int shortest=0;
        if (inside) for (int i=0;i<3;++i) {
            const double sq=a[i]*a[i];
            if (sq==minimumSquared) {
                shortest=i;
                if (x[i]!=0) hard=false;
            } else {
                const double ratio=a[i]*x[i]/(sq-minimumSquared);
                hardSum+=ratio*ratio;
            }
        }
        if (hard && java::Double::isFinite(hardSum) && hardSum<=1) {
            for (int i=0;i<3;++i) {
                const double sq=a[i]*a[i];
                if (sq!=minimumSquared) y[i]=sq*x[i]/(sq-minimumSquared);
            }
            y[shortest]=a[shortest]*java::Math::sqrt(1-hardSum);
        } else {
            double lo=inside ? -minimumSquared : 0, hi=inside ? 0 : 1;
            if (!inside) {
                bool bracketed=false;
                for (int iteration=0;iteration<128;++iteration) {
                    double f=0;
                    for (int i=0;i<3;++i) { const double z=a[i]*x[i]/(a[i]*a[i]+hi); f+=z*z; }
                    if (java::Double::isFinite(f) && f<=1) { bracketed=true; break; }
                    hi*=2;
                    if (!java::Double::isFinite(hi)) break;
                }
                if (!bracketed) return GeometryDistanceStatus::Indeterminate;
            }
            for (int iteration=0;iteration<128;++iteration) {
                const double mid=lo+(hi-lo)*0.5;
                if (mid==lo || mid==hi) break;
                double f=0;
                for (int i=0;i<3;++i) { const double z=a[i]*x[i]/(a[i]*a[i]+mid); f+=z*z; }
                if (!java::Double::isFinite(f)) return GeometryDistanceStatus::Indeterminate;
                if (f>1) lo=mid; else hi=mid;
            }
            const double lambda=lo+(hi-lo)*0.5;
            double surfaceCheck=0;
            for (int i=0;i<3;++i) {
                const double denominator=a[i]*a[i]+lambda;
                if (!(denominator>0)) return GeometryDistanceStatus::Indeterminate;
                y[i]=a[i]*a[i]*x[i]/denominator;
                const double z=y[i]/a[i]; surfaceCheck+=z*z;
            }
            if (!java::Double::isFinite(surfaceCheck) || java::Math::abs(surfaceCheck-1)>1e-8)
                return GeometryDistanceStatus::Indeterminate;
        }
        double squaredDistance=0;
        for (int i=0;i<3;++i) { const double delta=y[i]-x[i]; squaredDistance+=delta*delta; }
        computedDistance=maxAxis*java::Math::sqrt(squaredDistance);
    }
    const double computedError=1e-9*(magnitude+maxAxis+computedDistance);
    if (!java::Double::isFinite(computedDistance) || !java::Double::isFinite(computedError)) return GeometryDistanceStatus::Indeterminate;
    double coordinates[3]={forward.get(0,3),forward.get(1,3),forward.get(2,3)};
    for (int i=0;i<3;++i) for (int j=0;j<3;++j) coordinates[j]+=axis[i][j]*y[i]*maxAxis;
    if (!java::Double::isFinite(coordinates[0]) || !java::Double::isFinite(coordinates[1]) ||
        !java::Double::isFinite(coordinates[2])) return GeometryDistanceStatus::Indeterminate;
    distance=computedDistance;
    estimatedError=computedError;
    witness=Vector3Dd(coordinates[0],coordinates[1],coordinates[2]);
    return GeometryDistanceStatus::Valid;
}
