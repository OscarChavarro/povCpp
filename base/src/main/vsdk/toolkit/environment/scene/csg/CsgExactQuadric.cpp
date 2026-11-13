#include "java/lang/Double.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/environment/scene/csg/CsgExactArithmetic.h"
#include "vsdk/toolkit/environment/scene/csg/CsgExactTransform.h"
#include "vsdk/toolkit/environment/scene/csg/CsgExactQuadric.h"
bool CsgExactQuadric::world(const double* local, const Matrix4x4d& forward, double* result)
{
    if (!local || !result) return false;
    for (int i=0;i<10;++i) if (!java::Double::isFinite(local[i])) return false;
    double inverse[4][4]; if (!CsgExactTransform::inverseNumerator(forward,inverse)) return false;
    double q[4][4]={{local[0],0,0,0},{0,local[1],0,0},{0,0,local[2],0},{0,0,0,local[9]}};
    const int row[6]={0,0,1,0,1,2}, col[6]={1,2,2,3,3,3};
    for (int k=0;k<6;++k) {
        const double half=local[3+k]*0.5;
        if (!CsgExactArithmetic::equalProducts(local[3+k],0.5,half,1)) return false;
        q[row[k]][col[k]]=q[col[k]][row[k]]=half;
    }
    double intermediate[4][4], world[4][4];
    for (int i=0;i<4;++i) for (int j=0;j<4;++j) {
        const double column[4]={inverse[0][j],inverse[1][j],inverse[2][j],inverse[3][j]};
        if (!CsgExactTransform::exactSum(q[i],column,4,intermediate[i][j])) return false;
    }
    for (int i=0;i<4;++i) for (int j=0;j<4;++j) {
        const double column[4]={inverse[0][i],inverse[1][i],inverse[2][i],inverse[3][i]};
        const double values[4]={intermediate[0][j],intermediate[1][j],intermediate[2][j],intermediate[3][j]};
        if (!CsgExactTransform::exactSum(column,values,4,world[i][j])) return false;
    }
    double candidate[10]={world[0][0],world[1][1],world[2][2],0,0,0,0,0,0,world[3][3]};
    for (int k=0;k<6;++k) {
        candidate[3+k]=world[row[k]][col[k]]*2;
        if (!CsgExactArithmetic::equalProducts(world[row[k]][col[k]],2,candidate[3+k],1)) return false;
    }
    for (int i=0;i<10;++i) result[i]=candidate[i];
    return true;
}
