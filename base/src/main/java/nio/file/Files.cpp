#include "java/io/File.h"
#include "java/io/FileInputStream.h"
#include "java/lang/StringBuilder.h"
#include "java/nio/file/Files.h"

namespace java {

bool
Files::readString(const String &path, String &outText)
{
    File file(path);
    if ( !file.isFile() || !file.canRead() ) {
        return false;
    }
    FileInputStream input(path.c_str());
    StringBuilder text;
    unsigned char buffer[4096];
    int count;
    while ( (count = input.read(buffer, 0, ((int)(sizeof(buffer))))) > 0 ) {
        text.append(((const char *)(buffer)), count);
    }
    input.close();
    outText = text.toString();
    return true;
}

}
