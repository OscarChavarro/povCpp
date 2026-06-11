#include <cmath>
#include "java/lang/Math.h"
#include "numericalAnalysis/lookUpTables/LookUpTableSine.h"

LookUpTableSine::LookUpTableSine()
{
    for (int i = 0; i < SIZE; i++) {
        table[i] = sin(i / (double)SIZE * (java::Math::PI * 2.0));
    }
}

LookUpTableSine::LookUpTableSine(int numberOfApproximationDecimals)
{
    double scale = java::Math::pow(10.0, numberOfApproximationDecimals);
    double pi = java::Math::round(java::Math::PI * scale) / scale;

    for (int i = 0; i < SIZE; i++) {
        table[i] = sin(i / (double)SIZE * (pi * 2.0));
    }
}
