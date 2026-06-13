/**
This file was written by Alexander Enzmann.  He wrote the code for
4th-6th order shapes and generously provided us these enhancements.
*/

#include "java/lang/Math.h"
#include "numericalAnalysis/polynomial/CubicSolver.h"
#include "numericalAnalysis/polynomial/PolynomialSolver.h"
#include "numericalAnalysis/polynomial/QuadraticSolver.h"
#include "numericalAnalysis/polynomial/QuarticSolver.h"

int
QuarticSolver::hasDifficultCoefficients(int n, const double *coefficients)
{
    double largestCoefficient = 0.0;
    for (int coefficientIndex = 0; coefficientIndex <= n; coefficientIndex++) {
        if (java::Math::abs(coefficients[coefficientIndex]) > largestCoefficient) {
            largestCoefficient = coefficients[coefficientIndex];
        }
    }

    if (largestCoefficient == 0.0) {
        return 0;
    }

    for (int coefficientIndex = 0; coefficientIndex <= n; coefficientIndex++) {
        if (coefficients[coefficientIndex] != 0.0) {
            if (java::Math::abs(largestCoefficient / coefficients[coefficientIndex]) >
                FUDGE_FACTOR1) {
                return 1;
            }
        }
    }

    return 0;
}

int
QuarticSolver::solve(const double *coefficients, double *roots, double minValue)
{
    double cubic[4];
    double cubicRoots[3];
    double normalizedY;
    double linearAdjustment;
    double auxiliaryRoot;
    double firstThreshold;
    double secondThreshold;
    double normalizedC1;
    double normalizedC2;
    double normalizedC3;
    double normalizedC4;
    double discriminant;
    double quadraticCoefficient;
    double constantTerm;

    if (QuarticSolver::hasDifficultCoefficients(4, coefficients)) {
        if (java::Math::abs(coefficients[0]) < COEFFICIENT_LIMIT) {
            if (java::Math::abs(coefficients[1]) < COEFFICIENT_LIMIT) {
                return QuadraticSolver::solve(&coefficients[2], roots);
            }
            return CubicSolver::solve(&coefficients[1], roots);
        }
        return PolynomialSolver::solvePolynomial(4, coefficients, roots, minValue);
    }

    double normalizedC0 = coefficients[0];
    if (java::Math::abs(normalizedC0) < COEFFICIENT_LIMIT) {
        return CubicSolver::solve(&coefficients[1], roots);
    }
    if (java::Math::abs(coefficients[4]) < COEFFICIENT_LIMIT) {
        return CubicSolver::solve(coefficients, roots);
    }
    if (normalizedC0 != 1.0) {
        normalizedC1 = coefficients[1] / normalizedC0;
        normalizedC2 = coefficients[2] / normalizedC0;
        normalizedC3 = coefficients[3] / normalizedC0;
        normalizedC4 = coefficients[4] / normalizedC0;
    } else {
        normalizedC1 = coefficients[1];
        normalizedC2 = coefficients[2];
        normalizedC3 = coefficients[3];
        normalizedC4 = coefficients[4];
    }

    double leadingCoefficient = 4.0 * normalizedC4;
    cubic[0] = 1.0;
    cubic[1] = -1.0 * normalizedC2;
    cubic[2] = normalizedC1 * normalizedC3 - leadingCoefficient;
    cubic[3] = leadingCoefficient * normalizedC2 - normalizedC1 * normalizedC1 * normalizedC4 -
        normalizedC3 * normalizedC3;
    int rootCount = CubicSolver::solve(&cubic[0], &cubicRoots[0]);
    if (rootCount > 0) {
        normalizedY = cubicRoots[0];
    } else {
        return 0;
    }

    rootCount = 0;
    leadingCoefficient = normalizedC1 / 2.0;
    normalizedY = normalizedY / 2.0;

    firstThreshold = leadingCoefficient * leadingCoefficient - normalizedC2 + (normalizedY * 2.0);
    if (firstThreshold < 0.0) {
        if (firstThreshold > FUDGE_FACTOR2) {
            firstThreshold = 0.0;
        } else {
            return 0;
        }
    }
    if (firstThreshold < FUDGE_FACTOR3) {
        secondThreshold = normalizedY * normalizedY - normalizedC4;
        if (secondThreshold < 0.0) {
            return 0;
        }
        auxiliaryRoot = 0.0;
        linearAdjustment = java::Math::sqrt(secondThreshold);
    } else {
        auxiliaryRoot = java::Math::sqrt(firstThreshold);
        linearAdjustment =
            0.5 * (leadingCoefficient * (normalizedY * 2.0) - normalizedC3) / auxiliaryRoot;
    }

    quadraticCoefficient = -leadingCoefficient - auxiliaryRoot;
    constantTerm = normalizedY + linearAdjustment;
    discriminant = quadraticCoefficient * quadraticCoefficient - 4.0 * constantTerm;
    if (discriminant >= 0.0) {
        discriminant = java::Math::sqrt(discriminant);
        roots[0] = 0.5 * (quadraticCoefficient + discriminant);
        roots[1] = 0.5 * (quadraticCoefficient - discriminant);
        rootCount = 2;
    }

    quadraticCoefficient = quadraticCoefficient + auxiliaryRoot + auxiliaryRoot;
    constantTerm = normalizedY - linearAdjustment;
    discriminant = quadraticCoefficient * quadraticCoefficient - 4.0 * constantTerm;
    if (discriminant >= 0.0) {
        discriminant = java::Math::sqrt(discriminant);
        roots[rootCount++] = 0.5 * (quadraticCoefficient + discriminant);
        roots[rootCount++] = 0.5 * (quadraticCoefficient - discriminant);
    }
    return rootCount;
}
