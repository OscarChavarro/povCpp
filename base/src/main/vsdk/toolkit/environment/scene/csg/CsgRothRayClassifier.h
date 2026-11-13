#ifndef VSDK_CSG_ROTH_RAY_CLASSIFIER_H
#define VSDK_CSG_ROTH_RAY_CLASSIFIER_H
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidQueryStatus.h"
class CsgExpression;
class CsgRayClassification;
class CsgRayWorkspace;
class Ray;
class RayCrossingQuery;
// [ROTH1982] section 3.3 ray classification: every leaf is classified as IN/OUT
// segments and the segments are combined through the expression with the
// union, intersection and difference rows of Table 3. Events with exactly
// equal parameters are applied together before the combined state is read,
// so coincident faces and touching operands yield the regularized result and
// point contacts produce no boundary. ON neighbourhoods (section 6.1) are not
// modelled. Uncertain or unsupported leaves fail the whole query.
class CsgRothRayClassifier {
private:
    static bool evaluate(const CsgExpression& expression, const java::ArrayList<bool>& states,
        long int& leafIndex);
    static RaySolidQueryStatus combine(const CsgExpression& expression, CsgRayWorkspace& workspace,
        CsgRayClassification& result);
public:
    static RaySolidQueryStatus classify(const CsgExpression& expression, const Ray& ray,
        const RayCrossingQuery& query, CsgRayClassification& result);
    // Same query with caller-owned scratch storage; with a reused workspace
    // and result it allocates only inside leaf classification.
    static RaySolidQueryStatus classify(const CsgExpression& expression, const Ray& ray,
        const RayCrossingQuery& query, CsgRayWorkspace& workspace, CsgRayClassification& result);
    // Membership of the expression for given per-leaf states, in the leaf order
    // of CsgRayLeafQuery. The expression must be the one that produced them.
    static bool isInside(const CsgExpression& expression, const java::ArrayList<bool>& states);
};
#endif
