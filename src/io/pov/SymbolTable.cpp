#include "io/pov/SymbolTable.h"

#include <cstring>

SymbolTable::SymbolTable()
{
    clear();
}

Constant *
SymbolTable::data()
{
    return mData;
}

int &
SymbolTable::size()
{
    return mSize;
}

void
SymbolTable::clear()
{
    mSize = 0;
    std::memset(mData, 0, sizeof(mData));
}

int
SymbolTable::findByIdentifierNumber(int identifierNumber) const
{
    for (int i = 1; i <= mSize; ++i) {
        if (mData[i].identifierNumber == identifierNumber) {
            return i;
        }
    }
    return -1;
}

Constant *
SymbolTable::upsertByIdentifierNumber(int identifierNumber)
{
    int idx = findByIdentifierNumber(identifierNumber);
    if (idx != -1) {
        return &mData[idx];
    }

    if (++mSize >= ParserConstants::MAX_CONSTANTS) {
        return nullptr;
    }

    mData[mSize].identifierNumber = identifierNumber;
    return &mData[mSize];
}
