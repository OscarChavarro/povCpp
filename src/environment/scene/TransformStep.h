#ifndef __TRANSFORM_STEP__
#define __TRANSFORM_STEP__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

// Passive record of one elementary translate/rotate/scale/invert keyword as
// encountered at parse time, in the exact order the .pov file applies them.
// This is parsed-scene data (what the file said), not render policy or
// geometry math - see doc/performanceReviewPlan5.md Phase 1.
struct TransformStep {
    enum class Kind { Translate, Rotate, Scale, Invert };

    Kind kind;
    Vector3Dd vector;  // unused for Invert

    TransformStep() : kind(Kind::Invert), vector(0.0, 0.0, 0.0) {}
    TransformStep(Kind kind, const Vector3Dd &vector) : kind(kind), vector(vector) {}

    bool operator==(const TransformStep &other) const
    {
        return kind == other.kind &&
            vector.x() == other.vector.x() &&
            vector.y() == other.vector.y() &&
            vector.z() == other.vector.z();
    }
};

#endif
