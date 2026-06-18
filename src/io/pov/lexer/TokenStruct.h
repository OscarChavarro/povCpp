#ifndef __TOKEN_STRUCT_H__
#define __TOKEN_STRUCT_H__


class TokenStruct {
  public:
    int getTokenId() const { return tokenId; }
    void setTokenId(int value) { tokenId = value; }
    int getTokenLineNo() const { return tokenLineNo; }
    void setTokenLineNo(int value) { tokenLineNo = value; }
    int tokenColumnNo;
    char *tokenString;
    double tokenFloat;
    int identifierNumber;
    bool ungetToken;
    bool endOfFile;
    char *fileName;

  private:
    int tokenId;
    int tokenLineNo;
};

#endif
