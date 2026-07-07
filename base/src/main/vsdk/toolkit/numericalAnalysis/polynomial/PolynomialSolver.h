#ifndef __POLYNOMIALSOLVER__
#define __POLYNOMIALSOLVER__

class PolynomialSolver {
  public:
    static constexpr int MAX_ORDER = 7;

    PolynomialSolver();
    PolynomialSolver(int order, const double *coefficients);

    int solve(double *roots, double minValue, double epsilon) const;

  private:
    int order;
    double coefficients[MAX_ORDER + 1];

    static constexpr double COEFFICIENT_LIMIT = 1.0e-20;
    static constexpr int MAX_ITERATIONS = 50;
    static constexpr double POLYNOMIAL_MAX_DISTANCE = 1.0e7;

    int polynomialRemainder(
        const PolynomialSolver &divisor, PolynomialSolver *remainder) const;
    int solveByRegulaFalsi(
        double a, double b, double epsilon, double *root) const;
    void bisectRoots(int sequenceLength, const PolynomialSolver *sturmSequence,
        double minimumValue, double maximumValue, int changesAtMinimum,
        int changesAtMaximum, double epsilon, double *roots) const;
    int countSignChanges(
        int sequenceLength, const PolynomialSolver *sturmSequence, double value) const;
    double evaluatePolynomial(double x) const;
    int buildSturmSequence(PolynomialSolver *sturmSequence) const;
    int countVisibleRoots(int sequenceLength, const PolynomialSolver *sturmSequence,
        int *atNegative, int *atPositive) const;
};

#endif
