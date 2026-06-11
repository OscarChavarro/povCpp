#ifndef __POLYNOMIAL_TERM_COUNTS_H__
#define __POLYNOMIAL_TERM_COUNTS_H__

/**
Number of monomial terms (coefficients) of a 3-variable polynomial,
indexed by its order. Pure combinatorial data shared by the parser
(to know how many coefficients to read/allocate) and the polynomial
shape implementation (to know how many coefficients to evaluate).
*/
class PolynomialTermCounts {
  public:
    static const int termCountsByOrder[];

    static const int *table();
};

#endif
