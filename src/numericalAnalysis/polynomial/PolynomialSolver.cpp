/**
This file was written by Alexander Enzmann.  He wrote the code for
4th-6th order shapes and generously provided us these enhancements.
*/

#include "java/lang/Math.h"
#include "numericalAnalysis/polynomial/Polynomial.h"
#include "numericalAnalysis/polynomial/PolynomialSolver.h"

#undef EPSILON
static constexpr double EPSILON = 1.0e-10;
static constexpr double COEFF_LIMIT = 1.0e-20;

/**
WARNING      WARNING     WARNING

The following three constants have been defined so that quartic equations
will properly render on fpu/compiler combinations that only have 64 bit
IEEE precision.  Do not arbitrarily change any of these values.

If you have a machine with a proper fpu/compiler combo - like a Mac with
a 68881, then use the native floating format (96 bits) and tune the
values for a little less tolerance: something like: factor1 = 1.0e15,
factor2 = -1.0e-7, factor3 = 1.0e-10.

The meaning of the three constants are:
    factor1 - the magnitude of difference between coefficients in a
                 quartic equation at which the Sturmian root solver will
                 kick in.  The Sturm solver is quite a bit slower than
                 the algebraic solver, so the bigger this is, the faster
                 the root solving will go.  The algebraic solver is less
                 accurate so the bigger this is, the more likely you will
                 get bad roots.

    factor2 - Tolerance value that defines how close the quartic equation
                 is to being a square of a quadratic.  The closer this can
                 get to zero before roots disappear, the less the chance
                 you will get spurious roots.

    factor3 - Similar to factor2 at a later stage of the algebraic solver.
*/
static constexpr double FUDGE_FACTOR1 = 1.0e11;
static constexpr double FUDGE_FACTOR2 = -1.0e-5;
static constexpr double FUDGE_FACTOR3 = 1.0e-7;

inline double
PolynomialSolver::absoluteValue(double x)
{
    return (x < 0.0) ? (0.0 - x) : x;
}

static constexpr double TWO_PI_3 = 2.0943951023931954923084;
static constexpr double TWO_PI_43 = 4.1887902047863909846168;
static constexpr int MAX_ITERATIONS = 50;

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
    int dividendIndex;
    int quotientIndex;
    int coefficientIndex;
    for (dividendIndex = 0; dividendIndex <= dividend->order; dividendIndex++) {
        remainder->coefficients[dividendIndex] = dividend->coefficients[dividendIndex];
    }
    for (dividendIndex = dividend->order + 1;
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
        java::Math::abs(remainder->coefficients[quotientIndex]) < COEFF_LIMIT) {
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
    int coefficientIndex;
    double normalizationFactor;
    double *normalizedCoefficient;
    double *sourceCoefficient;
    Polynomial *sequenceTerm;

    sturmSequence[0].order = order;
    sturmSequence[1].order = order - 1;

    // Calculate the derivative and normalize the leading coefficient
    normalizationFactor =
        java::Math::abs(sturmSequence[0].coefficients[order] * order);
    normalizedCoefficient = sturmSequence[1].coefficients;
    sourceCoefficient = sturmSequence[0].coefficients + 1;
    for (coefficientIndex = 1; coefficientIndex <= order; coefficientIndex++) {
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
    int changesAtPositiveInfinity;
    int changesAtZero;
    const Polynomial *sequenceTerm;
    double currentValue;
    double lastValue;

    changesAtPositiveInfinity = changesAtZero = 0;
    // Changes at positve infinity
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
    int signChanges;
    double currentValue;
    double lastValue;
    const Polynomial *sequenceTerm;
    signChanges = 0;
    lastValue = PolynomialSolver::evaluatePolynomial(
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
    int changesAtMaximum, double *roots)
{
    double midpoint;
    int rootsInLeftHalf;
    int rootsInRightHalf;
    int iteration;
    int changesAtMidpoint;
    int rootCountInInterval;

    if ((rootCountInInterval = changesAtMinimum - changesAtMaximum) == 1) {
        // First try using regula-falsa to find the root
        if (PolynomialSolver::solveByRegulaFalsi(
                sturmSequence->order, sturmSequence->coefficients, minimumValue,
                maximumValue, roots)) {
            return;
        } // That failed, so now find it by bisection
        for (iteration = 0; iteration < MAX_ITERATIONS; iteration++) {
            midpoint = (minimumValue + maximumValue) / 2;
            changesAtMidpoint =
                PolynomialSolver::countSignChanges(sequenceLength, sturmSequence, midpoint);
            if (java::Math::abs(midpoint) > EPSILON) {
                if (java::Math::abs((maximumValue - minimumValue) / midpoint) < EPSILON) {
                    roots[0] = midpoint;
                    return;
                }
            } else if (java::Math::abs(maximumValue - minimumValue) < EPSILON) {
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
                changesAtMidpoint, roots);
            PolynomialSolver::bisectRoots(
                sequenceLength, sturmSequence, midpoint, maximumValue, changesAtMidpoint,
                changesAtMaximum, &roots[rootsInLeftHalf]);
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
    int coefficientIndex;
    double value;
    value = coeffs[order];
    for (coefficientIndex = order - 1; coefficientIndex >= 0; coefficientIndex--) {
        value = value * x + coeffs[coefficientIndex];
    }
    return value;
}

// Close in on a root by using regula-falsa
int
PolynomialSolver::solveByRegulaFalsi(
    int order, const double *coefficients, double a, double b, double *root)
{
    int iteration;
    double valueAtA;
    double valueAtB;
    double estimatedRoot;
    double valueAtRoot;
    double lastValueAtRoot;

    valueAtA = PolynomialSolver::evaluatePolynomial(a, order, coefficients);
    valueAtB = PolynomialSolver::evaluatePolynomial(b, order, coefficients);

    if (valueAtA * valueAtB > 0.0) {
        return 0;
    }

    if (java::Math::abs(valueAtA) < COEFF_LIMIT) {
        *root = a;
        return 1;
    }

    if (java::Math::abs(valueAtB) < COEFF_LIMIT) {
        *root = b;
        return 1;
    }

    lastValueAtRoot = valueAtA;
    for (iteration = 0; iteration < MAX_ITERATIONS; iteration++) {
        estimatedRoot = (valueAtB * a - valueAtA * b) / (valueAtB - valueAtA);
        valueAtRoot = PolynomialSolver::evaluatePolynomial(
            estimatedRoot, order, coefficients);

        if (java::Math::abs(estimatedRoot) > EPSILON) {
            if (java::Math::abs(valueAtRoot / estimatedRoot) < EPSILON) {
                *root = estimatedRoot;
                return 1;
            }
        } else if (java::Math::abs(valueAtRoot) < EPSILON) {
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
        if (java::Math::abs(b - a) < EPSILON) {
            // Check for underflow in the domain
            *root = estimatedRoot;
            return 1;
        }
        lastValueAtRoot = valueAtRoot;
    }
    return 0;
}

/**
Solve the quadratic equation:
                 x[0] * x^2 + x[1] * x + x[2] = 0.

The value returned by this function is the number of real roots.
The roots themselves are returned in y[0], y[1].
*/
int
PolynomialSolver::solveQuadratic(const double *x, double *y)
{
    double discriminant;
    double denominator;
    double quadraticCoefficient;
    double linearCoefficient;
    double constantCoefficient;
    quadraticCoefficient = x[0];
    linearCoefficient = -x[1];
    constantCoefficient = x[2];
    if (quadraticCoefficient == 0.0) {
        if (linearCoefficient == 0.0) {
            return 0;
        }
        y[0] = constantCoefficient / linearCoefficient;
        return 1;
    }
    discriminant = linearCoefficient * linearCoefficient -
        4.0 * quadraticCoefficient * constantCoefficient;
    if (discriminant < 0.0) {
        return 0;
    }
    if (java::Math::abs(discriminant) < COEFF_LIMIT) {
        y[0] = 0.5 * linearCoefficient / quadraticCoefficient;
        return 1;
    }
    discriminant = java::Math::sqrt(discriminant);
    denominator = 2.0 * quadraticCoefficient;
    y[0] = (linearCoefficient + discriminant) / denominator;
    y[1] = (linearCoefficient - discriminant) / denominator;
    return 2;
}

/**
Solve the cubic equation:

    x[0] * x^3 + x[1] * x^2 + x[2] * x + x[3] = 0.

The result of this function is an integer that tells how many real
roots exist.  Determination of how many are distinct is up to the
process that calls this routine.  The roots that exist are stored
in (y[0], y[1], y[2]).

Note: this function relies very heavily on trigonometric functions and
the square root function.  If an alternative solution is found that does
not rely on transcendentals this code will be replaced.
*/
int
PolynomialSolver::solveCubic(const double *x, double *y)
{
    double quadraticTerm;
    double cubicTerm;
    double quadraticTermCubed;
    double cubicTermSquared;
    double scaleFactor;
    double discriminant;
    double offset;
    double angle;
    double leadingCoefficient;
    double firstNormalizedCoefficient;
    double secondNormalizedCoefficient;
    double thirdNormalizedCoefficient;
    double firstCoefficientSquared;
    leadingCoefficient = x[0];
    if (leadingCoefficient == 0.0) {
        return PolynomialSolver::solveQuadratic(&x[1], y);
    }
    if (leadingCoefficient != 1.0) {
        firstNormalizedCoefficient = x[1] / leadingCoefficient;
        secondNormalizedCoefficient = x[2] / leadingCoefficient;
        thirdNormalizedCoefficient = x[3] / leadingCoefficient;
    } else {
        firstNormalizedCoefficient = x[1];
        secondNormalizedCoefficient = x[2];
        thirdNormalizedCoefficient = x[3];
    }
    firstCoefficientSquared = firstNormalizedCoefficient * firstNormalizedCoefficient;
    quadraticTerm = (firstCoefficientSquared - 3.0 * secondNormalizedCoefficient) / 9.0;
    cubicTerm = (2.0 * firstCoefficientSquared * firstNormalizedCoefficient -
        9.0 * firstNormalizedCoefficient * secondNormalizedCoefficient +
        27.0 * thirdNormalizedCoefficient) /
        54.0;
    quadraticTermCubed = quadraticTerm * quadraticTerm * quadraticTerm;
    cubicTermSquared = cubicTerm * cubicTerm;
    discriminant = quadraticTermCubed - cubicTermSquared;
    offset = firstNormalizedCoefficient / 3.0;
    if (discriminant >= 0.0) {
        // Three real roots
        discriminant = cubicTerm / java::Math::sqrt(quadraticTermCubed);
        angle = java::Math::acos(discriminant) / 3.0;
        scaleFactor = -2.0 * java::Math::sqrt(quadraticTerm);
        y[0] = scaleFactor * java::Math::cos(angle) - offset;
        y[1] = scaleFactor * java::Math::cos(angle + TWO_PI_3) - offset;
        y[2] = scaleFactor * java::Math::cos(angle + TWO_PI_43) - offset;
        return 3;
    }
    scaleFactor = java::Math::pow(
        java::Math::sqrt(cubicTermSquared - quadraticTermCubed) +
            PolynomialSolver::absoluteValue(cubicTerm),
        1.0 / 3.0);
    if (cubicTerm < 0) {
        y[0] = (scaleFactor + quadraticTerm / scaleFactor) - offset;
    } else {
        y[0] = -(scaleFactor + quadraticTerm / scaleFactor) - offset;
    }
    return 1;
}

/**
Test to see if any coeffs are more than 6 orders of magnitude
larger than the smallest
*/
int
PolynomialSolver::hasDifficultCoefficients(int n, const double *coefficients)
{
    int coefficientIndex;
    double largestCoefficient;

    largestCoefficient = 0.0;
    for (coefficientIndex = 0; coefficientIndex <= n; coefficientIndex++) {
        if (java::Math::abs(coefficients[coefficientIndex]) > largestCoefficient) {
            largestCoefficient = coefficients[coefficientIndex];
        }
    }

    // Everything is zero no sense in doing any more
    if (largestCoefficient == 0.0) {
        return 0;
    }

    for (coefficientIndex = 0; coefficientIndex <= n; coefficientIndex++) {
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
PolynomialSolver::solveQuartic(const double *x, double *results, double minValue)
{
    double cubic[4];
    double roots[3];
    double leadingCoefficient;
    double normalizedY;
    double linearAdjustment;
    double auxiliaryRoot;
    double firstThreshold;
    double secondThreshold;
    double normalizedC0;
    double normalizedC1;
    double normalizedC2;
    double normalizedC3;
    double normalizedC4;
    double discriminant;
    double quadraticCoefficient;
    double constantTerm;
    int rootCount;

    // Figure out the size difference between coefficients
    if (PolynomialSolver::hasDifficultCoefficients(4, x)) {
        if (java::Math::abs(x[0]) < COEFF_LIMIT) {
            if (java::Math::abs(x[1]) < COEFF_LIMIT) {
                return PolynomialSolver::solveQuadratic(&x[2], results);
            }
            return PolynomialSolver::solveCubic(&x[1], results);
        }
        return PolynomialSolver::solvePolynomial(4, x, results, minValue);
    }

    normalizedC0 = x[0];
    if (java::Math::abs(normalizedC0) < COEFF_LIMIT) {
        return PolynomialSolver::solveCubic(&x[1], results);
    }
    if (java::Math::abs(x[4]) < COEFF_LIMIT) {
        return PolynomialSolver::solveCubic(x, results);
    }
    if (normalizedC0 != 1.0) {
        normalizedC1 = x[1] / normalizedC0;
        normalizedC2 = x[2] / normalizedC0;
        normalizedC3 = x[3] / normalizedC0;
        normalizedC4 = x[4] / normalizedC0;
    } else {
        normalizedC1 = x[1];
        normalizedC2 = x[2];
        normalizedC3 = x[3];
        normalizedC4 = x[4];
    }

    /**
    The first step is to take the original equation:

        x^4 + b*x^3 + c*x^2 + d*x + e = 0

    and rewrite it as:

        x^4 + b*x^3 = -c*x^2 - d*x - e,

    adding (b*x/2)^2 + (x^2 + b*x/2)y + y^2/4 to each side gives a
    perfect square on the lhs:

        (x^2 + b*x/2 + y/2)^2 = (b^2/4 - c + y)x^2 + (b*y/2 - d)x + y^2/4 - e

    By choosing the appropriate value for y, the rhs can be made a perfect
    square also.  This value is found when the rhs is treated as a quadratic
    in x with the discriminant equal to 0.  This will be true when:

        (b*y/2 - d)^2 - 4.0 * (b^2/4 - c*y)*(y^2/4 - e) = 0, or

        y^3 - c*y^2 + (b*d - 4*e)*y - b^2*e + 4*c*e - d^2 = 0.

    This is called the resolvent of the quartic equation.
    */

    leadingCoefficient = 4.0 * normalizedC4;
    cubic[0] = 1.0;
    cubic[1] = -1.0 * normalizedC2;
    cubic[2] = normalizedC1 * normalizedC3 - leadingCoefficient;
    cubic[3] = leadingCoefficient * normalizedC2 - normalizedC1 * normalizedC1 * normalizedC4 -
        normalizedC3 * normalizedC3;
    rootCount = PolynomialSolver::solveCubic(&cubic[0], &roots[0]);
    if (rootCount > 0) {
        normalizedY = roots[0];
    } else {
        return 0;
    }

    /**
    What we are left with is a quadratic squared on the lhs and a
        linear term on the right.  The linear term has one of two signs,
        take each and add it to the lhs.  The form of the quartic is now:

            a' = b^2/4 - c + y,     b' = b*y/2 - d, (from rhs quadritic above)

            (x^2 + b*x/2 + y/2) = +java::Math::sqrt(a'*(x + 1/2 * b'/a')^2), and
            (x^2 + b*x/2 + y/2) = -java::Math::sqrt(a'*(x + 1/2 * b'/a')^2).

        By taking the linear term from each of the right hand sides and
        adding to the appropriate part of the left hand side, two quadratic
        formulas are created.  By solving each of these the four roots of
        the quartic are determined.
    */
    rootCount = 0;
    leadingCoefficient = normalizedC1 / 2.0;
    normalizedY = normalizedY / 2.0;

    firstThreshold = leadingCoefficient * leadingCoefficient - normalizedC2 + (normalizedY * 2.0);
    if (firstThreshold < 0.0) {
        if (firstThreshold > FUDGE_FACTOR2) {
            firstThreshold = 0.0;
        } else {
            // First Special case, a' < 0 means all roots are complex
            return 0;
        }
    }
    if (firstThreshold < FUDGE_FACTOR3) {
        /**
        Second special case, the "x" term on the right hand side above
            has vanished.  In this case:
                     (x^2 + b*x/2 + y/2) = +java::Math::sqrt(y^2/4 - e), and
                     (x^2 + b*x/2 + y/2) = -java::Math::sqrt(y^2/4 - e).
        */
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
    // Solve the first quadratic
    quadraticCoefficient = -leadingCoefficient - auxiliaryRoot;
    constantTerm = normalizedY + linearAdjustment;
    discriminant = quadraticCoefficient * quadraticCoefficient - 4.0 * constantTerm;
    if (discriminant >= 0.0) {
        discriminant = java::Math::sqrt(discriminant);
        results[0] = 0.5 * (quadraticCoefficient + discriminant);
        results[1] = 0.5 * (quadraticCoefficient - discriminant);
        rootCount = 2;
    }
    // Solve the second quadratic
    quadraticCoefficient = quadraticCoefficient + auxiliaryRoot + auxiliaryRoot;
    constantTerm = normalizedY - linearAdjustment;
    discriminant = quadraticCoefficient * quadraticCoefficient - 4.0 * constantTerm;
    if (discriminant >= 0.0) {
        discriminant = java::Math::sqrt(discriminant);
        results[rootCount++] = 0.5 * (quadraticCoefficient + discriminant);
        results[rootCount++] = 0.5 * (quadraticCoefficient - discriminant);
    }
    return rootCount;
}

// Root solver based on the Sturm sequences for a Polynomial
int
PolynomialSolver::solvePolynomial(
    int order, const double *coeffs, double *roots, double minValue)
{
    Polynomial sturmSequence[PolynomialConstants::MAX_ORDER + 1];
    double maximumValue;
    int coefficientIndex;
    int rootCount;
    int sequenceLength;
    int changesAtMinimum;
    int changesAtMaximum;

    // Put the coefficients into the top of the stack
    for (coefficientIndex = 0; coefficientIndex <= order; coefficientIndex++) {
        sturmSequence[0].coefficients[order - coefficientIndex] = coeffs[coefficientIndex];
    }

    // Build the Sturm sequence
    sequenceLength = PolynomialSolver::buildSturmSequence(order, &sturmSequence[0]);

    // Get the total number of visible roots
    if ((rootCount = PolynomialSolver::countVisibleRoots(
             sequenceLength, sturmSequence, &changesAtMinimum, &changesAtMaximum)) == 0) {
        return 0;
    }

    // Bracket the roots
    maximumValue = PolynomialConstants::POLYNOMIAL_MAX_DISTANCE;

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
        changesAtMaximum, roots);

    return rootCount;
}
