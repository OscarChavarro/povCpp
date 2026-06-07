#ifndef __POV_SYMBOL_TABLE_H__
#define __POV_SYMBOL_TABLE_H__

#include "io/pov/context/ParseGlobals.h"

class SymbolTable {
  public:
    static constexpr int MAX_CONSTANTS = 1000;

    SymbolTable();

    Constant *data();
    const Constant *data() const;
    int &size();
    bool isValidConstantId(int constantId) const;
    Constant *byConstantId(int constantId);
    const Constant *byConstantId(int constantId) const;
    void clear();
    int findByIdentifierNumber(int identifierNumber) const;
    Constant *upsertByIdentifierNumber(int identifierNumber);

  private:
    Constant mData[SymbolTable::MAX_CONSTANTS];
    int mSize;
};

#endif
