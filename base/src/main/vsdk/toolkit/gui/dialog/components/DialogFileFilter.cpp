#include "java/lang/String.h"
#include <cctype>

#include "vsdk/toolkit/gui/dialog/components/DialogFileFilter.h"

namespace {
java::String lowerCase(const char* text)
{
    java::String result(text != nullptr ? text : "");
    for ( int i = 0; i < result.size(); i++ ) {
        result[i] = static_cast<char>(
            std::tolower(static_cast<unsigned char>(result[i])));
    }
    return result;
}
}

DialogFileFilter::DialogFileFilter(const java::String& suffix,
                                   const java::String& description)
    : suffix(lowerCase(suffix.c_str()).c_str()), text(description)
{
}

bool DialogFileFilter::accepts(const java::String& path, bool directory) const
{
    if ( directory ) {
        return true;
    }
    java::String name(path.c_str());
    int dot = name.rfind('.');
    return dot != -1 && dot > 0 && dot < name.size() - 1 &&
        lowerCase(name.substr(dot + 1).c_str()) == suffix.c_str();
}

const java::String& DialogFileFilter::getSuffix() const
{
    return suffix;
}

java::String DialogFileFilter::description() const
{
    return text + " (*." + suffix + ")";
}
