#ifndef __DATA_FILE_H__
#define __DATA_FILE_H__

#include <stdio.h>

class DataFile {
  public:
    FILE *File;
    char *Filename;
    int Line_Number;

    int skipSpaces();
    int parseComments();
    int parseCComments();
    void endString();
    int readFloat();
    void parseString();
    int readSymbol();
};

#endif
