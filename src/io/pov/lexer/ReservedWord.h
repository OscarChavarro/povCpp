#ifndef __RESERVED_WORD__
#define __RESERVED_WORD__

class ReservedWord {
  public:
    ReservedWord(int tokenNumber, const char *tokenName);
    int getTokenNumber() const;
    void setTokenNumber(int value);
    const char *getTokenName() const;
    void setTokenName(const char *value);

  private:
    int tokenNumber;
    const char *tokenName;
};

inline ReservedWord::ReservedWord(int tokenNumberValue, const char *tokenNameValue)
    : tokenNumber(tokenNumberValue), tokenName(tokenNameValue)
{
}

inline int ReservedWord::getTokenNumber() const { return tokenNumber; }
inline void ReservedWord::setTokenNumber(int value) { tokenNumber = value; }
inline const char *ReservedWord::getTokenName() const { return tokenName; }
inline void ReservedWord::setTokenName(const char *value) { tokenName = value; }

#endif
