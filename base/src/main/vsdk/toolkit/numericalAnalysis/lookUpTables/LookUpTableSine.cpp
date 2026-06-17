#include "java/lang/Math.h"
#include "vsdk/toolkit/numericalAnalysis/lookUpTables/LookUpTableSine.h"
LookUpTableSine::LookUpTableSine()
{
    for (int i = 0; i < SIZE; i++) {
        table[i] = java::Math::sin(i / (double)SIZE * (java::Math::PI * 2.0));
    }
}

LookUpTableSine::LookUpTableSine(int numberOfApproximationDecimals)
{
    const double scale = java::Math::pow(10.0, numberOfApproximationDecimals);
    const double pi = java::Math::round(java::Math::PI * scale) / scale;

    for (int i = 0; i < SIZE; i++) {
        table[i] = java::Math::sin(i / (double)SIZE * (pi * 2.0));
    }
}
