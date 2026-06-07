#include "common/PolynomialTermCounts.h"

const int PolynomialTermCounts::termCountsByOrder[] = {1, 4, 10, 20, 35, 56, 84, 120};

const int *
PolynomialTermCounts::table()
{
    return termCountsByOrder;
}
