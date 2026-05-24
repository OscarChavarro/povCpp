#ifndef __POV_SYMBOL_TABLE_H__
#define __POV_SYMBOL_TABLE_H__

#include "io/pov/ParseGlobals.h"
#include "io/pov/ParserConstants.h"

class SymbolTable {
  public:
    SymbolTable();

    Constant *data();
    int &size();
    void clear();
    int findByIdentifierNumber(int identifierNumber) const;
    Constant *upsertByIdentifierNumber(int identifierNumber);

  private:
    Constant mData[MAX_CONSTANTS];
    int mSize;
};

#endif
