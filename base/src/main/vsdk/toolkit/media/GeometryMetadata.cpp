#include "java/lang/Double.h"
#include <cmath>
#include <cstdio>

#include "java/lang/String.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/media/GeometryMetadata.h"
#include "vsdk/toolkit/media/ShapeDescriptor.h"
long GeometryMetadata::lastId = 0;

GeometryMetadata::GeometryMetadata() : filename(nullptr) {
    lastId++;
    id = lastId;
}

GeometryMetadata::~GeometryMetadata() {
    for (long int i = 0; i < descriptors.size(); i++) {
        if (descriptors[i] != nullptr) {
            delete descriptors[i];
            descriptors[i] = nullptr;
        }
    }
    descriptors.clear();

    if (filename != nullptr) {
        delete filename;
        filename = nullptr;
    }
}

double GeometryMetadata::doMinskowskiDistance(
    const GeometryMetadata* other,
    double s,
    const java::String* subGroup) const {
    if (other == nullptr || subGroup == nullptr) {
        return java::Double::MAX_VALUE;
    }

    ShapeDescriptor* a = nullptr;
    ShapeDescriptor* b = nullptr;

    for (long int i = 0; i < descriptors.size(); i++) {
        ShapeDescriptor* aa = descriptors.get(i);
        if (aa != nullptr) {
            java::String* label = aa->getLabel();
            if (label != nullptr && label->equals(*subGroup)) {
                a = aa;
            }
            delete label;
        }
    }
    if (a == nullptr) {
        return java::Double::MAX_VALUE;
    }

    for (long int i = 0; i < other->descriptors.size(); i++) {
        ShapeDescriptor* bb = other->descriptors.get(i);
        if (bb != nullptr) {
            java::String* label = bb->getLabel();
            if (label != nullptr && label->equals(*subGroup)) {
                b = bb;
            }
            delete label;
        }
    }
    if (b == nullptr) {
        return java::Double::MAX_VALUE;
    }

    double* av = a->getFeatureVector();
    double* bv = b->getFeatureVector();

    if (av == nullptr || bv == nullptr) {
        if (av != nullptr) delete[] av;
        if (bv != nullptr) delete[] bv;
        return java::Double::MAX_VALUE;
    }

    int avLength = 0;
    int bvLength = 0;

    for (int i = 0; i < 1024; i++) {
        if (av[i] == 0 && i > 0) {
            avLength = i;
            break;
        }
    }

    for (int i = 0; i < 1024; i++) {
        if (bv[i] == 0 && i > 0) {
            bvLength = i;
            break;
        }
    }

    if (avLength != bvLength) {
        delete[] av;
        delete[] bv;
        return java::Double::MAX_VALUE;
    }

    double acum = 0.0;
    for (int j = 0; j < avLength; j++) {
        acum += std::pow(std::abs(av[j] - bv[j]), s);
    }

    delete[] av;
    delete[] bv;

    return std::pow(acum, 1.0 / s);
}

void GeometryMetadata::setId(long id_) {
    id = id_;
    if (lastId < id_) {
        lastId = id_;
    }
}

long GeometryMetadata::getId() const {
    return id;
}

void GeometryMetadata::setFilename(const java::String* filename) {
    if (filename != nullptr && filename->length() > 0) {
        if (this->filename != nullptr) {
            delete this->filename;
        }
        this->filename = new java::String(*filename);
    } else {
        if (this->filename != nullptr) {
            delete this->filename;
            this->filename = nullptr;
        }
    }
}

java::String* GeometryMetadata::getFilename() const {
    if (filename != nullptr) {
        return new java::String(*filename);
    }
    return nullptr;
}

java::ArrayList<ShapeDescriptor*>& GeometryMetadata::getDescriptors() {
    return descriptors;
}

const java::ArrayList<ShapeDescriptor*>& GeometryMetadata::getDescriptors() const {
    return descriptors;
}

ShapeDescriptor* GeometryMetadata::getDescriptorByName(const java::String* name) const {
    if (name == nullptr) {
        return nullptr;
    }

    for (long int i = 0; i < descriptors.size(); i++) {
        ShapeDescriptor* s = descriptors.get(i);
        if (s != nullptr) {
            java::String* label = s->getLabel();
            if (label != nullptr && label->equals(*name)) {
                delete label;
                return s;
            }
            delete label;
        }
    }
    return nullptr;
}

void GeometryMetadata::addDescriptor(ShapeDescriptor* descriptor) {
    if (descriptor != nullptr) {
        descriptors.add(descriptor);
    }
}

java::String* GeometryMetadata::toString() const {
    char fullBuffer[4096];
    int offset = 0;
    const char* filename_str = filename != nullptr ? filename->toCString() : "nullptr";

    offset += snprintf(fullBuffer + offset, sizeof(fullBuffer) - offset,
        "%s\n    . %zu shape descriptors\n",
        filename_str, descriptors.size());

    for (long int i = 0; i < descriptors.size(); i++) {
        offset += snprintf(fullBuffer + offset, sizeof(fullBuffer) - offset, "        . descriptor_%ld\n", i);
    }

    java::String* msg = new java::String(fullBuffer);
    return msg;
}
