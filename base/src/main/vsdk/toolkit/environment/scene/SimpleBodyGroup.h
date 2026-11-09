#ifndef __SIMPLE_BODY_GROUP__
#define __SIMPLE_BODY_GROUP__

#include "vsdk/toolkit/common/Entity.h"
#include "java/lang/String.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
class SimpleBody;
class Ray;

class SimpleBodyGroup : public Entity {
private:
    java::ArrayList<SimpleBody*> bodies;
    Vector3Dd position;
    Vector3Dd scale;
    Matrix4x4d rotation;
    Matrix4x4d rotationInverse;
    java::String name;

public:
    SimpleBodyGroup();
    virtual ~SimpleBodyGroup() {}

    java::ArrayList<SimpleBody*>& getBodies();

    const java::String& getName() const;
    void setName(const java::String& n);

    Matrix4x4d getRotation() const;
    void setRotation(const Matrix4x4d& rotation);

    Matrix4x4d getRotationInverse() const;
    void setRotationInverse(const Matrix4x4d& value);

    Vector3Dd getPosition() const;
    void setPosition(const Vector3Dd& p);

    Vector3Dd getScale() const;
    void setScale(const Vector3Dd& s);

    Matrix4x4d getTransformationMatrix() const;

    Ray* doIntersectionFirstHit(const Ray& inOutRay);
    double* getMinMax();
};

#endif
