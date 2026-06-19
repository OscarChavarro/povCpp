#ifndef __DATA_FILE__
#define __DATA_FILE__

#include <cstdio>

class Tokenizer;

class DataFile {
  public:
    FILE *getFile() const;
    void setFile(FILE *value);
    char *getFilename() const;
    void setFilename(char *value);
    Tokenizer *getTokenizer() const;
    void setTokenizer(Tokenizer *value);
    int getLineNumber() const;
    void setLineNumber(int value);
    void incrementLineNumber();

    int skipSpaces();
    int parseComments();
    int parseCComments();
    void endString();
    int readFloat();
    void parseString();
    int readSymbol();

  private:
    FILE *File;
    char *Filename;
    Tokenizer *tokenizer;
    int lineNumber;
};

inline FILE *
DataFile::getFile() const
{
    return File;
}

inline void
DataFile::setFile(FILE *value)
{
    File = value;
}

inline char *
DataFile::getFilename() const
{
    return Filename;
}

inline void
DataFile::setFilename(char *value)
{
    Filename = value;
}

inline Tokenizer *
DataFile::getTokenizer() const
{
    return tokenizer;
}

inline void
DataFile::setTokenizer(Tokenizer *value)
{
    tokenizer = value;
}

inline int
DataFile::getLineNumber() const
{
    return lineNumber;
}

inline void
DataFile::setLineNumber(int value)
{
    lineNumber = value;
}

inline void
DataFile::incrementLineNumber()
{
    lineNumber++;
}

#endif
