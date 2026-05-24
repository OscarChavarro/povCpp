#ifndef __REWINDABLE_TOKEN_STREAM_H__
#define __REWINDABLE_TOKEN_STREAM_H__

#include <cstddef>
#include <vector>

#include "io/pov/ITokenStream.h"

class RewindableTokenStream : public ITokenStream {
  public:
    RewindableTokenStream();
    explicit RewindableTokenStream(ITokenStream *source);

    void setSource(ITokenStream *source);
    void clear();

    ReservedWord *reservedWords() override;
    TokenStruct &token() override;
    void getToken() override;
    void ungetToken() override;
    bool canRewind() const override;
    int mark() override;
    bool rewind(int marker) override;

  private:
    ITokenStream *mSource;
    std::vector<TokenStruct> mHistory;
    std::size_t mCursor;
    TokenStruct mCurrentToken;
};

#endif
