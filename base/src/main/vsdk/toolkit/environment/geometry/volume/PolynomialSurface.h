#ifndef VSDK_POLYNOMIAL_SURFACE_H
#define VSDK_POLYNOMIAL_SURFACE_H

#include "vsdk/toolkit/environment/geometry/volume/ImplicitSolid.h"

// Implicit solid F(x,y,z) < 0 of a polynomial of order 2 to 7 in x, y and z.
// Coefficients are stored in the PovRay term order: decreasing powers of x,
// then of y, then of z, so a quartic's 35 terms run from x^4 (index 0) down to
// the constant (index 34). getTermIndex and getTermPowers convert between term
// indices and powers. Ray roots come from the Sturm-sequence solver, or for
// quartics from the closed-form solver unless Sturm sequences are requested.
// The surface is generally unbounded; bound it by composition when needed.
class PolynomialSurface : public ImplicitSolid {
public:
    static constexpr int MINIMUM_ORDER = 2;
    static constexpr int MAXIMUM_ORDER = 7;
    static constexpr int MAXIMUM_TERMS = 120;
    // Root solver tolerance of PovRay polynomial shapes.
    static constexpr double SOLVER_EPSILON = 1.0e-10;
    // PovRay drops expanded ray coefficients of magnitude below this value.
    static constexpr double POVRAY_EXPANSION_CUTOFF = 1.0e-4;
private:
    static const int TERM_COUNTS[MAXIMUM_ORDER + 1];
    static const int BINOMIAL[11][12];
    static const int FACTORIALS[MAXIMUM_ORDER + 1];
    int order;
    bool sturm;
    double coefficients[MAXIMUM_TERMS];
    static int rollTerm(int order, int x, int y, int z);
    static void transform(int order, double* coeffs, Matrix4x4d* q, double cutoff);
    static double partialTerm(const Matrix4x4d* q, int row, int power, int i, int j, int k, int l);
    int intersectGeneral(const Vector3Dd& origin, const Vector3Dd& direction,
        double minimumDistance, double* depths, double cutoff) const;
    int intersectQuartic(const Vector3Dd& origin, const Vector3Dd& direction,
        double minimumDistance, double* depths) const;
    Vector3Dd quarticGradient(const Vector3Dd& point) const;
    static FloatingInterval power(const FloatingInterval& value, int exponent);
public:
    // All coefficients start at zero. Throws java::IllegalArgumentException
    // for an order outside [MINIMUM_ORDER, MAXIMUM_ORDER].
    PolynomialSurface(int order, bool useSturmSequences);
    static bool isValidOrder(int order);
    static int getTermCount(int order);
    // Term index of x^xPower y^yPower z^zPower, or -1 when the powers are
    // negative or add up to more than order.
    static int getTermIndex(int order, int xPower, int yPower, int zPower);
    // Powers of term index; constantPower is order minus the other three.
    static void getTermPowers(int order, int index, int& xPower, int& yPower,
        int& zPower, int& constantPower);
    int getOrder() const { return order; }
    bool usesSturmSequences() const { return sturm; }
    double* getCoefficients() { return coefficients; }
    const double* getCoefficients() const { return coefficients; }
    // PovRay ray roots: distances d >= minimumDistance along the unit direction
    // where the surface is crossed, in solver order; returns their count (at
    // most order). Orders other than 4 drop expanded coefficients below
    // POVRAY_EXPANSION_CUTOFF; the typed queries keep them.
    int intersect(const Vector3Dd& origin, const Vector3Dd& unitDirection,
        double minimumDistance, double* depths) const;
    // Coefficients of t^order down to t^0 of F(origin + t*direction), by
    // general term expansion, zeroing those below cutoff in magnitude; the
    // quartic solver path uses its own expansion.
    void getRayPolynomial(const Vector3Dd& origin, const Vector3Dd& direction,
        double* result, double cutoff) const;
    void getQuarticRayPolynomial(const Vector3Dd& origin, const Vector3Dd& direction,
        double* result) const;
    double evaluate(const Vector3Dd& point) const;
    Vector3Dd gradient(const Vector3Dd& point) const;

    Geometry* copyGeometry() const override;
    double evaluateField(const Vector3Dd& point) const override { return evaluate(point); }
    Vector3Dd evaluateFieldGradient(const Vector3Dd& point) const override { return gradient(point); }
    FloatingInterval encloseField(const FloatingInterval& x, const FloatingInterval& y,
        const FloatingInterval& z) const override;
    bool encloseFieldGradient(const FloatingInterval& x, const FloatingInterval& y,
        const FloatingInterval& z, FloatingInterval& gx, FloatingInterval& gy,
        FloatingInterval& gz) const override;
    bool findRayRoots(const Vector3Dd& origin, const Vector3Dd& unitDirection,
        double maximumDistance, java::ArrayList<double>& distances) const override;
    void getBounds(GeometryBounds& result) const override;
};
#endif
