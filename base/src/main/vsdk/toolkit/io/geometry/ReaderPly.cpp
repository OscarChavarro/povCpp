#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "java/io/FileInputStream.h"
#include "java/lang/String.h"
#include "java/util/ArrayList.txx"
#include "java/util/concurrent/Callable.h"
#include "java/util/concurrent/ExecutorService.h"
#include "java/util/concurrent/Executors.h"
#include "java/util/concurrent/Future.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/environment/geometry/element/Triangle.h"
#include "vsdk/toolkit/environment/geometry/element/Vertex.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMesh.h"
#include "vsdk/toolkit/environment/material/SimpleMaterial.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/environment/scene/SimpleScene.h"
#include "vsdk/toolkit/io/geometry/ReaderPly.h"

enum ReaderPlyFormat {
    READER_PLY_ASCII,
    READER_PLY_BINARY_LITTLE_ENDIAN,
    READER_PLY_BINARY_BIG_ENDIAN
};

struct ReaderPlyProperty {
    bool isList;
    java::String name;
    java::String type;
    java::String countType;
    java::String valueType;

    ReaderPlyProperty()
        : isList(false), name(), type(), countType(), valueType()
    {
    }
};

struct ReaderPlyElement {
    java::String name;
    int count;
    java::ArrayList<ReaderPlyProperty> properties;

    ReaderPlyElement() : name(), count(0), properties()
    {
    }
};

struct ReaderPlyHeader {
    ReaderPlyFormat format;
    java::ArrayList<ReaderPlyElement> elements;

    ReaderPlyHeader() : format(READER_PLY_ASCII), elements()
    {
    }
};

struct ReaderPlyTriangleIndices {
    int point0;
    int point1;
    int point2;

    ReaderPlyTriangleIndices() : point0(0), point1(0), point2(0)
    {
    }

    ReaderPlyTriangleIndices(int inA, int inB, int inC) : point0(inA), point1(inB), point2(inC)
    {
    }
};

static java::ArrayList<java::String> splitWhitespace(const java::String& line)
{
    java::ArrayList<java::String> tokens;
    int i = 0;
    const int n = line.size();

    while ( i < n ) {
        while ( i < n && isspace((unsigned char)line[i]) ) {
            i++;
        }
        if ( i >= n ) {
            break;
        }
        int start = i;
        while ( i < n && !isspace((unsigned char)line[i]) ) {
            i++;
        }
        tokens.add(line.substr(start, i - start));
    }
    return tokens;
}

static bool readPlyAsciiLine(java::InputStream& inputStream, java::String* outLine)
{
    if ( outLine == 0 ) {
        return false;
    }

    char* rawLine = PersistenceElement::readAsciiLine(inputStream);
    if ( rawLine == 0 ) {
        return false;
    }
    *outLine = java::String(rawLine);
    free(rawLine);
    return true;
}

static bool readHeader(java::InputStream& inputStream, ReaderPlyHeader* header)
{
    if ( header == 0 ) {
        return false;
    }

    java::String line;
    if ( !readPlyAsciiLine(inputStream, &line) ) {
        return false;
    }
    java::ArrayList<java::String> tokens = splitWhitespace(line);
    if ( tokens.size() < 1 || tokens[0] != "ply" ) {
        Logger::reportMessage("ReaderPly", Logger::ERROR, "readHeader", "input is not a PLY stream");
        return false;
    }

    int currentElement = -1;
    while ( readPlyAsciiLine(inputStream, &line) ) {
        tokens = splitWhitespace(line);
        if ( tokens.size() == 0 ) {
            continue;
        }

        if ( tokens[0] == "end_header" ) {
            return true;
        }
        if ( tokens[0] == "comment" || tokens[0] == "obj_info" ) {
            continue;
        }
        if ( tokens[0] == "format" && tokens.size() >= 2 ) {
            if ( tokens[1] == "ascii" ) {
                header->format = READER_PLY_ASCII;
            }
            else if ( tokens[1] == "binary_little_endian" ) {
                header->format = READER_PLY_BINARY_LITTLE_ENDIAN;
            }
            else if ( tokens[1] == "binary_big_endian" ) {
                header->format = READER_PLY_BINARY_BIG_ENDIAN;
            }
            else {
                Logger::reportMessage("ReaderPly", Logger::ERROR, "readHeader", "unsupported PLY format");
                return false;
            }
            continue;
        }
        if ( tokens[0] == "element" && tokens.size() >= 3 ) {
            ReaderPlyElement element;
            element.name = tokens[1];
            element.count = atoi(tokens[2].c_str());
            header->elements.add(element);
            currentElement = (int)header->elements.size() - 1;
            continue;
        }
        if ( tokens[0] == "property" && currentElement >= 0 ) {
            ReaderPlyProperty property;
            if ( tokens.size() >= 5 && tokens[1] == "list" ) {
                property.isList = true;
                property.countType = tokens[2];
                property.valueType = tokens[3];
                property.name = tokens[4];
            }
            else if ( tokens.size() >= 3 ) {
                property.isList = false;
                property.type = tokens[1];
                property.name = tokens[2];
            }
            header->elements[currentElement].properties.add(property);
        }
    }

    Logger::reportMessage("ReaderPly", Logger::ERROR, "readHeader", "missing end_header");
    return false;
}

static int plyTypeSize(const java::String& type)
{
    if ( type == "char" || type == "int8" || type == "uchar" || type == "uint8" ) {
        return 1;
    }
    if ( type == "short" || type == "int16" || type == "ushort" || type == "uint16" ) {
        return 2;
    }
    if ( type == "int" || type == "int32" || type == "uint" || type == "uint32" ||
         type == "float" || type == "float32" ) {
        return 4;
    }
    if ( type == "double" || type == "float64" ) {
        return 8;
    }
    return 0;
}

static unsigned long long readUnsignedIntegerBytes(
    java::InputStream& inputStream, int bytes, bool bigEndian)
{
    unsigned char buffer[8];
    for ( int i = 0; i < 8; i++ ) {
        buffer[i] = 0;
    }

    PersistenceElement::readBytes(inputStream, buffer, bytes);

    unsigned long long value = 0;
    if ( bigEndian ) {
        for ( int i = 0; i < bytes; i++ ) {
            value = (value << 8) | ((unsigned long long)buffer[i]);
        }
    }
    else {
        for ( int i = bytes - 1; i >= 0; i-- ) {
            value = (value << 8) | ((unsigned long long)buffer[i]);
        }
    }
    return value;
}

static long long unsignedToSigned(unsigned long long value, int bytes)
{
    if ( bytes == 1 && value >= 128ULL ) {
        return (long long)value - 256LL;
    }
    if ( bytes == 2 && value >= 32768ULL ) {
        return (long long)value - 65536LL;
    }
    if ( bytes == 4 && value >= 2147483648ULL ) {
        return (long long)value - 4294967296LL;
    }
    return (long long)value;
}

static bool isSignedIntegerType(const java::String& type)
{
    return type == "char" || type == "int8" ||
           type == "short" || type == "int16" ||
           type == "int" || type == "int32";
}

static bool isFloatType(const java::String& type)
{
    return type == "float" || type == "float32" || type == "double" || type == "float64";
}

static double readBinaryScalarAsDouble(
    java::InputStream& inputStream, const java::String& type, bool bigEndian)
{
    const int bytes = plyTypeSize(type);
    if ( bytes <= 0 ) {
        return 0.0;
    }

    unsigned long long raw = readUnsignedIntegerBytes(inputStream, bytes, bigEndian);
    if ( isFloatType(type) ) {
        if ( bytes == 4 ) {
            unsigned int v = (unsigned int)raw;
            float out = 0.0F;
            memcpy(&out, &v, sizeof(float));
            return (double)out;
        }
        unsigned long long v = raw;
        double out = 0.0;
        memcpy(&out, &v, sizeof(double));
        return out;
    }

    if ( isSignedIntegerType(type) ) {
        return (double)unsignedToSigned(raw, bytes);
    }
    return (double)raw;
}

static int readBinaryScalarAsInt(
    java::InputStream& inputStream, const java::String& type, bool bigEndian)
{
    return (int)readBinaryScalarAsDouble(inputStream, type, bigEndian);
}

static void skipBinaryScalar(java::InputStream& inputStream, const java::String& type)
{
    const int bytes = plyTypeSize(type);
    unsigned char buffer[8];
    if ( bytes > 0 ) {
        PersistenceElement::readBytes(inputStream, buffer, bytes);
    }
}

static void addFaceTriangles(
    const java::ArrayList<int>& indices,
    java::ArrayList<ReaderPlyTriangleIndices>* triangles)
{
    if ( triangles == 0 || indices.size() < 3 ) {
        return;
    }

    for ( long int i = 2; i < indices.size(); i++ ) {
        triangles->add(ReaderPlyTriangleIndices(indices[0], indices[i - 1], indices[i]));
    }
}

static void readVertexAscii(
    const ReaderPlyElement& element,
    const java::String& line,
    java::ArrayList<Vector3Dd>* vertices)
{
    java::ArrayList<java::String> tokens = splitWhitespace(line);
    int tokenIndex = 0;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    for ( long int i = 0; i < element.properties.size() && tokenIndex < tokens.size(); i++ ) {
        const ReaderPlyProperty& property = element.properties[i];
        if ( property.isList ) {
            int count = atoi(tokens[tokenIndex].c_str());
            tokenIndex++;
            tokenIndex += count;
            continue;
        }

        double value = atof(tokens[tokenIndex].c_str());
        tokenIndex++;
        if ( property.name == "x" ) {
            x = value;
        }
        else if ( property.name == "y" ) {
            y = value;
        }
        else if ( property.name == "z" ) {
            z = value;
        }
    }

    vertices->add(Vector3Dd(x, y, z));
}

static void readFaceAscii(
    const ReaderPlyElement& element,
    const java::String& line,
    java::ArrayList<ReaderPlyTriangleIndices>* triangles)
{
    java::ArrayList<java::String> tokens = splitWhitespace(line);
    int tokenIndex = 0;

    for ( long int i = 0; i < element.properties.size() && tokenIndex < tokens.size(); i++ ) {
        const ReaderPlyProperty& property = element.properties[i];
        if ( property.isList ) {
            int count = atoi(tokens[tokenIndex].c_str());
            tokenIndex++;
            java::ArrayList<int> indices;
            for ( int j = 0; j < count && tokenIndex < tokens.size(); j++ ) {
                int index = atoi(tokens[tokenIndex].c_str());
                tokenIndex++;
                if ( property.name == "vertex_indices" || property.name == "vertex_index" ) {
                    indices.add(index);
                }
            }
            if ( property.name == "vertex_indices" || property.name == "vertex_index" ) {
                addFaceTriangles(indices, triangles);
            }
        }
        else {
            tokenIndex++;
        }
    }
}

static void skipAsciiElementRecord(const ReaderPlyElement&, const java::String&)
{
}

static void readVertexBinary(
    java::InputStream& inputStream,
    const ReaderPlyElement& element,
    bool bigEndian,
    java::ArrayList<Vector3Dd>* vertices)
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    for ( long int i = 0; i < element.properties.size(); i++ ) {
        const ReaderPlyProperty& property = element.properties[i];
        if ( property.isList ) {
            int count = readBinaryScalarAsInt(inputStream, property.countType, bigEndian);
            for ( int j = 0; j < count; j++ ) {
                skipBinaryScalar(inputStream, property.valueType);
            }
            continue;
        }

        double value = readBinaryScalarAsDouble(inputStream, property.type, bigEndian);
        if ( property.name == "x" ) {
            x = value;
        }
        else if ( property.name == "y" ) {
            y = value;
        }
        else if ( property.name == "z" ) {
            z = value;
        }
    }

    vertices->add(Vector3Dd(x, y, z));
}

static void readFaceBinary(
    java::InputStream& inputStream,
    const ReaderPlyElement& element,
    bool bigEndian,
    java::ArrayList<ReaderPlyTriangleIndices>* triangles)
{
    for ( long int i = 0; i < element.properties.size(); i++ ) {
        const ReaderPlyProperty& property = element.properties[i];
        if ( property.isList ) {
            int count = readBinaryScalarAsInt(inputStream, property.countType, bigEndian);
            java::ArrayList<int> indices;
            for ( int j = 0; j < count; j++ ) {
                int index = readBinaryScalarAsInt(inputStream, property.valueType, bigEndian);
                if ( property.name == "vertex_indices" || property.name == "vertex_index" ) {
                    indices.add(index);
                }
            }
            if ( property.name == "vertex_indices" || property.name == "vertex_index" ) {
                addFaceTriangles(indices, triangles);
            }
        }
        else {
            skipBinaryScalar(inputStream, property.type);
        }
    }
}

static void skipBinaryElementRecord(
    java::InputStream& inputStream,
    const ReaderPlyElement& element,
    bool bigEndian)
{
    for ( long int i = 0; i < element.properties.size(); i++ ) {
        const ReaderPlyProperty& property = element.properties[i];
        if ( property.isList ) {
            int count = readBinaryScalarAsInt(inputStream, property.countType, bigEndian);
            for ( int j = 0; j < count; j++ ) {
                skipBinaryScalar(inputStream, property.valueType);
            }
        }
        else {
            skipBinaryScalar(inputStream, property.type);
        }
    }
}

static SimpleMaterial* makeDefaultPlyMaterial()
{
    SimpleMaterial* material = new SimpleMaterial();
    *material = material->withAmbient(ColorRgb(0.18, 0.18, 0.18));
    *material = material->withDiffuse(ColorRgb(0.72, 0.76, 0.70));
    *material = material->withSpecular(ColorRgb(0.2, 0.2, 0.2));
    *material = material->withDoubleSided(false);
    return material;
}

class ReaderPlyVertexBuildTask : public java::Callable< java::ArrayList<Vertex>* > {
private:
    const java::ArrayList<Vector3Dd>* vertices;

public:
    explicit ReaderPlyVertexBuildTask(const java::ArrayList<Vector3Dd>* inVertices)
        : vertices(inVertices)
    {
    }

    virtual java::ArrayList<Vertex>* call()
    {
        java::ArrayList<Vertex>* out = new java::ArrayList<Vertex>();
        if ( vertices == 0 ) {
            return out;
        }
        out->reserve(vertices->size());
        for ( long int i = 0; i < vertices->size(); i++ ) {
            Vertex vertex((*vertices)[i]);
            out->add(vertex);
        }
        return out;
    }
};

class ReaderPlyTriangleBuildTask : public java::Callable< java::ArrayList<Triangle>* > {
private:
    const java::ArrayList<ReaderPlyTriangleIndices>* triangles;
    int vertexCount;

public:
    ReaderPlyTriangleBuildTask(
        const java::ArrayList<ReaderPlyTriangleIndices>* inTriangles,
        int inVertexCount)
        : triangles(inTriangles), vertexCount(inVertexCount)
    {
    }

    virtual java::ArrayList<Triangle>* call()
    {
        java::ArrayList<Triangle>* out = new java::ArrayList<Triangle>();
        if ( triangles == 0 ) {
            return out;
        }
        out->reserve(triangles->size());
        for ( long int i = 0; i < triangles->size(); i++ ) {
            const ReaderPlyTriangleIndices& t = (*triangles)[i];
            if ( t.point0 >= 0 && t.point0 < vertexCount &&
                 t.point1 >= 0 && t.point1 < vertexCount &&
                 t.point2 >= 0 && t.point2 < vertexCount ) {
                out->add(Triangle(t.point0, t.point1, t.point2));
            }
        }
        return out;
    }
};

static TriangleMesh* buildTriangleMesh(
    const java::ArrayList<Vector3Dd>& vertices,
    const java::ArrayList<ReaderPlyTriangleIndices>& triangles)
{
    if ( vertices.size() <= 0 || triangles.size() <= 0 ) {
        return 0;
    }

    java::ExecutorService* executorService = java::Executors::newFixedThreadPool(2);
    java::Future< java::ArrayList<Vertex>* > vertexFuture =
        executorService->submit(new ReaderPlyVertexBuildTask(&vertices));
    java::Future< java::ArrayList<Triangle>* > triangleFuture =
        executorService->submit(new ReaderPlyTriangleBuildTask(&triangles, (int)vertices.size()));

    java::ArrayList<Vertex>* meshVertices = vertexFuture.get();
    java::ArrayList<Triangle>* meshTriangles = triangleFuture.get();
    executorService->shutdownNow();
    delete executorService;

    TriangleMesh* mesh = new TriangleMesh();
    mesh->setName("PLY mesh");
    mesh->setVertexes(*meshVertices, false, false, false, false);
    mesh->setTriangles(*meshTriangles);
    mesh->calculateNormals();

    java::ArrayList<SimpleMaterial*> materials;
    materials.add(makeDefaultPlyMaterial());
    mesh->setMaterials(materials);
    mesh->setOwnsMaterials(true);
    java::ArrayList< java::ArrayList<int> > materialRanges;
    java::ArrayList<int> materialRange;
    materialRange.add(0);
    materialRange.add(0);
    materialRanges.add(materialRange);
    mesh->setMaterialRanges(materialRanges);

    delete meshVertices;
    delete meshTriangles;
    return mesh;
}

static void rotateLikeObj(java::ArrayList<Vector3Dd>& vertices)
{
    const double pi = 3.14159265358979323846;
    Matrix4x4d rotation;
    rotation = rotation.axisRotation(90.0 * pi / 180.0, Vector3Dd(1, 0, 0));

    for ( long int i = 0; i < vertices.size(); i++ ) {
        vertices[i] = rotation.multiply(vertices[i]);
    }
}

ReaderPlyResult* ReaderPly::importGeometry(const java::File& sceneFile)
{
    java::FileInputStream inputStream(sceneFile.getPath().c_str());
    ReaderPlyResult* result = importGeometry(inputStream);
    inputStream.close();
    return result;
}

ReaderPlyResult* ReaderPly::importGeometry(java::InputStream& inputStream)
{
    ReaderPlyHeader header;
    if ( !readHeader(inputStream, &header) ) {
        return new ReaderPlyResult();
    }

    java::ArrayList<Vector3Dd> vertices;
    java::ArrayList<ReaderPlyTriangleIndices> triangles;
    const bool binary = header.format != READER_PLY_ASCII;
    const bool bigEndian = header.format == READER_PLY_BINARY_BIG_ENDIAN;

    for ( long int i = 0; i < header.elements.size(); i++ ) {
        const ReaderPlyElement& element = header.elements[i];
        if ( element.name == "vertex" ) {
            vertices.reserve(element.count);
        }

        for ( int j = 0; j < element.count; j++ ) {
            if ( binary ) {
                if ( element.name == "vertex" ) {
                    readVertexBinary(inputStream, element, bigEndian, &vertices);
                }
                else if ( element.name == "face" ) {
                    readFaceBinary(inputStream, element, bigEndian, &triangles);
                }
                else {
                    skipBinaryElementRecord(inputStream, element, bigEndian);
                }
            }
            else {
                java::String line;
                readPlyAsciiLine(inputStream, &line);
                if ( element.name == "vertex" ) {
                    readVertexAscii(element, line, &vertices);
                }
                else if ( element.name == "face" ) {
                    readFaceAscii(element, line, &triangles);
                }
                else {
                    skipAsciiElementRecord(element, line);
                }
            }
        }
    }

    rotateLikeObj(vertices);

    ReaderPlyResult* result = new ReaderPlyResult();
    result->pointCloud = vertices;
    result->triangleMesh = buildTriangleMesh(vertices, triangles);
    return result;
}

static void addMeshToScene(TriangleMesh* mesh, java::ArrayList<SimpleBody*>& bodies)
{
    if ( mesh == 0 ) {
        return;
    }
    SimpleBody* body = new SimpleBody();
    body->setGeometry(mesh);
    body->setPosition(Vector3Dd());
    body->setRotation(Matrix4x4d());
    body->setRotationInverse(Matrix4x4d());
    body->setMaterial(makeDefaultPlyMaterial());
    bodies.add(body);
}

void ReaderPly::importEnvironment(const java::File& sceneFile, SimpleScene* scene)
{
    if ( scene == 0 ) {
        return;
    }

    ReaderPlyResult* result = importGeometry(sceneFile);
    if ( result == 0 ) {
        return;
    }
    addMeshToScene(result->detachTriangleMesh(), scene->getSimpleBodies());
    delete result;
}
