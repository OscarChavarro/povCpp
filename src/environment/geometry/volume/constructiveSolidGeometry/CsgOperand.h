#ifndef __CSG_OPERAND__
#define __CSG_OPERAND__

#include "environment/geometry/TransformedGeometry.h"
#include "environment/material/Material.h"

class CsgOperand {
  private:
    TransformedGeometry *geometry = nullptr;
    Material *material = nullptr;

  public:
    CsgOperand(TransformedGeometry *geometry, Material *material) :
        geometry(geometry), material(material)
    {
    }

    CsgOperand(const CsgOperand &other) :
        geometry(other.geometry != nullptr ?
            (TransformedGeometry *)other.geometry->copy() : nullptr),
        material(other.material != nullptr ? other.material->copy() : nullptr)
    {
    }

    ~CsgOperand()
    {
        delete geometry;
        delete material;
    }

    CsgOperand *copy() const { return new CsgOperand(*this); }

    TransformedGeometry *getGeometry() const { return geometry; }
    Material *getMaterial() const { return material; }
    Material *getEffectiveMaterial(Material *materialOverride) const
    {
        return material != nullptr ? material : materialOverride;
    }

    void translate(Vector3Dd *vector)
    {
        if (geometry != nullptr) {
            geometry->translateGeometry(vector);
        }
        if (material != nullptr) {
            material = material->translate(vector);
        }
    }

    void rotate(Vector3Dd *vector)
    {
        if (geometry != nullptr) {
            geometry->rotateGeometry(vector);
        }
        if (material != nullptr) {
            material = material->rotate(vector);
        }
    }

    void scale(Vector3Dd *vector)
    {
        if (geometry != nullptr) {
            geometry->scaleGeometry(vector);
        }
        if (material != nullptr) {
            material = material->scale(vector);
        }
    }

    void invert()
    {
        if (geometry != nullptr) {
            geometry->invertGeometry();
        }
    }
};

#endif
