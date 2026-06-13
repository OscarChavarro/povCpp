#include "java/lang/Math.h"
#include "vsdk/toolkit/common/numericalAnalysis/polynomial/PolynomialSolver.h"

PolynomialSolver::PolynomialSolver() : order(0), coefficients{0.0}
{
}

PolynomialSolver::PolynomialSolver(int order, const double *coefficients) : order(order)
{
    for (int coefficientIndex = 0; coefficientIndex <= MAX_ORDER; coefficientIndex++) {
        this->coefficients[coefficientIndex] = 0.0;
    }
    for (int coefficientIndex = 0; coefficientIndex <= order; coefficientIndex++) {
        this->coefficients[order - coefficientIndex] = coefficients[coefficientIndex];
    }
}

/**
Calculates the modulus of u(x) / v(x) leaving it in r, it
returns 0 if r(x) is a constant.
note: this function assumes the leading coefficient of v
is 1 or -1
*/
int
PolynomialSolver::polynomialRemainder(
    const PolynomialSolver &divisor, PolynomialSolver *remainder) const
{
    int quotientIndex;
    int coefficientIndex;
    for (int dividendIndex = 0; dividendIndex <= order; dividendIndex++) {
        remainder->coefficients[dividendIndex] = coefficients[dividendIndex];
    }
    for (int dividendIndex = order + 1; dividendIndex <= MAX_ORDER; dividendIndex++) {
        remainder->coefficients[dividendIndex] = 0.0;
    }

    if (divisor.coefficients[divisor.order] < 0.0) {
        for (quotientIndex = order - divisor.order - 1; quotientIndex >= 0;
            quotientIndex -= 2) {
            remainder->coefficients[quotientIndex] = -remainder->coefficients[quotientIndex];
        }
        for (quotientIndex = order - divisor.order; quotientIndex >= 0;
            quotientIndex--) {
            for (coefficientIndex = divisor.order + quotientIndex - 1;
                coefficientIndex >= quotientIndex;
                coefficientIndex--) {
                remainder->coefficients[coefficientIndex] =
                    -remainder->coefficients[coefficientIndex] -
                    remainder->coefficients[divisor.order + quotientIndex] *
                        divisor.coefficients[coefficientIndex - quotientIndex];
            }
        }
    } else {
        for (quotientIndex = order - divisor.order; quotientIndex >= 0;
            quotientIndex--) {
            for (coefficientIndex = divisor.order + quotientIndex - 1;
                coefficientIndex >= quotientIndex;
                coefficientIndex--) {
                remainder->coefficients[coefficientIndex] -=
                    remainder->coefficients[divisor.order + quotientIndex] *
                    divisor.coefficients[coefficientIndex - quotientIndex];
            }
        }
    }

    quotientIndex = divisor.order - 1;
    while (quotientIndex >= 0 &&
        java::Math::abs(remainder->coefficients[quotientIndex]) <
               COEFFICIENT_LIMIT) {
        remainder->coefficients[quotientIndex] = 0.0;
        quotientIndex--;
    }
    remainder->order = (quotientIndex < 0) ? 0 : quotientIndex;
    return remainder->order;
}

/**
Build the Sturmian sequence for a Polynomial
*/
int
PolynomialSolver::buildSturmSequence(PolynomialSolver *sturmSequence) const
{
    PolynomialSolver *sequenceTerm;

    sturmSequence[0] = *this;
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
        (sequenceTerm - 2)->polynomialRemainder(*(sequenceTerm - 1), sequenceTerm);
        sequenceTerm++) {
        // Reverse the sign and normalize
        normalizationFactor = -java::Math::abs(sequenceTerm->coefficients[sequenceTerm->order]);
        for (normalizedCoefficient = &sequenceTerm->coefficients[sequenceTerm->order];
            normalizedCoefficient >= sequenceTerm->coefficients;
            normalizedCoefficient--) {
            *normalizedCoefficient /= normalizationFactor;
        }
    }
    sequenceTerm->coefficients[0] = -sequenceTerm->coefficients[0];
    return sequenceTerm - sturmSequence;
}

/**
Find out how many visible intersections there are
*/
int
PolynomialSolver::countVisibleRoots(
    int sequenceLength, const PolynomialSolver *sturmSequence, int *atZero,
    int *atPositive) const
{
    const PolynomialSolver *sequenceTerm;
    double currentValue;
    double lastValue;
    int changesAtPositiveInfinity = 0;
    int changesAtZero = 0;

    lastValue = sturmSequence[0].coefficients[sturmSequence[0].order];
    for (sequenceTerm = sturmSequence + 1; sequenceTerm <= sturmSequence + sequenceLength;
        sequenceTerm++) {
        currentValue = sequenceTerm->coefficients[sequenceTerm->order];
        if (lastValue == 0.0 || lastValue * currentValue < 0) {
            changesAtPositiveInfinity++;
        }
        lastValue = currentValue;
    }

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
    return changesAtZero - changesAtPositiveInfinity;
}

/**
Return the number of sign changes in the Sturm sequence in
sseq at the value a.
*/
int
PolynomialSolver::countSignChanges(
    int sequenceLength, const PolynomialSolver *sturmSequence, double value) const
{
    double currentValue;
    const PolynomialSolver *sequenceTerm;
    int signChanges = 0;
    double lastValue = sturmSequence[0].evaluatePolynomial(value);
    for (sequenceTerm = sturmSequence + 1;
        sequenceTerm <= sturmSequence + sequenceLength;
        sequenceTerm++) {
        currentValue = sequenceTerm->evaluatePolynomial(value);
        if (lastValue == 0.0 || lastValue * currentValue < 0) {
            signChanges++;
        }
        lastValue = currentValue;
    }
    return signChanges;
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
PolynomialSolver::bisectRoots(
    int sequenceLength, const PolynomialSolver *sturmSequence,
    double minimumValue, double maximumValue, int changesAtMinimum,
    int changesAtMaximum, double epsilon, double *roots) const
{
    double midpoint;
    int rootsInLeftHalf;
    int rootsInRightHalf;
    int iteration;
    int changesAtMidpoint;
    int rootCountInInterval = changesAtMinimum - changesAtMaximum;

    if (rootCountInInterval == 1) {
        if (sturmSequence->solveByRegulaFalsi(minimumValue, maximumValue, epsilon, roots)) {
            return;
        }

        for (iteration = 0; iteration < MAX_ITERATIONS; iteration++) {
            midpoint = (minimumValue + maximumValue) / 2;
            changesAtMidpoint =
                countSignChanges(sequenceLength, sturmSequence, midpoint);
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

        roots[0] = midpoint;
        return;
    }

    for (iteration = 0; iteration < MAX_ITERATIONS; iteration++) {
        midpoint = (minimumValue + maximumValue) / 2;
        changesAtMidpoint =
            countSignChanges(sequenceLength, sturmSequence, midpoint);
        rootsInLeftHalf = changesAtMinimum - changesAtMidpoint;
        rootsInRightHalf = changesAtMidpoint - changesAtMaximum;
        if (rootsInLeftHalf != 0 && rootsInRightHalf != 0) {
            bisectRoots(sequenceLength, sturmSequence, minimumValue, midpoint,
                changesAtMinimum, changesAtMidpoint, epsilon, roots);
            bisectRoots(sequenceLength, sturmSequence, midpoint, maximumValue,
                changesAtMidpoint, changesAtMaximum, epsilon,
                &roots[rootsInLeftHalf]);
            return;
        }
        if (rootsInLeftHalf == 0) {
            minimumValue = midpoint;
        } else {
            maximumValue = midpoint;
        }
    }

    for (rootsInLeftHalf = changesAtMaximum; rootsInLeftHalf < changesAtMinimum;
        rootsInLeftHalf++) {
        roots[rootsInLeftHalf - changesAtMaximum] = midpoint;
    }
}

double
PolynomialSolver::evaluatePolynomial(double x) const
{
    double value = coefficients[order];
    for (int coefficientIndex = order - 1; coefficientIndex >= 0; coefficientIndex--) {
        value = value * x + coefficients[coefficientIndex];
    }
    return value;
}

/**
Close in on a root by using regula-falsa
*/
int
PolynomialSolver::solveByRegulaFalsi(
    double a, double b, double epsilon, double *root) const
{
    double estimatedRoot;
    double valueAtRoot;
    double valueAtA = evaluatePolynomial(a);
    double valueAtB = evaluatePolynomial(b);

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
        valueAtRoot = evaluatePolynomial(estimatedRoot);

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
            *root = estimatedRoot;
            return 1;
        }
        lastValueAtRoot = valueAtRoot;
    }
    return 0;
}

/**
Root solver based on the Sturm sequences for a Polynomial
*/
int
PolynomialSolver::solve(double *roots, double minValue, double epsilon) const
{
    PolynomialSolver sturmSequence[MAX_ORDER + 1];
    int rootCount;
    int changesAtMinimum;
    int changesAtMaximum;

    int sequenceLength = buildSturmSequence(&sturmSequence[0]);

    rootCount = countVisibleRoots(
        sequenceLength, sturmSequence, &changesAtMinimum, &changesAtMaximum);
    if (rootCount == 0) {
        return 0;
    }

    double maximumValue = POLYNOMIAL_MAX_DISTANCE;

    changesAtMinimum = countSignChanges(sequenceLength, sturmSequence, minValue);
    changesAtMaximum = countSignChanges(sequenceLength, sturmSequence, maximumValue);
    rootCount = changesAtMinimum - changesAtMaximum;
    if (rootCount == 0) {
        return 0;
    }

    bisectRoots(sequenceLength, sturmSequence, minValue, maximumValue, changesAtMinimum,
        changesAtMaximum, epsilon, roots);

    return rootCount;
}
