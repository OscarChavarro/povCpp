#ifndef VSDK_CSG_SOLID_BOUNDS_H
#define VSDK_CSG_SOLID_BOUNDS_H
#include "java/util/ArrayList.h"
class CsgExpression;
class GeometryBounds;
// Conservative bounds of a shared expression's solid in its own coordinates.
// Union encloses every operand, intersection lies within each known operand,
// and difference lies within its minuend. Nested expressions are placed with
// outward rounding; unknown operands and cycles stay explicit.
class CsgSolidBounds {
private:
    static void queryExpression(const CsgExpression& expression,
        java::ArrayList<const CsgExpression*>& path, GeometryBounds& result);
public:
    static void query(const CsgExpression& expression, GeometryBounds& result);
};
#endif
