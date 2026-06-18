#ifndef __POV_TOKEN__
#define __POV_TOKEN__

class PovToken {
  private:
    int tokenId;
    int tokenLineNumber;
    int tokenColumnNumber;
    char *tokenString;
    double tokenFloat;
    bool endOfFile;
    int identifierNumber;
    bool unGetToken;
    char *fileName;

  public:
    int getTokenId() const { return tokenId; }
    void setTokenId(int value) { tokenId = value; }
    int getTokenLineNumber() const { return tokenLineNumber; }
    void setTokenLineNumber(int value) { tokenLineNumber = value; }
    int getTokenColumnNumber() const { return tokenColumnNumber; }
    void setTokenColumnNumber(int value) { tokenColumnNumber = value; }
    char *getTokenString() const { return tokenString; }
    void setTokenString(char *value) { tokenString = value; }
    double getTokenFloat() const { return tokenFloat; }
    void setTokenFloat(double value) { tokenFloat = value; }
    bool isEndOfFile() const { return endOfFile; }
    void setEndOfFile(bool value) { endOfFile = value; }
    int getIdentifierNumber() const { return identifierNumber; }
    void setIdentifierNumber(int value) { identifierNumber = value; }
    bool isUnGetToken() const { return unGetToken; }
    void setUnGetToken(bool value) { unGetToken = value; }
    char *getFileName() const { return fileName; }
    void setFileName(char *value) { fileName = value; }
};

#endif
