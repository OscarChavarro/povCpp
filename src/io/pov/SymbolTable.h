#ifndef __POV_SYMBOL_TABLE_H__
#define __POV_SYMBOL_TABLE_H__

#include "io/pov/ParseGlobals.h"
#include "io/pov/ParserConstants.h"

class SymbolTable {
  public:
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
    Constant mData[ParserConstants::MAX_CONSTANTS];
    int mSize;
};

#endif
