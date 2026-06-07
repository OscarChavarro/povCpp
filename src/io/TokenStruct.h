#ifndef __TOKEN_STRUCT_H__
#define __TOKEN_STRUCT_H__


class TokenStruct {
  public:
    int tokenId;
    int tokenLineNo;
    int tokenColumnNo;
    char *Token_String;
    double tokenFloat;
    int identifierNumber;
    bool ungetToken;
    bool endOfFile;
    char *Filename;
};

#endif
