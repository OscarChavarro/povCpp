#ifndef __INPUT_GIZMO_VALUE_CHANGE_RULES__
#define __INPUT_GIZMO_VALUE_CHANGE_RULES__

/**
Rules that define how the value of a box of an `InputGizmo` changes when it is
stepped with the keyboard (see `InputGizmo::processKeyPressedEvent`).

Different owners of a nested `InputGizmo` need different units and behavior:
i.e. `TranslateGizmo` steps world units, while `RotateGizmo` steps degrees,
that make sense to wrap around a full turn. This class collects that behavior
so each owner can pass its own rules to its `InputGizmo` (see `forTranslation`
and `forRotation`), instead of `InputGizmo` hard-coding a single one.

- Step levels: the increments of level 1, 2 and 3 (requested with RIGHT/LEFT,
  UP/DOWN and PAGEUP/PAGEDOWN respectively). i.e. 0.1, 1 and 5 world units for
  translation, or 1, 5 and 15 degrees for rotation.
- Grid restriction: when enabled (see `setGridRestrictionEnabled`), every
  stepped value is rounded to the nearest multiple of `getGridSize()`, i.e.
  stepping 0.3333 by 10 gives 10.3333, but with a grid size of 1 it gives 10
  (or 11, whichever is nearest). This flag is meant to be toggled at runtime
  (i.e. while a modifier key is held), so it is mutable.
- Circular range: when enabled, every stepped value is wrapped into
  `[getMinValue(), getMaxValue())` instead of growing without bound, i.e.
  stepping 355 degrees by 15 gives 10 (not 370) when the range is [0, 360).
*/
class InputGizmoValueChangeRules {
private:
    /// Increment requested by RIGHT / LEFT
    double levelOneStep;
    /// Increment requested by UP / DOWN
    double levelTwoStep;
    /// Increment requested by PAGEUP / PAGEDOWN
    double levelThreeStep;

    /// True if stepped values are rounded to the nearest multiple of
    /// `gridSize`; meant to be toggled at runtime
    bool gridRestrictionEnabled;
    /// Size of the grid the stepped values are rounded to, while the grid
    /// restriction is enabled; always positive
    double gridSize;

    /// True if stepped values are wrapped into [minValue, maxValue)
    bool circular;
    /// Lower bound (included) of the circular range
    double minValue;
    /// Upper bound (excluded) of the circular range
    double maxValue;

    double wrap(double value) const;

public:
    /**
    Creates rules with no grid restriction and no circular range: stepping
    only adds the step of the requested level.
    */
    InputGizmoValueChangeRules(double levelOneStep, double levelTwoStep,
                               double levelThreeStep);

    /**
    Creates rules with a circular range and no grid restriction (see
    `setGridRestrictionEnabled` to turn it on later).
    @param minValue lower bound (included) of the circular range
    @param maxValue upper bound (excluded) of the circular range, greater than
    `minValue`
    */
    InputGizmoValueChangeRules(double levelOneStep, double levelTwoStep,
                               double levelThreeStep, double minValue,
                               double maxValue);

    /**
    @return the rules `TranslateGizmo` uses: world units stepped by 0.1, 1 and
    5, with no grid restriction and no circular range
    */
    static InputGizmoValueChangeRules forTranslation();

    /**
    @return the rules `RotateGizmo` uses: degrees stepped by 1, 5 and 15,
    wrapped around a full turn, [0, 360)
    */
    static InputGizmoValueChangeRules forRotation();

    /**
    @return the rules `ScaleGizmo` uses: scale factors stepped by 0.01, 0.1
    and 1, with no grid restriction and no circular range
    */
    static InputGizmoValueChangeRules forScale();

    double getLevelOneStep() const;
    double getLevelTwoStep() const;
    double getLevelThreeStep() const;
    bool isGridRestrictionEnabled() const;
    void setGridRestrictionEnabled(bool enabled);
    double getGridSize() const;

    /**
    @param gridSize size of the grid stepped values are rounded to; not
    positive values are ignored
    */
    void setGridSize(double gridSize);
    bool isCircular() const;
    double getMinValue() const;
    double getMaxValue() const;

    /**
    Applies a step to a value: adds it, then rounds the result to the grid
    (if the grid restriction is enabled) and wraps it into the circular range
    (if this instance is circular).
    @param currentValue value before the step
    @param step signed increment to apply
    @return the value after the step, restricted as configured
    */
    double applyStep(double currentValue, double step) const;
};

#endif
