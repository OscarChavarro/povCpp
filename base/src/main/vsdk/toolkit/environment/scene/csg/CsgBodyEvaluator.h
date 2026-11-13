#ifndef VSDK_CSG_BODY_EVALUATOR_H
#define VSDK_CSG_BODY_EVALUATOR_H

// Ray evaluator of a CsgBody: the regularized [ROTH1982] classification
// (CsgRothRayClassifier) or the POV-Ray 1.0 crossing membership heuristic
// (CsgCrossingMembershipFilter), which keeps union internal surfaces.
enum class CsgBodyEvaluator { ROTH, CROSSING_MEMBERSHIP };
#endif
