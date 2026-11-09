#ifndef __PATTERN__
#define __PATTERN__

#include "java/lang/String.h"
#include "java/util/regex/Matcher.h"
#include "vsdk/toolkit/common/memoryManagement/SharedPointer.h"

namespace java {
namespace util {
namespace regex {

/**
Emulation of `java.util.regex.Pattern` over POSIX extended regular
expressions. The Java expression is translated to the POSIX syntax:
character classes, groups, non capturing groups `(?:...)`, alternatives,
quantifiers, `\\s`, `\\S`, `\\d`, `\\D`, `\\w`, `\\W` and escapes are
supported. Java only features (lazy and possessive quantifiers,
lookaround, backreferences, `\\Q...\\E` inside patterns) are not: `quote`
escapes each special character instead. As POSIX matching finds the
longest match at the leftmost position, alternatives that are prefixes of
other alternatives may select a longer text than Java.
*/
class Pattern {
  private:
    java::String patternText;
    SharedPointer<CompiledExpression> expression;

    explicit Pattern(const java::String &regex);

  public:
    /**
    @param regex regular expression
    @throws java::IllegalArgumentException if the expression is not valid
    (Java throws PatternSyntaxException)
    */
    static Pattern
    compile(const java::String &regex);

    /**
    @return an expression matching the given text literally
    */
    static java::String
    quote(const java::String &text);

    /**
    @param input text to search; the matcher keeps its own copy
    */
    Matcher
    matcher(const java::String &input) const;

    const java::String &
    pattern() const;
};

}
}
}

#endif
