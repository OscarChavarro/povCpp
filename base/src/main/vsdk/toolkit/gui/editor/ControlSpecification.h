#ifndef __CONTROL_SPECIFICATION__
#define __CONTROL_SPECIFICATION__

#include "java/lang/String.h"

/**
Parsed form of one control specification string, as stored in
`Entity::getControlSpecifications()`. The string format is
"type;name;valid interval", for example "double;radius;(0, INFINITE)".

The name identifies the pair of accessors the entity registered for the
attribute with `Entity::addControlSpecification` (in Java, the methods
`getRadius()` and `setRadius(value)` found by reflection). In the interval,
"[" and "]" denote inclusive limits, while "(" and ")" denote exclusive
limits. The words "INFINITE" and "-INFINITE" denote unbounded limits. The
interval part may be omitted, meaning that any value is valid.
*/
class ControlSpecification {
public:
    static const char *const INFINITE;

private:
    java::String type;
    java::String name;
    java::String intervalText;
    double lowerBound;
    double upperBound;
    bool lowerInclusive;
    bool upperInclusive;

    ControlSpecification(const java::String &type, const java::String &name,
        const java::String &intervalText, double lowerBound,
        double upperBound, bool lowerInclusive, bool upperInclusive);

    static void reportMalformed(const java::String &specification,
                                const char *reason);

public:
    /**
    Parses a control specification string.
    @param specification string in "type;name;valid interval" format
    @return the parsed specification (owned by the caller), or null if the
    string is malformed
    */
    static ControlSpecification *parse(const java::String &specification);

    /**
    @return the type name, for example "double"
    */
    const java::String &getType() const { return type; }

    /**
    @return the attribute name, for example "radius"
    */
    const java::String &getName() const { return name; }

    /**
    @return the attribute name with its first letter in upper case, as used
    in accessor names and GUI labels, for example "Radius"
    */
    java::String getLabel() const;

    /**
    @return the interval as written in the specification, or an empty string
    when the value is unbounded
    */
    const java::String &getIntervalText() const { return intervalText; }

    /**
    @param value value to test
    @return true if the value lies inside the valid interval
    */
    bool contains(double value) const;
};

#endif
