#ifndef __TOKEN_STRUCT_H__
#define __TOKEN_STRUCT_H__

#include "common/frame.h"

class TokenStruct {
  public:
    int Token_Id;
    int Token_Line_No;
    char *Token_String;
    DBL Token_Float;
    int Identifier_Number;
    int Unget_Token, End_Of_File;
    char *Filename;
};

#endif
