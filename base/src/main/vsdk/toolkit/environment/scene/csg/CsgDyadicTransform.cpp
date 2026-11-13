#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/environment/scene/csg/CsgDyadicTransform.h"
bool CsgDyadicTransform::dot(const CsgDyadic* a, const CsgDyadic* b, int count, CsgDyadic& result)
{
    if (!a || !b || count<=0 || count>4) return false;
    CsgDyadic sum;
    for (int i=0;i<count;++i) {
        CsgDyadic product,next;
        if (!CsgDyadic::multiply(a[i],b[i],product) || !CsgDyadic::add(sum,product,next)) return false;
        sum=next;
    }
    result=sum; return true;
}
bool CsgDyadicTransform::inverseNumerator(const Matrix4x4d& forward, CsgDyadic result[4][4])
{
    if (!result || forward.get(3,0)!=0 || forward.get(3,1)!=0 || forward.get(3,2)!=0 || forward.get(3,3)!=1) return false;
    CsgDyadic matrix[3][3], translation[3], cofactors[3][3], candidate[4][4];
    for (int i=0;i<3;++i) {
        if (!CsgDyadic::fromDouble(-forward.get(i,3),translation[i])) return false;
        for (int j=0;j<3;++j) if (!CsgDyadic::fromDouble(forward.get(i,j),matrix[i][j])) return false;
    }
    for (int i=0;i<3;++i) for (int j=0;j<3;++j) {
        const int r=(i+1)%3,s=(i+2)%3,c=(j+1)%3,d=(j+2)%3;
        CsgDyadic first,second;
        if (!CsgDyadic::multiply(matrix[r][c],matrix[s][d],first) ||
            !CsgDyadic::multiply(matrix[r][d],matrix[s][c],second) ||
            !CsgDyadic::subtract(first,second,cofactors[i][j])) return false;
    }
    CsgDyadic determinant;
    if (!dot(matrix[0],cofactors[0],3,determinant) || determinant.getSign()==0) return false;
    for (int i=0;i<3;++i) {
        for (int j=0;j<3;++j) candidate[i][j]=cofactors[j][i];
        if (!dot(candidate[i],translation,3,candidate[i][3])) return false;
    }
    candidate[3][3]=determinant;
    for (int i=0;i<4;++i) for (int j=0;j<4;++j) result[i][j]=candidate[i][j];
    return true;
}
