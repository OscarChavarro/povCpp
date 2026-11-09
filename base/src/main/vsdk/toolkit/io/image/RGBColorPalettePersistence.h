#ifndef __RGB_COLOR_PALETTE_PERSISTENCE__
#define __RGB_COLOR_PALETTE_PERSISTENCE__

#include "java/io/InputStream.h"
#include "java/io/Reader.h"
#include "vsdk/toolkit/io/PersistenceElement.h"
class RGBColorPalette;

/**
This class contains the persistence operations to load and save
RGBColorPalettes.

Ownership: the caller owns the returned palette (which owns its colors).

Differences with the Java port: importRawPalette takes a java::InputStream
(there is no DataInputStream / available() in the C++ port) and reads until
read() returns -1; an incomplete last triple is discarded (Java would throw
EOFException). Raw values are NOT divided by 255, bug-compatible with Java.
*/
class RGBColorPalettePersistence : public PersistenceElement {
  public:
    /**
    This method reads a text formated stream in the style of GIMP
    palettes and from its data generates a RGBColorPalette
    @return the RGBColorPalette builded from the source
    */
    static RGBColorPalette* importGimpPalette(java::Reader& source);

    /** This method reads a binary raw stream of consecutive RGB color values */
    static RGBColorPalette* importRawPalette(java::InputStream& source);
};

#endif
