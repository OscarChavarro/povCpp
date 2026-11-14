#ifndef __MATCHER__
#define __MATCHER__

#include <cstddef>
#include <regex.h>

#include "java/lang/String.h"
#include "java/util/ArrayList.h"
#include "java/util/concurrent/atomic/AtomicLong.h"

namespace java {
namespace util {
namespace regex {

/**
Compiled POSIX expression shared by a `Pattern` and its matchers, with the
POSIX group that corresponds to each Java capturing group. It counts its
references (patterns and matchers) and is deleted by the last one released.
*/
class CompiledExpression {
  private:
    java::AtomicLong references;
    ~CompiledExpression();

  public:
    regex_t expression;
    /// Index 0 is the whole match, then one entry per Java group
    java::ArrayList<int> posixGroupOfJavaGroup;

    /**
    @param javaRegex expression in Java syntax
    @throws java::IllegalArgumentException if it is not valid
    */
    explicit CompiledExpression(const char *javaRegex);
    CompiledExpression(const CompiledExpression &) = delete;
    CompiledExpression &operator=(const CompiledExpression &) = delete;

    void
    retain();

    /**
    Drops one reference; the last one deletes the expression.
    */
    void
    release();
};

/**
Emulation of `java.util.regex.Matcher`: finds the successive matches of a
`Pattern` over a text.
*/
class Matcher {
  private:
    CompiledExpression *expression;
    java::String input;
    java::ArrayList<regmatch_t> match;
    std::size_t searchFrom;
    bool matched;

  public:
    /**
    @param expression compiled expression; the matcher retains it
    */
    Matcher(CompiledExpression *expression, const java::String &input);
    Matcher(const Matcher &other);
    Matcher &operator=(const Matcher &other);
    ~Matcher();

    /**
    Finds the next match, after the previous one.
    @return true if one was found
    */
    bool
    find();

    /**
    @return true if the whole text matches
    */
    bool
    matches();

    /**
    @param group group number, 0 for the whole match
    @return the text of the group in the last match, empty if it did not
    take part in it (Java returns null)
    @throws java::IllegalStateException if there is no match (Java throws
    IllegalStateException)
    */
    java::String
    group(int group) const;

    /**
    @return the text of the last match
    */
    java::String
    group() const;

    /**
    @return the number of capturing groups of the pattern
    */
    int
    groupCount() const;

    /**
    @return the index of the start of the last match
    */
    int
    start() const;

    /**
    @return the index after the end of the last match
    */
    int
    end() const;
};

}
}
}

#endif
