#ifndef __READER_PLY__
#define __READER_PLY__

#include "java/io/File.h"
#include "java/io/InputStream.h"
#include "vsdk/toolkit/environment/scene/SimpleScene.h"
#include "vsdk/toolkit/io/PersistenceElement.h"
#include "vsdk/toolkit/io/geometry/ReaderPlyResult.h"

class ReaderPly : public PersistenceElement {
  public:
    static ReaderPlyResult* importGeometry(const java::File& sceneFile);
    static ReaderPlyResult* importGeometry(java::InputStream& inputStream);
    static void importEnvironment(const java::File& sceneFile, SimpleScene* scene);
};

#endif
