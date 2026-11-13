#include "java/lang/Double.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/environment/scene/csg/CsgExactTransform.h"
#include "vsdk/toolkit/environment/scene/csg/CsgExactPlane.h"
bool CsgExactPlane::world(const double* local, const Matrix4x4d& forward, double* result)
{
    if (!local || !result) return false;
    for (int i=0;i<4;++i) if (!java::Double::isFinite(local[i])) return false;
    if (local[0]==0 && local[1]==0 && local[2]==0) return false;
    double inverse[4][4];
    if (!CsgExactTransform::inverseNumerator(forward,inverse)) return false;
    double candidate[4];
    for (int i=0;i<4;++i) {
        const double column[4]={inverse[0][i],inverse[1][i],inverse[2][i],inverse[3][i]};
        if (!CsgExactTransform::exactSum(column,local,4,candidate[i])) return false;
    }
    const double sign=inverse[3][3]<0 ? -1 : 1;
    for (int i=0;i<4;++i) result[i]=sign*candidate[i];
    return true;
}
