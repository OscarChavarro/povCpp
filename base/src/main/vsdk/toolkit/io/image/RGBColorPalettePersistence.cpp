#include "java/io/StreamTokenizer.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/media/RGBColorPalette.h"
#include "vsdk/toolkit/io/image/RGBColorPalettePersistence.h"

RGBColorPalette*
RGBColorPalettePersistence::importGimpPalette(java::Reader& source)
{
    RGBColorPalette* p = new RGBColorPalette();
    p->init(0);

    java::StreamTokenizer parser(source);

    parser.resetSyntax();
    parser.eolIsSignificant(true);
    parser.slashSlashComments(false);
    parser.slashStarComments(false);
    parser.commentChar('#');
    parser.whitespaceChars(' ', ' ');
    parser.whitespaceChars(',', ',');
    parser.whitespaceChars('\t', '\t');
    parser.wordChars('A', 'Z');
    parser.wordChars('a', 'z');
    parser.wordChars('0', '9');
    parser.wordChars('_', '_');
    parser.parseNumbers();

    int tokenType;
    int startline = 0;
    double r = 0.0;
    double g = 0.0;

    do {
        tokenType = parser.nextToken();
        switch ( tokenType ) {
          case java::StreamTokenizer::TT_EOL:
            startline = 0;
            break;
          case java::StreamTokenizer::TT_NUMBER:
            switch ( startline ) {
              case 0:
                r = (parser.nval)/255.0;
                break;
              case 1:
                g = (parser.nval)/255.0;
                break;
              case 2:
                p->addColor(r, g, (parser.nval)/255.0);
                break;
            }
            startline++;
            break;
          default:
            break;
        }
    } while ( tokenType != java::StreamTokenizer::TT_EOF );

    return p;
}

RGBColorPalette*
RGBColorPalettePersistence::importRawPalette(java::InputStream& source)
{
    RGBColorPalette* p = new RGBColorPalette();
    p->init(0);

    for ( ;; ) {
        int nr = source.read();
        int ng = source.read();
        int nb = source.read();
        if ( nr < 0 || ng < 0 || nb < 0 ) {
            break;
        }
        p->addColor((double)nr, (double)ng, (double)nb);
    }

    source.close();
    return p;
}
