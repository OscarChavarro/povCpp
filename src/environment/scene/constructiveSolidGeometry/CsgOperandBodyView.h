#ifndef __CSG_OPERAND_BODY_VIEW__
#define __CSG_OPERAND_BODY_VIEW__

#include "vsdk/toolkit/environment/scene/csg/CsgBodyOperand.h"

class PovrayBody;

/**
The `CsgTree` view of a POV CSG leaf: a VITRAL `CsgBodyOperand` whose child
expression is the CSG the leaf body holds, since a POV CSG is not a geometry.
*/
class CsgOperandBodyView : public CsgBodyOperand {
  private:
    const PovrayBody& leaf;

  public:
    explicit CsgOperandBodyView(const PovrayBody& leaf);
    const CsgExpression* getChildExpression() const override;
};

#endif
