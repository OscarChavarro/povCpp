#ifndef __STREAM_TOKENIZER__
#define __STREAM_TOKENIZER__

#include "java/io/Reader.h"
#include "java/lang/String.h"
namespace java {

/**
Port of the subset of java.io.StreamTokenizer (table-driven, as in OpenJDK)
used by Vitral: resetSyntax, eolIsSignificant, slashSlashComments,
slashStarComments, commentChar, whitespaceChars, wordChars, parseNumbers,
nextToken, ttype, nval, sval. The slash comment flags are stored but the
"/" comment handling is not implemented (Vitral only disables them).

Omitted: quoteChar/ordinaryChar(s)/lowerCaseMode/pushBack/lineno/toString,
and the quoted string tokens. The reader is not owned.
*/
class StreamTokenizer {
  private:
    Reader &reader;
    int peekc;
    bool eolIsSignificantP;
    bool slashSlashCommentsP;
    bool slashStarCommentsP;
    unsigned char ctype[256];

    enum {
        CT_WHITESPACE = 1, CT_DIGIT = 2, CT_ALPHA = 4, CT_COMMENT = 16
    };

    int
    typeOf(int c) const;

  public:
    static const int TT_EOF = -1;
    static const int TT_EOL = '\n';
    static const int TT_NUMBER = -2;
    static const int TT_WORD = -3;

    int ttype;
    double nval;
    java::String sval;

    explicit StreamTokenizer(Reader &reader);

    void resetSyntax();
    void eolIsSignificant(bool flag);
    void slashSlashComments(bool flag);
    void slashStarComments(bool flag);
    void commentChar(int ch);
    void whitespaceChars(int low, int hi);
    void wordChars(int low, int hi);
    void parseNumbers();
    int nextToken();
};

}

#endif
