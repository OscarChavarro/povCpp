#include "java/lang/Math.h"
#include "vsdk/toolkit/numericalAnalysis/polynomial/QuadraticSolver.h"
int
QuadraticSolver::solve(const double *coefficients, double *roots)
{
    double quadraticCoefficient = coefficients[0];
    double linearCoefficient = -coefficients[1];
    double constantCoefficient = coefficients[2];
    if (quadraticCoefficient == 0.0) {
        if (linearCoefficient == 0.0) {
            return 0;
        }
        roots[0] = constantCoefficient / linearCoefficient;
        return 1;
    }
    double discriminant = linearCoefficient * linearCoefficient -
        4.0 * quadraticCoefficient * constantCoefficient;
    if (discriminant < 0.0) {
        return 0;
    }
    if (java::Math::abs(discriminant) < COEFFICIENT_LIMIT) {
        roots[0] = 0.5 * linearCoefficient / quadraticCoefficient;
        return 1;
    }
    discriminant = java::Math::sqrt(discriminant);
    double denominator = 2.0 * quadraticCoefficient;
    roots[0] = (linearCoefficient + discriminant) / denominator;
    roots[1] = (linearCoefficient - discriminant) / denominator;
    return 2;
}
