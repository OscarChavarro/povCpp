#ifndef __READER_ALTIMETRY__
#define __READER_ALTIMETRY__

class HeightFieldSolid;

// Reads a text altimetry grid into a height field. The format is the header
// "altimetry <columns> <rows>" followed by columns*rows heights, row by row
// from z = 0, each row from x = 0; "#" starts a comment up to the end of the
// line. Both counts must be at least 2. The solid's box spans the whole grid,
// from the lowest to the highest height; when every height is equal the box
// is one unit tall above them.
class ReaderAltimetry {
public:
    // Throws VSDKFatalException for unreadable or malformed files.
    static HeightFieldSolid* importHeightField(const char* fileName);
};

#endif
