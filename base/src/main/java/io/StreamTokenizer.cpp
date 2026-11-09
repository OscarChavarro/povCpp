#include "java/io/StreamTokenizer.h"
namespace java {

static const int NEED_CHAR = 0x7FFFFFFF;
static const int SKIP_LF = 0x7FFFFFFE;

StreamTokenizer::StreamTokenizer(Reader &reader):
    reader(reader),
    peekc(NEED_CHAR),
    eolIsSignificantP(false),
    slashSlashCommentsP(false),
    slashStarCommentsP(false),
    ttype(-4), // JDK TT_NOTHING
    nval(0.0)
{
    resetSyntax();
    wordChars('a', 'z');
    wordChars('A', 'Z');
    wordChars(128 + 32, 255);
    whitespaceChars(0, ' ');
    commentChar('/');
    parseNumbers();
}

int
StreamTokenizer::typeOf(int c) const {
    return c < 256 ? ctype[c] : CT_ALPHA;
}

void
StreamTokenizer::resetSyntax() {
    for ( int i = 0; i < 256; i++ ) {
        ctype[i] = 0;
    }
}

void
StreamTokenizer::eolIsSignificant(bool flag) {
    eolIsSignificantP = flag;
}

void
StreamTokenizer::slashSlashComments(bool flag) {
    slashSlashCommentsP = flag;
}

void
StreamTokenizer::slashStarComments(bool flag) {
    slashStarCommentsP = flag;
}

void
StreamTokenizer::commentChar(int ch) {
    if ( ch >= 0 && ch < 256 ) {
        ctype[ch] = CT_COMMENT;
    }
}

void
StreamTokenizer::whitespaceChars(int low, int hi) {
    if ( low < 0 ) low = 0;
    if ( hi >= 256 ) hi = 255;
    while ( low <= hi ) {
        ctype[low++] = CT_WHITESPACE;
    }
}

void
StreamTokenizer::wordChars(int low, int hi) {
    if ( low < 0 ) low = 0;
    if ( hi >= 256 ) hi = 255;
    while ( low <= hi ) {
        ctype[low++] |= CT_ALPHA;
    }
}

void
StreamTokenizer::parseNumbers() {
    for ( int i = '0'; i <= '9'; i++ ) {
        ctype[i] |= CT_DIGIT;
    }
    ctype[(int)'.'] |= CT_DIGIT;
    ctype[(int)'-'] |= CT_DIGIT;
}

int
StreamTokenizer::nextToken() {
    sval = "";
    int c = peekc;
    if ( c == SKIP_LF ) {
        c = reader.read();
        if ( c < 0 ) {
            return ttype = TT_EOF;
        }
        if ( c == '\n' ) {
            c = NEED_CHAR;
        }
    }
    if ( c == NEED_CHAR ) {
        c = reader.read();
        if ( c < 0 ) {
            return ttype = TT_EOF;
        }
    }
    ttype = c;
    peekc = NEED_CHAR;

    for ( ;; ) {
        int ct = typeOf(c);
        while ( (ct & CT_WHITESPACE) != 0 ) {
            if ( c == '\r' ) {
                if ( eolIsSignificantP ) {
                    peekc = SKIP_LF;
                    return ttype = TT_EOL;
                }
                c = reader.read();
                if ( c == '\n' ) {
                    c = reader.read();
                }
            }
            else {
                if ( c == '\n' && eolIsSignificantP ) {
                    return ttype = TT_EOL;
                }
                c = reader.read();
            }
            if ( c < 0 ) {
                return ttype = TT_EOF;
            }
            ct = typeOf(c);
        }

        if ( (ct & CT_DIGIT) != 0 ) {
            bool neg = false;
            if ( c == '-' ) {
                c = reader.read();
                if ( c != '.' && (c < '0' || c > '9') ) {
                    peekc = c;
                    return ttype = '-';
                }
                neg = true;
            }
            double v = 0;
            int decexp = 0;
            int seendot = 0;
            for ( ;; ) {
                if ( c == '.' && seendot == 0 ) {
                    seendot = 1;
                }
                else if ( '0' <= c && c <= '9' ) {
                    v = v * 10 + (c - '0');
                    decexp += seendot;
                }
                else {
                    break;
                }
                c = reader.read();
            }
            peekc = c;
            if ( decexp != 0 ) {
                double denom = 10;
                decexp--;
                while ( decexp > 0 ) {
                    denom *= 10;
                    decexp--;
                }
                v = v / denom;
            }
            nval = neg ? -v : v;
            return ttype = TT_NUMBER;
        }

        if ( (ct & CT_ALPHA) != 0 ) {
            int capacity = 32;
            int length = 0;
            char *buf = new char[capacity];
            do {
                if ( length + 1 >= capacity ) {
                    char *bigger = new char[capacity * 2];
                    for ( int i = 0; i < length; i++ ) bigger[i] = buf[i];
                    delete[] buf;
                    buf = bigger;
                    capacity *= 2;
                }
                buf[length++] = (char)c;
                c = reader.read();
                ct = c < 0 ? (int)CT_WHITESPACE : typeOf(c);
            } while ( (ct & (CT_ALPHA | CT_DIGIT)) != 0 );
            buf[length] = '\0';
            peekc = c;
            sval = buf;
            delete[] buf;
            return ttype = TT_WORD;
        }

        if ( (ct & CT_COMMENT) != 0 ) {
            while ( (c = reader.read()) != '\n' && c != '\r' && c >= 0 ) {
            }
            if ( c < 0 ) {
                return ttype = TT_EOF;
            }
            // c is the line terminator: process it as the next char
            ttype = c;
            continue;
        }

        return ttype = c;
    }
}

}
