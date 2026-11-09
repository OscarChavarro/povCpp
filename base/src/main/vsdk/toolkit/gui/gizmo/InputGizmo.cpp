#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/gui/gizmo/InputGizmo.h"

const ColorRgb InputGizmo::HIGHLIGHT_COLOR(1, 1, 0);

namespace {
const int MAX_EDIT_LENGTH = 12;

bool isZeroText(const java::String& text)
{
    // Matches "0*\.?0*"
    int i = 0;
    int n = text.length();
    while ( i < n && text.charAt(i) == '0' ) {
        i++;
    }
    if ( i < n && text.charAt(i) == '.' ) {
        i++;
    }
    while ( i < n && text.charAt(i) == '0' ) {
        i++;
    }
    return i == n;
}

java::String repeatZeros(int n)
{
    java::String text;
    int i;
    for ( i = 0; i < n; i++ ) {
        text += "0";
    }
    return text;
}
}

InputGizmo::InputGizmo(int numberOfFields)
    : decimals(DECIMALS), integerDigits(1), limitTypedDecimals(false),
      valueChangeRules(InputGizmoValueChangeRules::forTranslation())
{
    init(numberOfFields);
}

InputGizmo::InputGizmo(int numberOfFields, int decimals, int integerDigits)
    : decimals(decimals > 0 ? decimals : 0),
      integerDigits(integerDigits > 1 ? integerDigits : 1),
      limitTypedDecimals(true),
      valueChangeRules(InputGizmoValueChangeRules::forTranslation())
{
    init(numberOfFields);
}

InputGizmo::InputGizmo(int numberOfFields, int decimals, int integerDigits,
                       const InputGizmoValueChangeRules& valueChangeRules)
    : decimals(decimals > 0 ? decimals : 0),
      integerDigits(integerDigits > 1 ? integerDigits : 1),
      limitTypedDecimals(true), valueChangeRules(valueChangeRules)
{
    init(numberOfFields);
}

void InputGizmo::init(int numberOfFields)
{
    int count = numberOfFields > 1 ? numberOfFields : 1;
    int i;

    for ( i = 0; i < count; i++ ) {
        values.add(0.0);
        colors.add(ColorRgb(0, 0, 0));
        highlighted.add(false);
        editTexts.add(java::String(""));
        editing.add(false);
    }
    selectedField = 0;
    commitPending = false;
}

InputGizmoValueChangeRules& InputGizmo::getValueChangeRules()
{
    return valueChangeRules;
}

void InputGizmo::setValueChangeRules(
    const InputGizmoValueChangeRules& valueChangeRules)
{
    this->valueChangeRules = valueChangeRules;
}

java::String InputGizmo::format(double value)
{
    return format(value, DECIMALS);
}

java::String InputGizmo::format(double value, int decimals)
{
    char buffer[128];
    std::snprintf(buffer, sizeof(buffer), "%.*f", decimals, value);
    java::String text(buffer);

    if ( text.startsWith("-") && isZeroText(text.substring(1)) ) {
        // A tiny negative value must not show as a "negative zero"
        return text.substring(1);
    }
    return text;
}

int InputGizmo::getDecimals() const
{
    return decimals;
}

java::String InputGizmo::getReferenceText() const
{
    java::String text = java::String("-") + repeatZeros(integerDigits);
    if ( decimals > 0 ) {
        text += ".";
        text += repeatZeros(decimals);
    }
    return text;
}

int InputGizmo::getNumberOfFields() const
{
    return (int)values.size();
}

double InputGizmo::getValue(int field) const
{
    return values[field];
}

void InputGizmo::setValue(int field, double value)
{
    values[field] = value;
}

const ColorRgb& InputGizmo::getFieldColor(int field) const
{
    return colors[field];
}

void InputGizmo::setFieldColor(int field, const ColorRgb& color)
{
    colors[field] = color;
}

bool InputGizmo::isFieldHighlighted(int field) const
{
    return highlighted[field];
}

void InputGizmo::setFieldHighlighted(int field, bool highlighted)
{
    this->highlighted[field] = highlighted;
}

ColorRgb InputGizmo::getFieldDisplayColor(int field) const
{
    return highlighted[field] ? HIGHLIGHT_COLOR : colors[field];
}

int InputGizmo::getSelectedField() const
{
    return selectedField;
}

void InputGizmo::setSelectedField(int field)
{
    if ( field >= 0 && field < values.size() ) {
        selectedField = field;
    }
}

void InputGizmo::selectNextField()
{
    selectedField = (selectedField + 1) % values.size();
}

void InputGizmo::selectPreviousField()
{
    selectedField = (selectedField + values.size() - 1) % values.size();
}

bool InputGizmo::isEditing(int field) const
{
    return editing[field];
}

bool InputGizmo::isEditing() const
{
    long i;
    for ( i = 0; i < editing.size(); i++ ) {
        if ( editing[i] ) {
            return true;
        }
    }
    return false;
}

java::String InputGizmo::getEditText(int field) const
{
    return editing[field] ? editTexts[field] : java::String("");
}

java::String InputGizmo::getDisplayText(int field) const
{
    if ( editing[field] ) {
        return editTexts[field];
    }
    return format(values[field], decimals);
}

void InputGizmo::cancelEditing()
{
    long i;
    for ( i = 0; i < editTexts.size(); i++ ) {
        editTexts[i] = "";
        editing[i] = false;
    }
    commitPending = false;
}

bool InputGizmo::consumesKey(const KeyEvent& event) const
{
    if ( (event.modifierMask & (KeyEvent::MASK_CTRL | KeyEvent::MASK_ALT)) != 0 ) {
        return false;
    }
    if ( digitOf(event) >= 0 || isDecimalPoint(event) || isMinus(event) ||
         stepOf(event) != 0.0 ) {
        return true;
    }
    switch ( event.keycode ) {
      case KeyEvent::KEY_TAB:
      case KeyEvent::KEY_BACKSPACE:
        return true;
      case KeyEvent::KEY_ENTER:
      case KeyEvent::KEY_NUMENTER:
      case KeyEvent::KEY_ESC:
        return isEditing();
      default:
        return false;
    }
}

bool InputGizmo::processKeyPressedEvent(const KeyEvent& event)
{
    if ( !consumesKey(event) ) {
        return false;
    }

    int digit = digitOf(event);

    if ( digit >= 0 ) {
        appendToSelected((char)('0' + digit));
    }
    else if ( isDecimalPoint(event) ) {
        appendDecimalPoint();
    }
    else if ( isMinus(event) ) {
        toggleSignOfSelected();
    }
    else if ( stepOf(event) != 0.0 ) {
        stepSelected(stepOf(event));
    }
    else {
        switch ( event.keycode ) {
          case KeyEvent::KEY_TAB:
            if ( (event.modifierMask & KeyEvent::MASK_SHIFT) != 0 ) {
                selectPreviousField();
            }
            else {
                selectNextField();
            }
            break;
          case KeyEvent::KEY_BACKSPACE:
            deleteLastCharacterOfSelected();
            break;
          case KeyEvent::KEY_ESC:
            cancelEditing();
            break;
          case KeyEvent::KEY_ENTER:
          case KeyEvent::KEY_NUMENTER:
            commitPending = true;
            break;
          default:
            break;
        }
    }
    return true;
}

bool InputGizmo::consumeCommit()
{
    bool commit = commitPending;

    commitPending = false;
    return commit;
}

java::ArrayList<double> InputGizmo::getValuesWithEdits() const
{
    java::ArrayList<double> result(values);
    long i;

    for ( i = 0; i < result.size(); i++ ) {
        double value;

        if ( editing[i] && parse(editTexts[i], &value) ) {
            result[i] = value;
        }
    }
    return result;
}

bool InputGizmo::parse(const java::String& text, double* outValue)
{
    if ( text.isEmpty() ) {
        return false;
    }
    const char* start = text.c_str();
    char* end = nullptr;
    double value = std::strtod(start, &end);

    if ( end == start ) {
        return false;
    }
    while ( *end == ' ' || *end == '\t' ) {
        end++;
    }
    if ( *end != '\0' || std::isnan(value) || std::isinf(value) ) {
        return false;
    }
    *outValue = value;
    return true;
}

int InputGizmo::digitOf(const KeyEvent& event)
{
    if ( event.unicodeId >= '0' && event.unicodeId <= '9' ) {
        return event.unicodeId - '0';
    }
    if ( event.keycode >= KeyEvent::KEY_0 && event.keycode <= KeyEvent::KEY_9 ) {
        return event.keycode - KeyEvent::KEY_0;
    }
    if ( event.keycode >= KeyEvent::KEY_NUM0 &&
         event.keycode <= KeyEvent::KEY_NUM9 ) {
        return event.keycode - KeyEvent::KEY_NUM0;
    }
    return -1;
}

bool InputGizmo::isDecimalPoint(const KeyEvent& event)
{
    // The comma is accepted too, as it is the decimal separator of many
    // keyboard layouts.
    return event.unicodeId == '.' || event.unicodeId == ',' ||
        event.keycode == KeyEvent::KEY_PERIOD ||
        event.keycode == KeyEvent::KEY_NUMPERIOD ||
        event.keycode == KeyEvent::KEY_COMMA;
}

bool InputGizmo::isMinus(const KeyEvent& event)
{
    return event.unicodeId == '-' || event.keycode == KeyEvent::KEY_MINUS ||
        event.keycode == KeyEvent::KEY_NUMMINUS;
}

double InputGizmo::stepOf(const KeyEvent& event) const
{
    switch ( event.keycode ) {
      case KeyEvent::KEY_RIGHT: return valueChangeRules.getLevelOneStep();
      case KeyEvent::KEY_LEFT: return -valueChangeRules.getLevelOneStep();
      case KeyEvent::KEY_UP: return valueChangeRules.getLevelTwoStep();
      case KeyEvent::KEY_DOWN: return -valueChangeRules.getLevelTwoStep();
      case KeyEvent::KEY_PAGEUP: return valueChangeRules.getLevelThreeStep();
      case KeyEvent::KEY_PAGEDOWN: return -valueChangeRules.getLevelThreeStep();
      default: return 0.0;
    }
}

void InputGizmo::stepSelected(double step)
{
    double typed;
    double current = values[selectedField];
    if ( editing[selectedField] && parse(editTexts[selectedField], &typed) ) {
        current = typed;
    }
    double next = valueChangeRules.applyStep(current, step);

    // Rounded, so repeated small steps do not accumulate binary noise
    // (plain notation, 9 decimals at most, no trailing zeros)
    char buffer[128];
    std::snprintf(buffer, sizeof(buffer), "%.9f", next);
    java::String stepped(buffer);
    if ( stepped.indexOf('.') >= 0 ) {
        int end = stepped.length();
        while ( end > 0 && stepped.charAt(end - 1) == '0' ) {
            end--;
        }
        if ( end > 0 && stepped.charAt(end - 1) == '.' ) {
            end--;
        }
        stepped = stepped.substring(0, end);
    }
    if ( stepped.equals("-0") ) {
        stepped = "0";
    }

    setSelectedEditText(stepped);
    commitPending = true;
}

java::String InputGizmo::selectedEditTextOrEmpty() const
{
    if ( editing[selectedField] ) {
        return editTexts[selectedField];
    }
    return java::String("");
}

void InputGizmo::setSelectedEditText(const java::String& text)
{
    editTexts[selectedField] = text;
    editing[selectedField] = true;
}

void InputGizmo::appendToSelected(char character)
{
    java::String text = selectedEditTextOrEmpty();

    if ( text.length() >= MAX_EDIT_LENGTH ) {
        return;
    }
    if ( limitTypedDecimals ) {
        int point = text.indexOf('.');

        if ( point >= 0 && text.length() - point - 1 >= decimals ) {
            return;
        }
    }
    char buffer[2] = {character, '\0'};
    setSelectedEditText(text + buffer);
}

void InputGizmo::appendDecimalPoint()
{
    java::String text = selectedEditTextOrEmpty();

    if ( text.indexOf('.') >= 0 || (limitTypedDecimals && decimals == 0) ) {
        return;
    }
    if ( text.isEmpty() || text.equals("-") ) {
        text += "0";
    }
    if ( text.length() >= MAX_EDIT_LENGTH ) {
        return;
    }
    setSelectedEditText(text + ".");
}

void InputGizmo::toggleSignOfSelected()
{
    // Over a box that is not being edited, it starts a negative number.
    java::String text = selectedEditTextOrEmpty();

    if ( text.startsWith("-") ) {
        setSelectedEditText(text.substring(1));
    }
    else if ( text.length() < MAX_EDIT_LENGTH ) {
        setSelectedEditText(java::String("-") + text);
    }
}

void InputGizmo::deleteLastCharacterOfSelected()
{
    // Over a box that is not being edited, the deletion starts from the
    // text the box shows.
    java::String text = getDisplayText(selectedField);

    if ( !text.isEmpty() ) {
        text = text.substring(0, text.length() - 1);
    }
    setSelectedEditText(text);
}
