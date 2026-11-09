#ifndef __INPUT_GIZMO__
#define __INPUT_GIZMO__

#include "java/lang/String.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/gui/KeyEvent.h"
#include "vsdk/toolkit/gui/gizmo/Gizmo.h"
#include "vsdk/toolkit/gui/gizmo/InputGizmoValueChangeRules.h"

/**
Gizmo made of a row of numeric input boxes, designed to show (and let the user
type) the numbers that describe the state of another gizmo, i.e. the
coordinates of a `TranslateGizmo`, which uses it nested.

Each box shows a value with `getDecimals()` digits (`DECIMALS` by default),
unless the user is editing it: then it shows the text typed. Exactly one box
is selected, and TAB (SHIFT+TAB) cycles the selection. Digits, the decimal
point and BACKSPACE edit the selected box as an input: typing over a box that
is not being edited replaces its value, and `-` starts a negative number, or
changes the sign of the number being edited. The arrow keys and PAGEUP /
PAGEDOWN step the value of the selected box, and accept it right away (see
below). ENTER accepts every box edited (see `consumeCommit`), ESC discards
the edition. The owner keeps the values updated (see `setValue`) and must
discard the edition (see `cancelEditing`) whenever it changes the state the
values describe by other means, so the boxes go back to show the real values.

Each box has a color (see `setFieldColor`), that becomes the highlight color
when the owner highlights it (see `setFieldHighlighted`).

This class only keeps state and processes vitral events: painting is up to
renderers, and using the numbers accepted is up to the owner.
*/
class InputGizmo : public Gizmo {
public:
    static const int DECIMALS = 3;
    static const ColorRgb HIGHLIGHT_COLOR;

private:
    int decimals;
    int integerDigits;
    bool limitTypedDecimals;
    java::ArrayList<double> values;
    java::ArrayList<ColorRgb> colors;
    java::ArrayList<bool> highlighted;
    java::ArrayList<java::String> editTexts;
    java::ArrayList<bool> editing;
    int selectedField;
    bool commitPending;
    InputGizmoValueChangeRules valueChangeRules;

    void init(int numberOfFields);
    static int digitOf(const KeyEvent& event);
    static bool isDecimalPoint(const KeyEvent& event);
    static bool isMinus(const KeyEvent& event);
    double stepOf(const KeyEvent& event) const;
    void stepSelected(double step);
    void appendToSelected(char character);
    void appendDecimalPoint();
    void toggleSignOfSelected();
    void deleteLastCharacterOfSelected();
    java::String selectedEditTextOrEmpty() const;
    void setSelectedEditText(const java::String& text);

public:
    /**
    Creates a gizmo that shows `DECIMALS` decimals, and lets the user type as
    many as fit in a box. Its boxes step by
    `InputGizmoValueChangeRules::forTranslation()`.
    @param numberOfFields number of input boxes, at least 1
    */
    explicit InputGizmo(int numberOfFields);

    /**
    Creates a gizmo whose boxes show, and accept typing, only the given number
    of decimals (i.e. 2 for angles in degrees). Its boxes step by
    `InputGizmoValueChangeRules::forTranslation()`.
    @param numberOfFields number of input boxes, at least 1
    @param decimals number of digits shown after the decimal point (not
    negative); the user can not type more
    @param integerDigits number of digits before the decimal point the boxes
    are wide enough for (at least 1); i.e. 3 for angles in degrees
    */
    InputGizmo(int numberOfFields, int decimals, int integerDigits);

    /**
    Creates a gizmo whose boxes show, and accept typing, only the given number
    of decimals, and step as described by the given rules (i.e.
    `InputGizmoValueChangeRules::forRotation()` for angles in degrees).
    */
    InputGizmo(int numberOfFields, int decimals, int integerDigits,
               const InputGizmoValueChangeRules& valueChangeRules);

    virtual ~InputGizmo() {}

    /**
    @return the rules that describe how the selected box steps with the
    keyboard (mutable, i.e. to toggle the grid restriction at runtime)
    */
    InputGizmoValueChangeRules& getValueChangeRules();
    void setValueChangeRules(const InputGizmoValueChangeRules& valueChangeRules);

    /**
    @return the text shown, with `DECIMALS` decimals, for a value that is not
    being edited
    */
    static java::String format(double value);

    /**
    @return the text shown, with given decimals, for a value that is not
    being edited
    */
    static java::String format(double value, int decimals);

    int getDecimals() const;

    /**
    @return the widest text a box is expected to show (i.e. `-0.000`), so
    presenters can give every box the same width
    */
    java::String getReferenceText() const;
    int getNumberOfFields() const;

    //= Values and colors =================================================
    double getValue(int field) const;
    void setValue(int field, double value);
    const ColorRgb& getFieldColor(int field) const;
    void setFieldColor(int field, const ColorRgb& color);
    bool isFieldHighlighted(int field) const;

    /**
    @param field index of a box
    @param highlighted true to draw the box with `HIGHLIGHT_COLOR`
    */
    void setFieldHighlighted(int field, bool highlighted);

    /**
    @return the color the box must be drawn with
    */
    ColorRgb getFieldDisplayColor(int field) const;

    //= Selection and edition state =======================================
    int getSelectedField() const;

    /**
    @param field index of the box to select; invalid indexes are ignored
    */
    void setSelectedField(int field);
    void selectNextField();
    void selectPreviousField();

    /**
    @return true if the user has typed something in the box, and it has not
    been accepted or discarded yet
    */
    bool isEditing(int field) const;

    /**
    @return true if any box is being edited
    */
    bool isEditing() const;

    /**
    @param field index of a box
    @return the text typed in the box, or an empty string if it is not being
    edited (see `isEditing(int)`)
    */
    java::String getEditText(int field) const;

    /**
    @return the text the box must show: the one being typed or its value
    */
    java::String getDisplayText(int field) const;

    /**
    Discards the text typed in every box (and any pending commit), so they
    show their values again.
    */
    void cancelEditing();

    //= Events ============================================================

    /**
    Tells if a key press belongs to the boxes, so the caller can keep it away
    from other commands that use the same keys (digits, `-`, TAB...). ENTER
    and ESC are only taken while some box is being edited.
    @return true if `processKeyPressedEvent` would use the event
    */
    bool consumesKey(const KeyEvent& event) const;

    /**
    Edits the boxes as requested by a key press.
    @return true if the event was used (see `consumesKey`)
    */
    bool processKeyPressedEvent(const KeyEvent& event);

    /**
    @return true (once) if the user accepted the edition with ENTER (or
    stepped a value with the arrow keys) since the last call; the owner must
    then use the numbers (see `getValuesWithEdits`) and discard the edition
    */
    bool consumeCommit();

    /**
    @return the values of the boxes, replacing the ones of the boxes edited by
    the numbers typed. Texts that are not a number (i.e. empty ones) are
    ignored.
    */
    java::ArrayList<double> getValuesWithEdits() const;

    /**
    @param text text typed in a box
    @param outValue the number it represents, when valid
    @return true if the text is a valid (finite) number
    */
    static bool parse(const java::String& text, double* outValue);
};

#endif
