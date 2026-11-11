#ifndef __FLOATING_INTERVAL__
#define __FLOATING_INTERVAL__

// Closed binary64 enclosure with one-ULP outward rounding per operation.
// Invalid inputs, overflow and unsupported floating-point environments produce
// an invalid interval. The implementation requires IEEE-754 binary64,
// round-to-nearest, gradual underflow and correctly rounded elementary sqrt.
class FloatingInterval {
private:
    double lower;
    double upper;
    bool valid;
    FloatingInterval(double lowerValue, double upperValue, bool isValid);
    static FloatingInterval rounded(double low, double high);
public:
    FloatingInterval();
    static bool isSupportedEnvironment();
    static FloatingInterval exact(double value);
    bool isValid() const { return valid; }
    double getLower() const { return lower; }
    double getUpper() const { return upper; }
    FloatingInterval add(const FloatingInterval& other) const;
    FloatingInterval subtract(const FloatingInterval& other) const;
    FloatingInterval multiply(const FloatingInterval& other) const;
    FloatingInterval divide(const FloatingInterval& other) const;
    FloatingInterval square() const;
    FloatingInterval squareRoot() const;
};

#endif
