#include <cstring>
#include "io/pov/context/SymbolTable.h"

SymbolTable::SymbolTable()
{
    clear();
}

Constant *
SymbolTable::data()
{
    return mData;
}

const Constant *
SymbolTable::data() const
{
    return mData;
}

int &
SymbolTable::size()
{
    return mSize;
}

bool
SymbolTable::isValidConstantId(int constantId) const
{
    return constantId >= 1 && constantId <= mSize;
}

Constant *
SymbolTable::byConstantId(int constantId)
{
    if (!isValidConstantId(constantId)) {
        return nullptr;
    }
    return &mData[constantId];
}

const Constant *
SymbolTable::byConstantId(int constantId) const
{
    if (!isValidConstantId(constantId)) {
        return nullptr;
    }
    return &mData[constantId];
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
    const int idx = findByIdentifierNumber(identifierNumber);
    if (idx != -1) {
        return &mData[idx];
    }

    if (++mSize >= SymbolTable::MAX_CONSTANTS) {
        return nullptr;
    }

    std::memset(&mData[mSize], 0, sizeof(Constant));
    mData[mSize].identifierNumber = identifierNumber;
    return &mData[mSize];
}
