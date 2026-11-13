#include "java/lang/Double.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/environment/scene/csg/CsgExactArithmetic.h"
#include "vsdk/toolkit/environment/scene/csg/CsgExactTransform.h"
bool CsgExactTransform::exactSum(const double* a, const double* b, int count, double& result)
{
    if (!a || !b || count<=0 || count>4) return false;
    double candidate=0;
    for (int i=0;i<count;++i) candidate+=a[i]*b[i];
    if (!java::Double::isFinite(candidate)) return false;
    const double c[4]={candidate,0,0,0}, d[4]={1,0,0,0};
    if (!CsgExactArithmetic::equalSumProducts(a,b,c,d,count)) return false;
    result=candidate;
    return true;
}
bool CsgExactTransform::inverseNumerator(const Matrix4x4d& forward, double result[4][4])
{
    if (!result || forward.get(3,0)!=0 || forward.get(3,1)!=0 || forward.get(3,2)!=0 || forward.get(3,3)!=1) return false;
    double cofactor[3][3];
    for (int i=0;i<3;++i) for (int j=0;j<3;++j) {
        const int r=(i+1)%3,s=(i+2)%3,c=(j+1)%3,d=(j+2)%3;
        const double a[2]={forward.get(r,c),-forward.get(r,d)};
        const double b[2]={forward.get(s,d),forward.get(s,c)};
        if (!exactSum(a,b,2,cofactor[i][j])) return false;
    }
    const double detTerms[3]={forward.get(0,0),forward.get(0,1),forward.get(0,2)};
    double determinant=0;
    if (!exactSum(detTerms,cofactor[0],3,determinant) || determinant==0) return false;
    double candidate[4][4]={{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}};
    for (int i=0;i<3;++i) {
        for (int j=0;j<3;++j) candidate[i][j]=cofactor[j][i];
        const double translation[3]={-forward.get(0,3),-forward.get(1,3),-forward.get(2,3)};
        if (!exactSum(candidate[i],translation,3,candidate[i][3])) return false;
    }
    candidate[3][3]=determinant;
    for (int i=0;i<4;++i) for (int j=0;j<4;++j) result[i][j]=candidate[i][j];
    return true;
}
