#ifndef VSDK_CSG_EXACT_ARITHMETIC_H
#define VSDK_CSG_EXACT_ARITHMETIC_H
// Exact comparisons of finite binary64 expressions using dyadic integer limbs.
// Unsupported arithmetic environments or out-of-range intermediates return false.
class CsgExactArithmetic {
private:
    static bool decompose(double value, unsigned long long& significand, int& exponent);
    static bool addProduct(unsigned long long* positive, unsigned long long* negative,
        double a, double b, bool negate);
    static bool addWord(unsigned long long* target, int index, unsigned long long word);
public:
    static bool equalSumProducts(const double* a, const double* b, const double* c, const double* d, int count);
    static bool equalProducts(double a, double b, double c, double d);
    static bool equalAffine(double a, double b, double c, double d, double e, double f);
};
#endif
