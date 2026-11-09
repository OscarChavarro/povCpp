#ifndef __VERTEX_NODE_2_D__
#define __VERTEX_NODE_2_D__

#include "vsdk/toolkit/common/color/ColorRgb.h"
template <class E> class _DoubleLinkedListNode;

class _VertexNode2D {
public:
    double x;
    double y;
    ColorRgb color;
    unsigned char flags;
    _DoubleLinkedListNode<_VertexNode2D>* pairNode;

    _VertexNode2D() : x(0), y(0), color(), flags(0), pairNode(0) {}
    _VertexNode2D(double x, double y) : x(x), y(y), color(), flags(0), pairNode(0) {}
    _VertexNode2D(double x, double y, double r, double g, double b)
        : x(x), y(y), color(r, g, b), flags(0), pairNode(0) {}
    _VertexNode2D(const _VertexNode2D& other)
        : x(other.x), y(other.y), color(other.color.r(), other.color.g(), other.color.b()),
          flags(other.flags), pairNode(other.pairNode) {}
};

#endif
