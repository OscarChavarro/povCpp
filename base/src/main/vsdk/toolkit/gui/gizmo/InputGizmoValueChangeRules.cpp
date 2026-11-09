#include <cmath>

#include "vsdk/toolkit/gui/gizmo/InputGizmoValueChangeRules.h"

namespace {
/// Default values of the grid restriction, used while it is disabled
const double DEFAULT_GRID_SIZE = 1.0;
}

InputGizmoValueChangeRules::InputGizmoValueChangeRules(
    double levelOneStep, double levelTwoStep, double levelThreeStep)
    : InputGizmoValueChangeRules(levelOneStep, levelTwoStep, levelThreeStep,
                                 0.0, 0.0)
{
}

InputGizmoValueChangeRules::InputGizmoValueChangeRules(
    double levelOneStep, double levelTwoStep, double levelThreeStep,
    double minValue, double maxValue)
    : levelOneStep(levelOneStep), levelTwoStep(levelTwoStep),
      levelThreeStep(levelThreeStep), gridRestrictionEnabled(false),
      gridSize(DEFAULT_GRID_SIZE), circular(maxValue > minValue),
      minValue(minValue), maxValue(maxValue)
{
}

InputGizmoValueChangeRules InputGizmoValueChangeRules::forTranslation()
{
    return InputGizmoValueChangeRules(0.1, 1.0, 5.0);
}

InputGizmoValueChangeRules InputGizmoValueChangeRules::forRotation()
{
    return InputGizmoValueChangeRules(1.0, 5.0, 15.0, 0.0, 360.0);
}

InputGizmoValueChangeRules InputGizmoValueChangeRules::forScale()
{
    return InputGizmoValueChangeRules(0.01, 0.1, 1.0);
}

double InputGizmoValueChangeRules::getLevelOneStep() const
{
    return levelOneStep;
}

double InputGizmoValueChangeRules::getLevelTwoStep() const
{
    return levelTwoStep;
}

double InputGizmoValueChangeRules::getLevelThreeStep() const
{
    return levelThreeStep;
}

bool InputGizmoValueChangeRules::isGridRestrictionEnabled() const
{
    return gridRestrictionEnabled;
}

void InputGizmoValueChangeRules::setGridRestrictionEnabled(bool enabled)
{
    gridRestrictionEnabled = enabled;
}

double InputGizmoValueChangeRules::getGridSize() const
{
    return gridSize;
}

void InputGizmoValueChangeRules::setGridSize(double gridSize)
{
    if ( gridSize > 0.0 ) {
        this->gridSize = gridSize;
    }
}

bool InputGizmoValueChangeRules::isCircular() const
{
    return circular;
}

double InputGizmoValueChangeRules::getMinValue() const
{
    return minValue;
}

double InputGizmoValueChangeRules::getMaxValue() const
{
    return maxValue;
}

double InputGizmoValueChangeRules::applyStep(double currentValue,
                                             double step) const
{
    double result = currentValue + step;

    if ( gridRestrictionEnabled ) {
        // std::floor(v + 0.5) mimics Java's Math.round
        result = std::floor(result / gridSize + 0.5) * gridSize;
    }
    if ( circular ) {
        result = wrap(result);
    }
    return result;
}

double InputGizmoValueChangeRules::wrap(double value) const
{
    double range = maxValue - minValue;
    double offset = std::fmod(value - minValue, range);

    if ( offset < 0.0 ) {
        offset += range;
    }
    return minValue + offset;
}
