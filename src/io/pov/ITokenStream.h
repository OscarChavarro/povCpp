#ifndef __I_TOKEN_STREAM_H__
#define __I_TOKEN_STREAM_H__

#include "io/ReservedWord.h"
#include "io/TokenStruct.h"

class ITokenStream {
  public:
    virtual ~ITokenStream() = default;
    virtual ReservedWord *reservedWords() = 0;
    virtual TokenStruct &token() = 0;
    virtual void getToken() = 0;
    virtual void ungetToken() = 0;
};

#endif
