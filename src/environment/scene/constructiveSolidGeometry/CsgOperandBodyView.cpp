#include "environment/scene/constructiveSolidGeometry/CsgOperandBodyView.h"
#include "environment/scene/constructiveSolidGeometry/ConstructiveSolidGeometry.h"
#include "environment/scene/PovrayBody.h"

CsgOperandBodyView::CsgOperandBodyView(const PovrayBody& leaf) :
    CsgBodyOperand(leaf),
    leaf(leaf)
{
}

const CsgExpression*
CsgOperandBodyView::getChildExpression() const
{
    return leaf.getCsg();
}
