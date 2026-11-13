#ifndef VSDK_CSG_DYADIC_H
#define VSDK_CSG_DYADIC_H
// Signed exact dyadic number. Fixed-capacity binary limbs avoid heap allocation;
// every operation reports failure and leaves its output unchanged on overflow.
class CsgDyadic {
private:
    static const int CAPACITY=1100;
    unsigned int limbs[CAPACITY];
    int length;
    int exponent;
    int sign;
    bool normalize();
    static bool shifted(const CsgDyadic& value, int exponent, unsigned int* result);
public:
    CsgDyadic();
    static bool fromDouble(double value, CsgDyadic& result);
    static bool add(const CsgDyadic& a, const CsgDyadic& b, CsgDyadic& result);
    static bool subtract(const CsgDyadic& a, const CsgDyadic& b, CsgDyadic& result);
    static bool multiply(const CsgDyadic& a, const CsgDyadic& b, CsgDyadic& result);
    int getSign() const;
    bool equals(const CsgDyadic& other) const;
    void negate();
};
#endif
