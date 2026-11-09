#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector4Dd.h"
#include "vsdk/toolkit/media/Calligraphic2DBuffer.h"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/geometry/surface/Surface.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMesh.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMeshGroup.h"
#include "vsdk/toolkit/environment/geometry/volume/Solid.h"
#include "vsdk/toolkit/environment/geometry/volume/Volume.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"
#include "vsdk/toolkit/environment/camera/Camera.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/GeometryTriangulator.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/render/hiddenLine/WireframeRenderer.h"
static int calculateCanonicalOutcode(const Vector3Dd& p, double fpd)
{
    int bits = 0x0;
    if ( p.z() + p.x() - 1 > 0 ) bits |= Camera::OPCODE_RIGHT;
    if ( p.z() - p.x() - 1 > 0 ) bits |= Camera::OPCODE_LEFT;
    if ( p.z() + p.y() - 1 > 0 ) bits |= Camera::OPCODE_UP;
    if ( p.z() - p.y() - 1 > 0 ) bits |= Camera::OPCODE_DOWN;
    if ( p.z() > 0 ) bits |= Camera::OPCODE_NEAR;
    if ( p.z() < -fpd ) bits |= Camera::OPCODE_FAR;
    return bits;
}

static bool clipLineCanonicVolume(const Vector3Dd& worldP0,
                                  const Vector3Dd& worldP1,
                                  const Camera* camera,
                                  Vector3Dd& outP0,
                                  Vector3Dd& outP1)
{
    Matrix4x4d nt = camera->getNormalizingTransformation();
    Vector3Dd p0 = nt.multiply(worldP0);
    Vector3Dd p1 = nt.multiply(worldP1);
    const double fpd =
        (camera->getFarPlaneDistance() - camera->getNearPlaneDistance()) /
        camera->getNearPlaneDistance();

    int outcode0 = calculateCanonicalOutcode(p0, fpd);
    int outcode1 = calculateCanonicalOutcode(p1, fpd);

    while ( true ) {
        if ( outcode0 == 0x0 && outcode1 == 0x0 ) {
            outP0 = p0;
            outP1 = p1;
            return true;
        }
        if ( (outcode0 & outcode1) != 0x0 ) {
            return false;
        }

        int outcodeout = (outcode0 != 0) ? outcode0 : outcode1;
        Vector3Dd dir = p1.subtract(p0);
        const double l = dir.length();
        if ( l < VSDK::EPSILON ) return false;
        dir = dir.multiply(1.0 / l);
        const double de = (outcodeout == outcode1) ? -VSDK::EPSILON : VSDK::EPSILON;

        Vector3Dd m;
        if ( (Camera::OPCODE_UP & outcodeout) != 0x0 ) {
            const double t = (l * (1 - p0.z() - p0.y())) /
                             (p1.z() - p0.z() + p1.y() - p0.y());
            m = p0.add(dir.multiply(t + de));
        }
        else if ( (Camera::OPCODE_DOWN & outcodeout) != 0x0 ) {
            const double t = (l * (p0.y() - p0.z() + 1)) /
                             (p1.z() - p0.z() - p1.y() + p0.y());
            m = p0.add(dir.multiply(t + de));
        }
        else if ( (Camera::OPCODE_LEFT & outcodeout) != 0x0 ) {
            const double t = (l * (p0.x() - p0.z() + 1)) /
                             (p1.z() - p0.z() - p1.x() + p0.x());
            m = p0.add(dir.multiply(t + de));
        }
        else if ( (Camera::OPCODE_RIGHT & outcodeout) != 0x0 ) {
            const double t = (l * (1 - p0.z() - p0.x())) /
                             (p1.z() - p0.z() + p1.x() - p0.x());
            m = p0.add(dir.multiply(t + de));
        }
        else if ( (Camera::OPCODE_NEAR & outcodeout) != 0x0 ) {
            const double t = (-p0.z() * l) / (p1.z() - p0.z());
            m = p0.add(dir.multiply(t + de));
        }
        else if ( (Camera::OPCODE_FAR & outcodeout) != 0x0 ) {
            const double t = ((-fpd - p0.z()) * l) / (p1.z() - p0.z());
            m = p0.add(dir.multiply(t + de));
        }

        if ( outcodeout == outcode0 ) {
            p0 = m;
            outcode0 = calculateCanonicalOutcode(p0, fpd);
        }
        else {
            p1 = m;
            outcode1 = calculateCanonicalOutcode(p1, fpd);
        }
    }
}

static void addLine(Calligraphic2DBuffer* lineSet,
                    const Vector3Dd& cp0, const Vector3Dd& cp1,
                    const Matrix4x4d& proj)
{
    Vector4Dd hp0(cp0);
    Vector4Dd hp1(cp1);
    Vector4Dd pp0 = proj.multiply(hp0).dividedByW();
    Vector4Dd pp1 = proj.multiply(hp1).dividedByW();
    lineSet->add2DLine(pp0.x(), pp0.y(), pp1.x(), pp1.y());
}

static void processTriangleMeshGroup(TriangleMeshGroup* mg,
                        SimpleBody* body, const Matrix4x4d& proj,
                        Calligraphic2DBuffer* lineSet, const Camera* camera)
{
    Matrix4x4d M = body->getTransformationMatrix();
    java::ArrayList<TriangleMesh>& meshes = mg->getMeshes();
    for (long int j = 0; j < meshes.size(); j++) {
        TriangleMesh& mesh = meshes[j];
        java::ArrayList<double>& v = mesh.getVertexPositions();
        java::ArrayList<int>& tr = mesh.getTriangleIndexes();
        for (int t = 0; t < mesh.getNumTriangles(); t++) {
            int p0 = tr[3*t];
            int p1 = tr[3*t+1];
            int p2 = tr[3*t+2];

            Vector3Dd a = M.multiply(Vector3Dd(v[3*p0], v[3*p0+1], v[3*p0+2]));
            Vector3Dd b = M.multiply(Vector3Dd(v[3*p1], v[3*p1+1], v[3*p1+2]));
            Vector3Dd c = M.multiply(Vector3Dd(v[3*p2], v[3*p2+1], v[3*p2+2]));

            Vector3Dd cp0, cp1;
            if ( clipLineCanonicVolume(a, b, camera, cp0, cp1) ) addLine(lineSet, cp0, cp1, proj);
            if ( clipLineCanonicVolume(b, c, camera, cp0, cp1) ) addLine(lineSet, cp0, cp1, proj);
            if ( clipLineCanonicVolume(c, a, camera, cp0, cp1) ) addLine(lineSet, cp0, cp1, proj);
        }
    }
}

static void processBrep(SimpleBody* body, const Matrix4x4d& proj,
                        Calligraphic2DBuffer* lineSet, const Camera* camera)
{
    Volume* volume = dynamic_cast<Volume*>(body->getGeometry());
    if ( volume == 0 ) return;

    PolyhedralBoundedSolid* brep = volume->exportToPolyhedralBoundedSolid();
    if ( brep == 0 ) return;

    Matrix4x4d M = body->getTransformationMatrix();
    for (size_t i = 0; i < brep->getEdgesList().size(); i++) {
        _PolyhedralBoundedSolidEdge* e = brep->getEdgesList()[i];
        if ( e == 0 || e->rightHalf == 0 || e->leftHalf == 0 ||
             e->rightHalf->startingVertex == 0 || e->leftHalf->startingVertex == 0 ) {
            continue;
        }
        Vector3Dd a = M.multiply(e->rightHalf->startingVertex->position);
        Vector3Dd b = M.multiply(e->leftHalf->startingVertex->position);
        Vector3Dd cp0, cp1;
        if ( clipLineCanonicVolume(a, b, camera, cp0, cp1) ) {
            addLine(lineSet, cp0, cp1, proj);
        }
    }
}

void WireframeRenderer::execute(Calligraphic2DBuffer* outLineSet,
                                java::ArrayList<SimpleBody*>& simpleBodies,
                                const Camera* camera)
{
    if ( outLineSet == 0 || camera == 0 ) return;

    Matrix4x4d proj;
    proj = proj.canonicalPerspectiveProjection();

    for (long int i = 0; i < simpleBodies.size(); i++) {
        SimpleBody* body = simpleBodies[i];
        if ( body == 0 || body->getGeometry() == 0 ) continue;
        Geometry* g = body->getGeometry();
        if ( dynamic_cast<Solid*>(g) != 0 ) {
            processBrep(body, proj, outLineSet, camera);
        }
        else {
            TriangleMeshGroup mg;
            if ( GeometryTriangulator::exportToTriangleMeshGroup(g, mg) ) {
                processTriangleMeshGroup(&mg, body, proj, outLineSet, camera);
            }
        }
    }
}
