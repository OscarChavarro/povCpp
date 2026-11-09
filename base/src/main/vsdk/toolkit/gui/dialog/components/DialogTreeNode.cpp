#include "java/lang/String.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/gui/dialog/components/DialogTreeNode.h"

DialogTreeNode::DialogTreeNode(const java::String& label, const void* identity)
    : label(label), identity(identity)
{
}

const java::String& DialogTreeNode::getLabel() const
{
    return label;
}

const void* DialogTreeNode::getIdentity() const
{
    return identity;
}

const java::ArrayList<DialogTreeNode>& DialogTreeNode::getChildren() const
{
    return children;
}

DialogTreeNode& DialogTreeNode::add(const DialogTreeNode& child)
{
    children.add(child);
    return children[children.size() - 1];
}

java::String DialogTreeNode::toString() const
{
    return label;
}
