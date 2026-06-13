#include "java/lang/Math.h"
#include "numericalAnalysis/polynomial/Polynomial.h"
#include "numericalAnalysis/polynomial/PolynomialSolver.h"

/**
Calculates the modulus of u(x) / v(x) leaving it in r, it
returns 0 if r(x) is a constant.
note: this function assumes the leading coefficient of v
is 1 or -1
*/
int
PolynomialSolver::polynomialRemainder(
    const Polynomial *dividend, const Polynomial *divisor, Polynomial *remainder)
{
    int quotientIndex;
    int coefficientIndex;
    for (int dividendIndex = 0; dividendIndex <= dividend->order; dividendIndex++) {
        remainder->coefficients[dividendIndex] = dividend->coefficients[dividendIndex];
    }
    for (int dividendIndex = dividend->order + 1;
        dividendIndex <= PolynomialConstants::MAX_ORDER;
        dividendIndex++) {
        remainder->coefficients[dividendIndex] = 0.0;
    }

    if (divisor->coefficients[divisor->order] < 0.0) {
        for (quotientIndex = dividend->order - divisor->order - 1;
            quotientIndex >= 0;
            quotientIndex -= 2) {
            remainder->coefficients[quotientIndex] = -remainder->coefficients[quotientIndex];
        }
        for (quotientIndex = dividend->order - divisor->order; quotientIndex >= 0;
            quotientIndex--) {
            for (coefficientIndex = divisor->order + quotientIndex - 1;
                coefficientIndex >= quotientIndex;
                coefficientIndex--) {
                remainder->coefficients[coefficientIndex] =
                    -remainder->coefficients[coefficientIndex] -
                    remainder->coefficients[divisor->order + quotientIndex] *
                        divisor->coefficients[coefficientIndex - quotientIndex];
            }
        }
    } else {
        for (quotientIndex = dividend->order - divisor->order; quotientIndex >= 0;
            quotientIndex--) {
            for (coefficientIndex = divisor->order + quotientIndex - 1;
                coefficientIndex >= quotientIndex;
                coefficientIndex--) {
                remainder->coefficients[coefficientIndex] -=
                    remainder->coefficients[divisor->order + quotientIndex] *
                    divisor->coefficients[coefficientIndex - quotientIndex];
            }
        }
    }

    quotientIndex = divisor->order - 1;
    while (quotientIndex >= 0 &&
        java::Math::abs(remainder->coefficients[quotientIndex]) <
               COEFFICIENT_LIMIT) {
        remainder->coefficients[quotientIndex] = 0.0;
        quotientIndex--;
    }
    remainder->order = (quotientIndex < 0) ? 0 : quotientIndex;
    return (remainder->order);
}

// Build the Sturmian sequence for a Polynomial
int
PolynomialSolver::buildSturmSequence(int order, Polynomial *sturmSequence)
{
    Polynomial *sequenceTerm;

    sturmSequence[0].order = order;
    sturmSequence[1].order = order - 1;

    // Calculate the derivative and normalize the leading coefficient
    double normalizationFactor =
        java::Math::abs(sturmSequence[0].coefficients[order] * order);
    double *normalizedCoefficient = sturmSequence[1].coefficients;
    double *sourceCoefficient = sturmSequence[0].coefficients + 1;
    for (int coefficientIndex = 1; coefficientIndex <= order; coefficientIndex++) {
        *normalizedCoefficient++ = *sourceCoefficient++ * coefficientIndex /
            normalizationFactor;
    }

    // Construct the rest of the Sturm sequence
    for (sequenceTerm = sturmSequence + 2;
        PolynomialSolver::polynomialRemainder(sequenceTerm - 2, sequenceTerm - 1, sequenceTerm);
        sequenceTerm++) {
        // Reverse the sign and normalize
        normalizationFactor = -java::Math::abs(sequenceTerm->coefficients[sequenceTerm->order]);
        for (normalizedCoefficient = &sequenceTerm->coefficients[sequenceTerm->order];
            normalizedCoefficient >= sequenceTerm->coefficients;
            normalizedCoefficient--) {
            *normalizedCoefficient /= normalizationFactor;
        }
    }
    sequenceTerm->coefficients[0] = -sequenceTerm->coefficients[0]; // Reverse the sign
    return (sequenceTerm - sturmSequence);
}

// Find out how many visible intersections there are
int
PolynomialSolver::countVisibleRoots(
    int sequenceLength, const Polynomial *sturmSequence, int *atZero, int *atPositive)
{
    const Polynomial *sequenceTerm;
    double currentValue;
    double lastValue;

    int changesAtPositiveInfinity = 0;
    int changesAtZero = 0;
    // Changes at positive infinity
    lastValue = sturmSequence[0].coefficients[sturmSequence[0].order];
    for (sequenceTerm = sturmSequence + 1; sequenceTerm <= sturmSequence + sequenceLength;
        sequenceTerm++) {
        currentValue = sequenceTerm->coefficients[sequenceTerm->order];
        if (lastValue == 0.0 || lastValue * currentValue < 0) {
            changesAtPositiveInfinity++;
        }
        lastValue = currentValue;
    }

    // Changes at zero
    lastValue = sturmSequence[0].coefficients[0];
    for (sequenceTerm = sturmSequence + 1; sequenceTerm <= sturmSequence + sequenceLength;
        sequenceTerm++) {
        currentValue = sequenceTerm->coefficients[0];
        if (lastValue == 0.0 || lastValue * currentValue < 0) {
            changesAtZero++;
        }
        lastValue = currentValue;
    }

    *atZero = changesAtZero;
    *atPositive = changesAtPositiveInfinity;
    return (changesAtZero - changesAtPositiveInfinity);
}

/**
Return the number of sign changes in the Sturm sequence in
sseq at the value a.
*/
int
PolynomialSolver::countSignChanges(int sequenceLength, const Polynomial *sturmSequence,
    double value)
{
    double currentValue;
    const Polynomial *sequenceTerm;
    int signChanges = 0;
    double lastValue = PolynomialSolver::evaluatePolynomial(
        value, sturmSequence[0].order, sturmSequence[0].coefficients);
    for (sequenceTerm = sturmSequence + 1;
        sequenceTerm <= sturmSequence + sequenceLength;
        sequenceTerm++) {
        currentValue = PolynomialSolver::evaluatePolynomial(
            value, sequenceTerm->order, sequenceTerm->coefficients);
        if (lastValue == 0.0 || lastValue * currentValue < 0) {
            signChanges++;
        }
        lastValue = currentValue;
    }
    return (signChanges);
}

/**
Uses a bisection based on the sturm sequence for the Polynomial
described in sseq to isolate intervals in which roots occur,
the roots are returned in the roots array in order of magnitude.

Note: This routine has one severe bug: When the interval containing the
root [min, max] has a root at one of its endpoints, as well as one
within the interval, the root at the endpoint will be returned rather
than the one inside.
*/
void
PolynomialSolver::bisectRoots(int sequenceLength, const Polynomial *sturmSequence,
    double minimumValue, double maximumValue, int changesAtMinimum,
    int changesAtMaximum, double epsilon, double *roots)
{
    double midpoint;
    int rootsInLeftHalf;
    int rootsInRightHalf;
    int iteration;
    int changesAtMidpoint;
    int rootCountInInterval = changesAtMinimum - changesAtMaximum;

    if (rootCountInInterval == 1) {
        // First try using regula-falsa to find the root
        if (PolynomialSolver::solveByRegulaFalsi(
                sturmSequence->order, sturmSequence->coefficients, minimumValue,
                maximumValue, epsilon, roots)) {
            return;
        } // That failed, so now find it by bisection
        for (iteration = 0; iteration < MAX_ITERATIONS; iteration++) {
            midpoint = (minimumValue + maximumValue) / 2;
            changesAtMidpoint =
                PolynomialSolver::countSignChanges(sequenceLength, sturmSequence, midpoint);
            if (java::Math::abs(midpoint) > epsilon) {
                if (java::Math::abs((maximumValue - minimumValue) / midpoint) < epsilon) {
                    roots[0] = midpoint;
                    return;
                }
            } else if (java::Math::abs(maximumValue - minimumValue) < epsilon) {
                roots[0] = midpoint;
                return;
            } else if ((changesAtMinimum - changesAtMidpoint) == 0) {
                minimumValue = midpoint;
            } else {
                maximumValue = midpoint;
            }
        }
        // Bisection took too long - just return what we got
        roots[0] = midpoint;
        return;
    }

    // There is more than one root in the interval.
    // Bisect to find new intervals
    for (iteration = 0; iteration < MAX_ITERATIONS; iteration++) {
        midpoint = (minimumValue + maximumValue) / 2;
        changesAtMidpoint =
            PolynomialSolver::countSignChanges(sequenceLength, sturmSequence, midpoint);
        rootsInLeftHalf = changesAtMinimum - changesAtMidpoint;
        rootsInRightHalf = changesAtMidpoint - changesAtMaximum;
        if (rootsInLeftHalf != 0 && rootsInRightHalf != 0) {
            PolynomialSolver::bisectRoots(
                sequenceLength, sturmSequence, minimumValue, midpoint, changesAtMinimum,
                changesAtMidpoint, epsilon, roots);
            PolynomialSolver::bisectRoots(
                sequenceLength, sturmSequence, midpoint, maximumValue, changesAtMidpoint,
                changesAtMaximum, epsilon, &roots[rootsInLeftHalf]);
            return;
        }
        if (rootsInLeftHalf == 0) {
            minimumValue = midpoint;
        } else {
            maximumValue = midpoint;
        }
    }

    // Took too long to bisect. Just return what we got
    for (rootsInLeftHalf = changesAtMaximum; rootsInLeftHalf < changesAtMinimum;
        rootsInLeftHalf++) {
        roots[rootsInLeftHalf - changesAtMaximum] = midpoint;
    }
}

double
PolynomialSolver::evaluatePolynomial(double x, int order, const double *coeffs)
{
    double value = coeffs[order];
    for (int coefficientIndex = order - 1; coefficientIndex >= 0; coefficientIndex--) {
        value = value * x + coeffs[coefficientIndex];
    }
    return value;
}

// Close in on a root by using regula-falsa
int
PolynomialSolver::solveByRegulaFalsi(
    int order, const double *coefficients, double a, double b, double epsilon, double *root)
{
    double estimatedRoot;
    double valueAtRoot;
    double valueAtA = PolynomialSolver::evaluatePolynomial(a, order, coefficients);
    double valueAtB = PolynomialSolver::evaluatePolynomial(b, order, coefficients);

    if (valueAtA * valueAtB > 0.0) {
        return 0;
    }

    if (java::Math::abs(valueAtA) < COEFFICIENT_LIMIT) {
        *root = a;
        return 1;
    }

    if (java::Math::abs(valueAtB) < COEFFICIENT_LIMIT) {
        *root = b;
        return 1;
    }

    double lastValueAtRoot = valueAtA;
    for (int iteration = 0; iteration < MAX_ITERATIONS; iteration++) {
        estimatedRoot = (valueAtB * a - valueAtA * b) / (valueAtB - valueAtA);
        valueAtRoot = PolynomialSolver::evaluatePolynomial(
            estimatedRoot, order, coefficients);

        if (java::Math::abs(estimatedRoot) > epsilon) {
            if (java::Math::abs(valueAtRoot / estimatedRoot) < epsilon) {
                *root = estimatedRoot;
                return 1;
            }
        } else if (java::Math::abs(valueAtRoot) < epsilon) {
            *root = estimatedRoot;
            return 1;
        }

        if (valueAtA < 0) {
            if (valueAtRoot < 0) {
                a = estimatedRoot;
                valueAtA = valueAtRoot;
                if ((lastValueAtRoot * valueAtRoot) > 0) {
                    valueAtB /= 2;
                }
            } else {
                b = estimatedRoot;
                valueAtB = valueAtRoot;
                if ((lastValueAtRoot * valueAtRoot) > 0) {
                    valueAtA /= 2;
                }
            }
        } else if (valueAtRoot < 0) {
            b = estimatedRoot;
            valueAtB = valueAtRoot;
            if ((lastValueAtRoot * valueAtRoot) > 0) {
                valueAtA /= 2;
            }
        } else {
            a = estimatedRoot;
            valueAtA = valueAtRoot;
            if ((lastValueAtRoot * valueAtRoot) > 0) {
                valueAtB /= 2;
            }
        }
        if (java::Math::abs(b - a) < epsilon) {
            // Check for underflow in the domain
            *root = estimatedRoot;
            return 1;
        }
        lastValueAtRoot = valueAtRoot;
    }
    return 0;
}

// Root solver based on the Sturm sequences for a Polynomial
int
PolynomialSolver::solvePolynomial(
    int order, const double *coefficients, double *roots, double minValue, double epsilon)
{
    Polynomial sturmSequence[PolynomialConstants::MAX_ORDER + 1];
    int rootCount;
    int changesAtMinimum;
    int changesAtMaximum;

    // Put the coefficients into the top of the stack
    for (int coefficientIndex = 0; coefficientIndex <= order; coefficientIndex++) {
        sturmSequence[0].coefficients[order - coefficientIndex] =
            coefficients[coefficientIndex];
    }

    // Build the Sturm sequence
    int sequenceLength = PolynomialSolver::buildSturmSequence(order, &sturmSequence[0]);

    // Get the total number of visible roots
    rootCount = PolynomialSolver::countVisibleRoots(
        sequenceLength, sturmSequence, &changesAtMinimum, &changesAtMaximum);
    if (rootCount == 0) {
        return 0;
    }

    // Bracket the roots
    double maximumValue = POLYNOMIAL_MAX_DISTANCE;

    changesAtMinimum = PolynomialSolver::countSignChanges(
        sequenceLength, sturmSequence, minValue);
    changesAtMaximum = PolynomialSolver::countSignChanges(
        sequenceLength, sturmSequence, maximumValue);
    rootCount = changesAtMinimum - changesAtMaximum;
    if (rootCount == 0) {
        return 0;
    }

    // Perform the bisection
    PolynomialSolver::bisectRoots(
        sequenceLength, sturmSequence, minValue, maximumValue, changesAtMinimum,
        changesAtMaximum, epsilon, roots);

    return rootCount;
}
