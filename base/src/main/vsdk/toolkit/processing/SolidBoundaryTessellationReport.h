#ifndef VSDK_SOLID_BOUNDARY_TESSELLATION_REPORT_H
#define VSDK_SOLID_BOUNDARY_TESSELLATION_REPORT_H

// Resolution and accuracy of a SolidBoundaryTessellator mesh.
class SolidBoundaryTessellationReport {
public:
    // Side of the cubic cells of the sampling grid, in world units.
    double cellSize;
    // Grid nodes along x, y and z.
    long int nodeCounts[3];
    // Grid lines whose ray classification failed; their nodes were
    // classified one by one.
    long int pointClassifiedLines;
    // Nodes whose containment stayed unknown; they count as outside.
    long int unknownNodes;
    // Edge crossings found by a ray hit on the body; the others are the edge
    // midpoints.
    long int exactCrossings;
    long int midpointCrossings;
    // Mesh vertices moved onto the body's boundary by a ray hit; the others
    // stay at the mean of their cell's crossings.
    long int boundaryVertices;
    long int cellVertices;
    // Crossings on the region's sides, where the mesh closes a solid that
    // continues beyond the region.
    long int regionCrossings;
    long int triangles;

    SolidBoundaryTessellationReport();
    void clear();
};

#endif
