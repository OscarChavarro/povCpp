#include "numericalAnalysis/polynomial/PolynomialTermCounts.h"

const int PolynomialTermCounts::termCountsByOrderTable[] = {1, 4, 10, 20, 35, 56, 84, 120};

const int *
PolynomialTermCounts::termCountsByOrder()
{
    return termCountsByOrderTable;
}
