#include "vsdk/toolkit/processing/SolidBoundaryTessellationReport.h"

SolidBoundaryTessellationReport::SolidBoundaryTessellationReport()
{
    clear();
}

void SolidBoundaryTessellationReport::clear()
{
    cellSize = 0;
    nodeCounts[0] = nodeCounts[1] = nodeCounts[2] = 0;
    pointClassifiedLines = 0;
    unknownNodes = 0;
    exactCrossings = 0;
    midpointCrossings = 0;
    boundaryVertices = 0;
    cellVertices = 0;
    regionCrossings = 0;
    triangles = 0;
}
