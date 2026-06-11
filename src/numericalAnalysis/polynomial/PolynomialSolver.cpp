/**
This file was written by Alexander Enzmann.  He wrote the code for
4th-6th order shapes and generously provided us these enhancements.
*/

#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
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
PolynomialSolver::PolynomialSolver::absInline(double x)
{
    return (x < 0.0) ? (0.0 - x) : x;
}

inline double
PolynomialSolver::PolynomialSolver::maxInline(double x, double y)
{
    return (x < y) ? y : x;
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
PolynomialSolver::modp(Polynomial *u, Polynomial *v, Polynomial *r)
{
    int i;
    int k;
    int j;
    for (i = 0; i < u->ord; i++) {
        r[i] = u[i];
    }

    if (v->coef[v->ord] < 0.0) {
        for (k = u->ord - v->ord - 1; k >= 0; k -= 2) {
            r->coef[k] = -r->coef[k];
        }
        for (k = u->ord - v->ord; k >= 0; k--) {
            for (j = v->ord + k - 1; j >= k; j--) {
                r->coef[j] = -r->coef[j] - r->coef[v->ord + k] * v->coef[j - k];
            }
        }
    } else {
        for (k = u->ord - v->ord; k >= 0; k--) {
            for (j = v->ord + k - 1; j >= k; j--) {
                r->coef[j] -= r->coef[v->ord + k] * v->coef[j - k];
            }
        }
    }

    k = v->ord - 1;
    while (k >= 0 && java::Math::abs(r->coef[k]) < COEFF_LIMIT) {
        r->coef[k] = 0.0;
        k--;
    }
    r->ord = (k < 0) ? 0 : k;
    return (r->ord);
}

// Build the sturmian sequence for a Polynomial
int
PolynomialSolver::buildsturm(int ord, Polynomial *sseq)
{
    int i;
    double f;
    double *fp;
    double *fc;
    Polynomial *sp;

    sseq[0].ord = ord;
    sseq[1].ord = ord - 1;

    // Calculate the derivative and normalize the leading coefficient
    f = java::Math::abs(sseq[0].coef[ord] * ord);
    fp = sseq[1].coef;
    fc = sseq[0].coef + 1;
    for (i = 1; i <= ord; i++) {
        *fp++ = *fc++ * i / f;
    }

    // Construct the rest of the Sturm sequence
    for (sp = sseq + 2; PolynomialSolver::modp(sp - 2, sp - 1, sp); sp++) {
        // Reverse the sign and normalize
        f = -java::Math::abs(sp->coef[sp->ord]);
        for (fp = &sp->coef[sp->ord]; fp >= sp->coef; fp--) {
            *fp /= f;
        }
    }
    sp->coef[0] = -sp->coef[0]; // Reverse the sign
    return (sp - sseq);
}

// Find out how many visible intersections there are
int
PolynomialSolver::visibleRoots(int np, Polynomial *sseq, int *atzer, int *atpos)
{
    int atposinf;
    int atzero;
    Polynomial *s;
    double f;
    double lf;

    atposinf = atzero = 0;
    // Changes at positve infinity
    lf = sseq[0].coef[sseq[0].ord];
    for (s = sseq + 1; s <= sseq + np; s++) {
        f = s->coef[s->ord];
        if (lf == 0.0 || lf * f < 0) {
            atposinf++;
        }
        lf = f;
    }

    // Changes at zero
    lf = sseq[0].coef[0];
    for (s = sseq + 1; s <= sseq + np; s++) {
        f = s->coef[0];
        if (lf == 0.0 || lf * f < 0) {
            atzero++;
        }
        lf = f;
    }

    *atzer = atzero;
    *atpos = atposinf;
    return (atzero - atposinf);
}

/**
Return the number of sign changes in the Sturm sequence in
sseq at the value a.
*/
int
PolynomialSolver::numchanges(int np, Polynomial *sseq, double a)
{
    int changes;
    double f;
    double lf;
    Polynomial *s;
    changes = 0;
    lf = PolynomialSolver::polyeval(a, sseq[0].ord, sseq[0].coef);
    for (s = sseq + 1; s <= sseq + np; s++) {
        f = PolynomialSolver::polyeval(a, s->ord, s->coef);
        if (lf == 0.0 || lf * f < 0) {
            changes++;
        }
        lf = f;
    }
    return (changes);
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
PolynomialSolver::sbisect(int np, Polynomial *sseq, double minValue,
    double maxValue, int atmin, int atmax, double *roots)
{
    double mid;
    int n1;
    int n2;
    int its;
    int atmid;
    int nroot;

    if ((nroot = atmin - atmax) == 1) {
        // First try using regula-falsa to find the root
        if (PolynomialSolver::regulaFalsa(
                sseq->ord, sseq->coef, minValue, maxValue, roots)) {
            return;
        } // That failed, so now find it by bisection
        for (its = 0; its < MAX_ITERATIONS; its++) {
            mid = (minValue + maxValue) / 2;
            atmid = PolynomialSolver::numchanges(np, sseq, mid);
            if (java::Math::abs(mid) > EPSILON) {
                if (java::Math::abs((maxValue - minValue) / mid) < EPSILON) {
                    roots[0] = mid;
                    return;
                }
            } else if (java::Math::abs(maxValue - minValue) < EPSILON) {
                roots[0] = mid;
                return;
            } else if ((atmin - atmid) == 0) {
                minValue = mid;
            } else {
                maxValue = mid;
            }
        }
        // Bisection took too long - just return what we got
        roots[0] = mid;
        return;
    }

    // There is more than one root in the interval.
    // Bisect to find new intervals
    for (its = 0; its < MAX_ITERATIONS; its++) {
        mid = (minValue + maxValue) / 2;
        atmid = PolynomialSolver::numchanges(np, sseq, mid);
        n1 = atmin - atmid;
        n2 = atmid - atmax;
        if (n1 != 0 && n2 != 0) {
            PolynomialSolver::sbisect(
                np, sseq, minValue, mid, atmin, atmid, roots);
            PolynomialSolver::sbisect(
                np, sseq, mid, maxValue, atmid, atmax, &roots[n1]);
            return;
        }
        if (n1 == 0) {
            minValue = mid;
        } else {
            maxValue = mid;
        }
    }

    // Took too long to bisect. Just return what we got
    for (n1 = atmax; n1 < atmin; n1++) {
        roots[n1 - atmax] = mid;
    }
}

double
PolynomialSolver::polyeval(double x, int n, double *coeffs)
{
    int i;
    double val;
    val = coeffs[n];
    for (i = n - 1; i >= 0; i--) {
        val = val * x + coeffs[i];
    }
    return val;
}

// Close in on a root by using regula-falsa
int
PolynomialSolver::regulaFalsa(
    int order, double *coef, double a, double b, double *val)
{
    int its;
    double fa;
    double fb;
    double x;
    double fx;
    double lfx;

    fa = PolynomialSolver::polyeval(a, order, coef);
    fb = PolynomialSolver::polyeval(b, order, coef);

    if (fa * fb > 0.0) {
        return 0;
    }

    if (java::Math::abs(fa) < COEFF_LIMIT) {
        *val = a;
        return 1;
    }

    if (java::Math::abs(fb) < COEFF_LIMIT) {
        *val = b;
        return 1;
    }

    lfx = fa;
    for (its = 0; its < MAX_ITERATIONS; its++) {
        x = (fb * a - fa * b) / (fb - fa);
        fx = PolynomialSolver::polyeval(x, order, coef);

        if (java::Math::abs(x) > EPSILON) {
            if (java::Math::abs(fx / x) < EPSILON) {
                *val = x;
                return 1;
            }
        } else if (java::Math::abs(fx) < EPSILON) {
            *val = x;
            return 1;
        }

        if (fa < 0) {
            if (fx < 0) {
                a = x;
                fa = fx;
                if ((lfx * fx) > 0) {
                    fb /= 2;
                }
            } else {
                b = x;
                fb = fx;
                if ((lfx * fx) > 0) {
                    fa /= 2;
                }
            }
        } else if (fx < 0) {
            b = x;
            fb = fx;
            if ((lfx * fx) > 0) {
                fa /= 2;
            }
        } else {
            a = x;
            fa = fx;
            if ((lfx * fx) > 0) {
                fb /= 2;
            }
        }
        if (java::Math::abs(b - a) < EPSILON) {
            // Check for underflow in the domain
            *val = x;
            return 1;
        }
        lfx = fx;
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
PolynomialSolver::solveQuadratic(double *x, double *y)
{
    double d;
    double t;
    double a;
    double b;
    double c;
    a = x[0];
    b = -x[1];
    c = x[2];
    if (a == 0.0) {
        if (b == 0.0) {
            return 0;
        }
        y[0] = c / b;
        return 1;
    }
    d = b * b - 4.0 * a * c;
    if (d < 0.0) {
        return 0;
    }
    if (java::Math::abs(d) < COEFF_LIMIT) {
        y[0] = 0.5 * b / a;
        return 1;
    }
    d = java::Math::sqrt(d);
    t = 2.0 * a;
    y[0] = (b + d) / t;
    y[1] = (b - d) / t;
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
PolynomialSolver::solveCubic(double *x, double *y)
{
    double q;
    double r;
    double q3;
    double r2;
    double sQ;
    double d;
    double an;
    double theta;
    double a0;
    double a1;
    double a2;
    double a3;
    double a1Squared;
    a0 = x[0];
    if (a0 == 0.0) {
        return PolynomialSolver::solveQuadratic(&x[1], y);
    }
    if (a0 != 1.0) {
        a1 = x[1] / a0;
        a2 = x[2] / a0;
        a3 = x[3] / a0;
    } else {
        a1 = x[1];
        a2 = x[2];
        a3 = x[3];
    }
    a1Squared = a1 * a1;
    q = (a1Squared - 3.0 * a2) / 9.0;
    r = (2.0 * a1Squared * a1 - 9.0 * a1 * a2 + 27.0 * a3) / 54.0;
    q3 = q * q * q;
    r2 = r * r;
    d = q3 - r2;
    an = a1 / 3.0;
    if (d >= 0.0) {
        // Three real roots
        d = r / java::Math::sqrt(q3);
        theta = java::Math::acos(d) / 3.0;
        sQ = -2.0 * java::Math::sqrt(q);
        y[0] = sQ * java::Math::cos(theta) - an;
        y[1] = sQ * java::Math::cos(theta + TWO_PI_3) - an;
        y[2] = sQ * java::Math::cos(theta + TWO_PI_43) - an;
        return 3;
    }
    sQ = java::Math::pow(java::Math::sqrt(r2 - q3) + PolynomialSolver::absInline(r), 1.0 / 3.0);
    if (r < 0) {
        y[0] = (sQ + q / sQ) - an;
    } else {
        y[0] = -(sQ + q / sQ) - an;
    }
    return 1;
}

/**
Test to see if any coeffs are more than 6 orders of magnitude
larger than the smallest
*/
int
PolynomialSolver::difficultCoeffs(int n, double *x)
{
    int i;
    double biggest;

    biggest = 0.0;
    for (i = 0; i <= n; i++) {
        if (java::Math::abs(x[i]) > biggest) {
            biggest = x[i];
        }
    }

    // Everything is zero no sense in doing any more
    if (biggest == 0.0) {
        return 0;
    }

    for (i = 0; i <= n; i++) {
        if (x[i] != 0.0) {
            if (java::Math::abs(biggest / x[i]) > FUDGE_FACTOR1) {
                return 1;
            }
        }
    }

    return 0;
}

int
PolynomialSolver::solveQuartic(double *x, double *results, double minValue)
{
    double cubic[4];
    double roots[3];
    double a0;
    double a1;
    double y;
    double d1;
    double x1;
    double t1;
    double t2;
    double c0;
    double c1;
    double c2;
    double c3;
    double c4;
    double d2;
    double q1;
    double q2;
    int i;

    // Figure out the size difference between coefficients
    if (PolynomialSolver::difficultCoeffs(4, x)) {
        if (java::Math::abs(x[0]) < COEFF_LIMIT) {
            if (java::Math::abs(x[1]) < COEFF_LIMIT) {
                return PolynomialSolver::solveQuadratic(&x[2], results);
            }
            return PolynomialSolver::solveCubic(&x[1], results);
        }
        return PolynomialSolver::polysolve(4, x, results, minValue);
    }

    c0 = x[0];
    if (java::Math::abs(c0) < COEFF_LIMIT) {
        return PolynomialSolver::solveCubic(&x[1], results);
    }
    if (java::Math::abs(x[4]) < COEFF_LIMIT) {
        return PolynomialSolver::solveCubic(x, results);
    }
    if (c0 != 1.0) {
        c1 = x[1] / c0;
        c2 = x[2] / c0;
        c3 = x[3] / c0;
        c4 = x[4] / c0;
    } else {
        c1 = x[1];
        c2 = x[2];
        c3 = x[3];
        c4 = x[4];
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

    a0 = 4.0 * c4;
    cubic[0] = 1.0;
    cubic[1] = -1.0 * c2;
    cubic[2] = c1 * c3 - a0;
    cubic[3] = a0 * c2 - c1 * c1 * c4 - c3 * c3;
    i = PolynomialSolver::solveCubic(&cubic[0], &roots[0]);
    if (i > 0) {
        y = roots[0];
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
    i = 0;
    a0 = c1 / 2.0;
    a1 = y / 2.0;

    t1 = a0 * a0 - c2 + y;
    if (t1 < 0.0) {
        if (t1 > FUDGE_FACTOR2) {
            t1 = 0.0;
        } else {
            // First Special case, a' < 0 means all roots are complex
            return 0;
        }
    }
    if (t1 < FUDGE_FACTOR3) {
        /**
        Second special case, the "x" term on the right hand side above
            has vanished.  In this case:
                     (x^2 + b*x/2 + y/2) = +java::Math::sqrt(y^2/4 - e), and
                     (x^2 + b*x/2 + y/2) = -java::Math::sqrt(y^2/4 - e).
        */
        t2 = a1 * a1 - c4;
        if (t2 < 0.0) {
            return 0;
        }
        x1 = 0.0;
        d1 = java::Math::sqrt(t2);
    } else {
        x1 = java::Math::sqrt(t1);
        d1 = 0.5 * (a0 * y - c3) / x1;
    }
    // Solve the first quadratic
    q1 = -a0 - x1;
    q2 = a1 + d1;
    d2 = q1 * q1 - 4.0 * q2;
    if (d2 >= 0.0) {
        d2 = java::Math::sqrt(d2);
        results[0] = 0.5 * (q1 + d2);
        results[1] = 0.5 * (q1 - d2);
        i = 2;
    }
    // Solve the second quadratic
    q1 = q1 + x1 + x1;
    q2 = a1 - d1;
    d2 = q1 * q1 - 4.0 * q2;
    if (d2 >= 0.0) {
        d2 = java::Math::sqrt(d2);
        results[i++] = 0.5 * (q1 + d2);
        results[i++] = 0.5 * (q1 - d2);
    }
    return i;
}

// Root solver based on the Sturm sequences for a Polynomial
int
PolynomialSolver::polysolve(
    int order, double *coeffs, double *roots, double minValue)
{
    Polynomial sseq[PolynomialConstants::MAX_ORDER + 1];
    double maxValue;
    int i;
    int nroots;
    int np;
    int atmin;
    int atmax;

    // Put the coefficients into the top of the stack
    for (i = 0; i <= order; i++) {
        sseq[0].coef[order - i] = coeffs[i];
    }

    // Build the Sturm sequence
    np = PolynomialSolver::buildsturm(order, &sseq[0]);

    // Get the total number of visible roots
    if ((nroots = PolynomialSolver::visibleRoots(np, sseq, &atmin, &atmax)) ==
        0) {
        return 0;
    }

    // Bracket the roots
    maxValue = PolynomialConstants::POLYNOMIAL_MAX_DISTANCE;

    atmin = PolynomialSolver::numchanges(np, sseq, minValue);
    atmax = PolynomialSolver::numchanges(np, sseq, maxValue);
    nroots = atmin - atmax;
    if (nroots == 0) {
        return 0;
    }

    // Perform the bisection
    PolynomialSolver::sbisect(
        np, sseq, minValue, maxValue, atmin, atmax, roots);

    return nroots;
}
