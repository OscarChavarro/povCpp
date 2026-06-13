#include "java/lang/Math.h"
#include "numericalAnalysis/polynomial/CubicSolver.h"
#include "numericalAnalysis/polynomial/QuadraticSolver.h"

double
CubicSolver::absoluteValue(double x)
{
    return (x < 0.0) ? (0.0 - x) : x;
}

int
CubicSolver::solve(const double *coefficients, double *roots)
{
    double scaleFactor;
    double angle;
    double firstNormalizedCoefficient;
    double secondNormalizedCoefficient;
    double thirdNormalizedCoefficient;
    double leadingCoefficient = coefficients[0];
    if (leadingCoefficient == 0.0) {
        return QuadraticSolver::solve(&coefficients[1], roots);
    }
    if (leadingCoefficient != 1.0) {
        firstNormalizedCoefficient = coefficients[1] / leadingCoefficient;
        secondNormalizedCoefficient = coefficients[2] / leadingCoefficient;
        thirdNormalizedCoefficient = coefficients[3] / leadingCoefficient;
    } else {
        firstNormalizedCoefficient = coefficients[1];
        secondNormalizedCoefficient = coefficients[2];
        thirdNormalizedCoefficient = coefficients[3];
    }
    double firstCoefficientSquared = firstNormalizedCoefficient * firstNormalizedCoefficient;
    double quadraticTerm = (firstCoefficientSquared - 3.0 * secondNormalizedCoefficient) / 9.0;
    double cubicTerm = (2.0 * firstCoefficientSquared * firstNormalizedCoefficient -
        9.0 * firstNormalizedCoefficient * secondNormalizedCoefficient +
        27.0 * thirdNormalizedCoefficient) /
        54.0;
    double quadraticTermCubed = quadraticTerm * quadraticTerm * quadraticTerm;
    double cubicTermSquared = cubicTerm * cubicTerm;
    double discriminant = quadraticTermCubed - cubicTermSquared;
    double offset = firstNormalizedCoefficient / 3.0;
    if (discriminant >= 0.0) {
        discriminant = cubicTerm / java::Math::sqrt(quadraticTermCubed);
        angle = java::Math::acos(discriminant) / 3.0;
        scaleFactor = -2.0 * java::Math::sqrt(quadraticTerm);
        roots[0] = scaleFactor * java::Math::cos(angle) - offset;
        roots[1] = scaleFactor * java::Math::cos(angle + TWO_PI_3) - offset;
        roots[2] = scaleFactor * java::Math::cos(angle + TWO_PI_43) - offset;
        return 3;
    }
    scaleFactor = java::Math::pow(
        java::Math::sqrt(cubicTermSquared - quadraticTermCubed) +
            CubicSolver::absoluteValue(cubicTerm),
        1.0 / 3.0);
    if (cubicTerm < 0) {
        roots[0] = (scaleFactor + quadraticTerm / scaleFactor) - offset;
    } else {
        roots[0] = -(scaleFactor + quadraticTerm / scaleFactor) - offset;
    }
    return 1;
}
