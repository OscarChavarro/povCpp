#include "java/lang/Double.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/common/numerics/FloatingInterval.h"
#include "vsdk/toolkit/environment/scene/csg/CsgExactArithmetic.h"

bool CsgExactArithmetic::decompose(double value, unsigned long long& significand, int& exponent)
{
    if (!java::Double::isFinite(value)) return false;
    if (value==0) { significand=0; exponent=0; return true; }
    const double magnitude=java::Math::abs(value);
    exponent=java::Math::getExponent(magnitude);
    const double normalized=java::Math::scalb(magnitude,-exponent);
    const double integer=java::Math::scalb(normalized,52);
    if (!java::Double::isFinite(integer) || integer<4503599627370496.0 || integer>=9007199254740992.0)
        return false;
    significand=static_cast<unsigned long long>(integer);
    exponent-=52;
    while ((significand&1ULL)==0) { significand>>=1; ++exponent; }
    return true;
}

bool CsgExactArithmetic::addWord(unsigned long long* target, int index, unsigned long long word)
{
    const unsigned long long mask=0xffffffffULL;
    while (word!=0) {
        if (index<0 || index>=140) return false;
        const unsigned long long sum=target[index]+word;
        target[index]=sum&mask;
        word=sum>>32;
        ++index;
    }
    return true;
}

bool CsgExactArithmetic::addProduct(unsigned long long* positive, unsigned long long* negative,
    double a, double b, bool negate)
{
    unsigned long long x=0,y=0;
    int ex=0,ey=0;
    if (!decompose(a,x,ex) || !decompose(b,y,ey)) return false;
    if (x==0 || y==0) return true;
    const unsigned long long mask=0xffffffffULL;
    const unsigned long long xl[2]={x&mask,x>>32}, yl[2]={y&mask,y>>32};
    unsigned long long product[4]={0,0,0,0};
    for (int i=0;i<2;++i) {
        unsigned long long carry=0;
        for (int j=0;j<2;++j) {
            const unsigned long long value=xl[i]*yl[j]+product[i+j]+carry;
            product[i+j]=value&mask;
            carry=value>>32;
        }
        product[i+2]=carry;
    }
    const int offset=ex+ey+2148;
    if (offset<0) return false;
    const int wordOffset=offset/32, shift=offset%32;
    unsigned long long* target=((a<0)!=(b<0))!=negate ? negative : positive;
    for (int i=0;i<4;++i) {
        const unsigned long long shifted=product[i]<<shift;
        if (!addWord(target,wordOffset+i,shifted&mask) ||
            !addWord(target,wordOffset+i+1,shifted>>32)) return false;
    }
    return true;
}

bool CsgExactArithmetic::equalSumProducts(const double* a, const double* b,
    const double* c, const double* d, int count)
{
    if (!a || !b || !c || !d || count<=0 || !FloatingInterval::isSupportedEnvironment()) return false;
    unsigned long long positive[140]={0}, negative[140]={0};
    for (int i=0;i<count;++i)
        if (!addProduct(positive,negative,a[i],b[i],false) ||
            !addProduct(positive,negative,c[i],d[i],true)) return false;
    for (int i=0;i<140;++i) if (positive[i]!=negative[i]) return false;
    return true;
}

bool CsgExactArithmetic::equalProducts(double a, double b, double c, double d)
{
    if (!FloatingInterval::isSupportedEnvironment()) return false;
    unsigned long long positive[140]={0}, negative[140]={0};
    if (!addProduct(positive,negative,a,b,false) || !addProduct(positive,negative,c,d,true)) return false;
    for (int i=0;i<140;++i) if (positive[i]!=negative[i]) return false;
    return true;
}

bool CsgExactArithmetic::equalAffine(double a, double b, double c, double d, double e, double f)
{
    if (!FloatingInterval::isSupportedEnvironment()) return false;
    unsigned long long positive[140]={0}, negative[140]={0};
    if (!addProduct(positive,negative,a,b,false) || !addProduct(positive,negative,c,1,false) ||
        !addProduct(positive,negative,d,e,true) || !addProduct(positive,negative,f,1,true)) return false;
    for (int i=0;i<140;++i) if (positive[i]!=negative[i]) return false;
    return true;
}
