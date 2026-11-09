#include "vsdk/toolkit/gui/widget/WidgetMenuElement.h"

namespace {

void appendChar(java::String& out, char c)
{
    char buffer[2] = {c, '\0'};
    out += buffer;
}

void appendUtf8(java::String& out, unsigned int cp)
{
    char buffer[4] = {'\0', '\0', '\0', '\0'};
    if ( cp < 0x80 ) {
        buffer[0] = (char)cp;
    }
    else if ( cp < 0x800 ) {
        buffer[0] = (char)(0xC0 | (cp >> 6));
        buffer[1] = (char)(0x80 | (cp & 0x3F));
    }
    else {
        buffer[0] = (char)(0xE0 | (cp >> 12));
        buffer[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        buffer[2] = (char)(0x80 | (cp & 0x3F));
    }
    out += buffer;
}

}

int WidgetMenuElement::fromHex(char c)
{
    if ( c >= '0' && c <= '9' ) {
        return c - '0';
    }
    if ( c >= 'a' && c <= 'f' ) {
        return c - 'a' + 10;
    }
    if ( c >= 'A' && c <= 'F' ) {
        return c - 'A' + 10;
    }
    return -1;
}

java::String WidgetMenuElement::processSimplifiedName(
    const java::String& codedName)
{
    java::String simplifiedName = "";

    int i;
    char c;

    for ( i = 0; i < codedName.length(); i++ ) {
        c = codedName.charAt(i);
        if ( c == '&' ) continue;

        if ( c == '\t' ) {
            break;
        }

        if ( c == '#' ) {
            // Process UNICODE escape sequences...
            int start = i;
            i++;
            int num1 = 0;
            int num2 = 0;
            int num3 = 0;
            int num4;
            int num;
            for ( ; i < codedName.length(); i++ ) {
                c = codedName.charAt(i);
                if ( c == '#' ) {
                    appendChar(simplifiedName, c);
                    break;
                }
                num = fromHex(c);
                if ( num < 0 ) {
                    break;
                }
                if ( i == start+1 ) {
                    num1 = num;
                }
                else if ( i == start+2 ) {
                    num2 = num;
                }
                else if ( i == start+3 ) {
                    num3 = num;
                }
                else if ( i == start+4 ) {
                    num4 = num;
                    appendUtf8(simplifiedName,
                        (unsigned int)(num1 << 12 | num2 << 8 | num3 << 4 | num4));
                    break;
                }
            }
        }
        else {
            appendChar(simplifiedName, c);
        }
    }

    return simplifiedName;
}

char WidgetMenuElement::processMnemonic(const java::String& codedName)
{
    int i;
    char c;

    for ( i = 0; i < codedName.length()-1; i++ ) {
        c = codedName.charAt(i);
        if ( c == '&' ) {
            i++;
            c = codedName.charAt(i);
            return c;
        }
    }

    return '\0';
}

java::String WidgetMenuElement::processAccelerator(
    const java::String& codedName)
{
    java::String accelerator = "";

    int i;
    char c;

    for ( i = 0; i < codedName.length(); i++ ) {
        c = codedName.charAt(i);
        if ( c == '\t' ) break;
    }

    for ( i++; i < codedName.length(); i++ ) {
        c = codedName.charAt(i);
        appendChar(accelerator, c);
    }

    return accelerator;
}
