#include <climits>
#include "java/lang/Double.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/common/numerics/FloatingInterval.h"
#include "vsdk/toolkit/environment/scene/csg/CsgDyadic.h"
CsgDyadic::CsgDyadic() : limbs{}, length(0), exponent(0), sign(0) {}
bool CsgDyadic::normalize()
{
    while (length>0 && limbs[length-1]==0) --length;
    if (!length) { exponent=0; sign=0; return true; }
    int words=0; while (words<length && limbs[words]==0) ++words;
    if (words) {
        for (int i=words;i<length;++i) limbs[i-words]=limbs[i];
        for (int i=length-words;i<length;++i) limbs[i]=0;
        length-=words;
        const long long shiftedExponent=static_cast<long long>(exponent)+32LL*words;
        if (shiftedExponent>INT_MAX) return false;
        exponent=static_cast<int>(shiftedExponent);
    }
    while ((limbs[0]&1U)==0) {
        unsigned int carry=0;
        for (int i=length-1;i>=0;--i) {
            const unsigned int next=limbs[i]&1U;
            limbs[i]=(limbs[i]>>1)|(carry<<31); carry=next;
        }
        if (exponent==INT_MAX) return false;
        ++exponent;
        while (length>0 && limbs[length-1]==0) --length;
    }
    return true;
}
bool CsgDyadic::fromDouble(double value, CsgDyadic& result)
{
    if (CHAR_BIT!=8 || sizeof(unsigned int)!=4 || sizeof(unsigned long long)<8 || !FloatingInterval::isSupportedEnvironment() ||
        !java::Double::isFinite(value)) return false;
    CsgDyadic candidate;
    if (value!=0) {
        const double absolute=java::Math::abs(value);
        int exp=java::Math::getExponent(absolute);
        const double significand=java::Math::scalb(java::Math::scalb(absolute,-exp),52);
        if (!java::Double::isFinite(significand) || significand<4503599627370496.0 || significand>=9007199254740992.0) return false;
        const unsigned long long bits=static_cast<unsigned long long>(significand);
        candidate.limbs[0]=static_cast<unsigned int>(bits&0xffffffffULL);
        candidate.limbs[1]=static_cast<unsigned int>(bits>>32);
        candidate.length=2; candidate.exponent=exp-52; candidate.sign=value<0 ? -1 : 1;
        if (!candidate.normalize()) return false;
    }
    result=candidate; return true;
}
bool CsgDyadic::shifted(const CsgDyadic& value, int targetExponent, unsigned int* result)
{
    const long long difference=static_cast<long long>(value.exponent)-targetExponent;
    if (difference<0) return false;
    const long long words=difference/32; const int bits=static_cast<int>(difference%32);
    if (value.length && (words>=CAPACITY || words+value.length+(bits?1:0)>CAPACITY)) return false;
    for (int i=0;i<CAPACITY;++i) result[i]=0;
    for (int i=0;i<value.length;++i) {
        const unsigned long long part=static_cast<unsigned long long>(value.limbs[i])<<bits;
        result[words+i] |= static_cast<unsigned int>(part&0xffffffffULL);
        if (bits) result[words+i+1] |= static_cast<unsigned int>(part>>32);
    }
    return true;
}
bool CsgDyadic::add(const CsgDyadic& a, const CsgDyadic& b, CsgDyadic& result)
{
    if (!a.sign) { result=b; return true; }
    if (!b.sign) { result=a; return true; }
    const int common=a.exponent<b.exponent ? a.exponent : b.exponent;
    unsigned int x[CAPACITY],y[CAPACITY];
    if (!shifted(a,common,x) || !shifted(b,common,y)) return false;
    CsgDyadic candidate; candidate.exponent=common;
    if (a.sign==b.sign) {
        unsigned long long carry=0;
        for (int i=0;i<CAPACITY;++i) {
            const unsigned long long sum=static_cast<unsigned long long>(x[i])+y[i]+carry;
            candidate.limbs[i]=static_cast<unsigned int>(sum&0xffffffffULL); carry=sum>>32;
        }
        if (carry) return false;
        candidate.sign=a.sign;
    } else {
        int comparison=0;
        for (int i=CAPACITY-1;i>=0;--i) if (x[i]!=y[i]) { comparison=x[i]>y[i] ? 1 : -1; break; }
        if (!comparison) { result=candidate; return true; }
        const unsigned int* large=comparison>0?x:y; const unsigned int* small=comparison>0?y:x;
        unsigned long long borrow=0;
        for (int i=0;i<CAPACITY;++i) {
            const unsigned long long lhs=large[i], rhs=static_cast<unsigned long long>(small[i])+borrow;
            candidate.limbs[i]=static_cast<unsigned int>((lhs-rhs)&0xffffffffULL); borrow=lhs<rhs ? 1 : 0;
        }
        candidate.sign=comparison>0 ? a.sign : b.sign;
    }
    candidate.length=CAPACITY; if (!candidate.normalize()) return false; result=candidate; return true;
}
bool CsgDyadic::subtract(const CsgDyadic& a, const CsgDyadic& b, CsgDyadic& result)
{ CsgDyadic opposite=b; opposite.negate(); return add(a,opposite,result); }
bool CsgDyadic::multiply(const CsgDyadic& a, const CsgDyadic& b, CsgDyadic& result)
{
    CsgDyadic candidate;
    if (!a.sign || !b.sign) { result=candidate; return true; }
    if (a.length+b.length-1>CAPACITY) return false;
    for (int i=0;i<a.length;++i) {
        unsigned long long carry=0;
        for (int j=0;j<b.length;++j) {
            const int k=i+j;
            if (k>=CAPACITY) return false;
            const unsigned long long value=static_cast<unsigned long long>(a.limbs[i])*b.limbs[j]+candidate.limbs[k]+carry;
            candidate.limbs[k]=static_cast<unsigned int>(value&0xffffffffULL); carry=value>>32;
        }
        int k=i+b.length;
        while (carry) {
            if (k>=CAPACITY) return false;
            const unsigned long long value=static_cast<unsigned long long>(candidate.limbs[k])+carry;
            candidate.limbs[k]=static_cast<unsigned int>(value&0xffffffffULL); carry=value>>32; ++k;
        }
    }
    const long long exp=static_cast<long long>(a.exponent)+b.exponent;
    if (exp<INT_MIN || exp>INT_MAX) return false;
    candidate.exponent=static_cast<int>(exp); candidate.sign=a.sign*b.sign;
    candidate.length=a.length+b.length; if (candidate.length>CAPACITY) candidate.length=CAPACITY;
    if (!candidate.normalize()) return false; result=candidate; return true;
}
int CsgDyadic::getSign() const { return sign; }
bool CsgDyadic::equals(const CsgDyadic& other) const
{
    if (sign!=other.sign || exponent!=other.exponent || length!=other.length) return false;
    for (int i=0;i<length;++i) if (limbs[i]!=other.limbs[i]) return false;
    return true;
}
void CsgDyadic::negate() { sign=-sign; }
