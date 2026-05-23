#ifndef __TOKEN_STRUCT_H__
#define __TOKEN_STRUCT_H__

#include "common/LegacyBoolean.h"

class TokenStruct {
  public:
    int tokenId;
    int tokenLineNo;
    char *Token_String;
    double tokenFloat;
    int identifierNumber;
    int ungetToken;
    int endOfFile;
    char *Filename;
};

#endif
