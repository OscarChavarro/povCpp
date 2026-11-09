#include <cmath>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector4Dd.h"
#include "vsdk/toolkit/media/Calligraphic2DBuffer.h"
#include "vsdk/toolkit/environment/geometry/element/Intersection.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/Triangle.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/environment/geometry/volume/Volume.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"
#include "vsdk/toolkit/environment/camera/Camera.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/render/hiddenLine/HiddenLineRenderer.h"
namespace {

class AppelEdgeSegment {
public:
    double lineParameter;
    int deltaQI;

    AppelEdgeSegment() : lineParameter(0.0), deltaQI(0) {}

    bool samePosition(const AppelEdgeSegment& other) const
    {
        return std::abs(lineParameter - other.lineParameter) <= VSDK::EPSILON;
    }
};

class AppelEdgeCache {
public:
    static const int HIDDEN_LINE = 0;
    static const int VISIBLE_LINE = 1;
    static const int CONTOUR_LINE = 2;

    int edgeType;
    bool onSequence;
    Vector3Dd start;
    Vector3Dd end;
    Vector3Dd direction;
    SimpleBody* ownerBody;
    PolyhedralBoundedSolid* ownerSolid;
    _PolyhedralBoundedSolidFace* visibleEdgeForContourLine;
    SimpleBody* visibleEdgeBody;
    _PolyhedralBoundedSolidFace* leftFace;
    _PolyhedralBoundedSolidFace* rightFace;
    int edgeIndex;

    AppelEdgeCache()
        : edgeType(VISIBLE_LINE),
          onSequence(false),
          start(),
          end(),
          direction(),
          ownerBody(0),
          ownerSolid(0),
          visibleEdgeForContourLine(0),
          visibleEdgeBody(0),
          leftFace(0),
          rightFace(0),
          edgeIndex(-1)
    {
    }

    void setStart(const Vector3Dd& s) { start = Vector3Dd(s); }
    void setEnd(const Vector3Dd& e) { end = Vector3Dd(e); }
};

const double CLIP_PLANES[][4] = {
    { 1.0, 0.0, 0.0, 1.0 },
    { -1.0, 0.0, 0.0, 1.0 },
    { 0.0, 1.0, 0.0, 1.0 },
    { 0.0, -1.0, 0.0, 1.0 },
    { 0.0, 0.0, 1.0, 1.0 },
    { 0.0, 0.0, -1.0, 1.0 }
};

double evaluateClipPlane(const double plane[4], const Vector4Dd& point)
{
    return plane[0] * point.x() + plane[1] * point.y() +
        plane[2] * point.z() + plane[3] * point.w();
}

Vector4Dd interpolate(const Vector4Dd& start, const Vector4Dd& end, double t)
{
    return start.multiply(1.0 - t).add(end.multiply(t));
}

bool clipLineToClipVolume(
    const Vector4Dd& start,
    const Vector4Dd& end,
    Vector4Dd& outStart,
    Vector4Dd& outEnd)
{
    outStart = start;
    outEnd = end;

    for (int i = 0; i < 6; ++i) {
        double d0 = evaluateClipPlane(CLIP_PLANES[i], outStart);
        double d1 = evaluateClipPlane(CLIP_PLANES[i], outEnd);

        if (d0 < 0.0 && d1 < 0.0) {
            return false;
        }
        if (d0 < 0.0 || d1 < 0.0) {
            double denominator = d0 - d1;
            if (std::abs(denominator) < VSDK::EPSILON) {
                return false;
            }
            double t = d0 / denominator;
            Vector4Dd intersection = interpolate(outStart, outEnd, t);
            if (d0 < 0.0) {
                outStart = intersection;
            }
            else {
                outEnd = intersection;
            }
        }
    }
    return true;
}

void addProjectedLine(
    Calligraphic2DBuffer* lineSet,
    const Vector3Dd& point0,
    const Vector3Dd& point1,
    const Camera* camera)
{
    if (lineSet == 0 || camera == 0) {
        return;
    }

    Matrix4x4d projection = camera->calculateProjectionMatrix();
    Vector4Dd clip0 = projection.multiply(Vector4Dd(point0));
    Vector4Dd clip1 = projection.multiply(Vector4Dd(point1));
    Vector4Dd clipped0(0.0, 0.0, 0.0, 1.0);
    Vector4Dd clipped1(0.0, 0.0, 0.0, 1.0);
    if (!clipLineToClipVolume(clip0, clip1, clipped0, clipped1)) {
        return;
    }

    Vector4Dd ndc0 = clipped0.dividedByW();
    Vector4Dd ndc1 = clipped1.dividedByW();
    lineSet->add2DLine(ndc0.x(), ndc0.y(), ndc1.x(), ndc1.y());
}

Vector3Dd transformToWorld(SimpleBody* body, const Vector3Dd& localPoint)
{
    if (body == 0) {
        return localPoint;
    }
    return body->getTransformationMatrix().multiply(localPoint);
}

Vector3Dd transformToLocal(SimpleBody* body, const Vector3Dd& worldPoint)
{
    if (body == 0) {
        return worldPoint;
    }

    Vector3Dd translatedPoint = worldPoint.subtract(body->getPosition());
    Vector3Dd rotatedPoint = body->getRotationInverse().multiply(translatedPoint);
    Vector3Dd scale = body->getScale();

    double x = std::abs(scale.x()) > VSDK::EPSILON ? rotatedPoint.x() / scale.x() : 0.0;
    double y = std::abs(scale.y()) > VSDK::EPSILON ? rotatedPoint.y() / scale.y() : 0.0;
    double z = std::abs(scale.z()) > VSDK::EPSILON ? rotatedPoint.z() / scale.z() : 0.0;
    return Vector3Dd(x, y, z);
}

InfinitePlane* getWorldContainingPlane(
    _PolyhedralBoundedSolidFace* face,
    SimpleBody* body)
{
    if (face == 0) {
        return 0;
    }

    InfinitePlane* localPlane = face->getContainingPlane();
    if (localPlane == 0) {
        return 0;
    }

    Vector3Dd pointOnFace;
    bool foundPoint = false;
    for (long int i = 0; i < face->boundariesList.size() && !foundPoint; ++i) {
        _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(i);
        _PolyhedralBoundedSolidHalfEdge* he =
            loop != 0 ? loop->boundaryStartHalfEdge : 0;
        if (he != 0 && he->startingVertex != 0) {
            pointOnFace = he->startingVertex->position;
            foundPoint = true;
        }
    }

    if (!foundPoint) {
        delete localPlane;
        return 0;
    }

    Vector3Dd worldPoint = transformToWorld(body, pointOnFace);
    Vector3Dd localNormal = localPlane->getNormal();
    delete localPlane;

    Vector3Dd worldNormal;
    if (body == 0) {
        worldNormal = localNormal;
    }
    else {
        Vector3Dd scale = body->getScale();
        Vector3Dd scaledNormal(
            std::abs(scale.x()) > VSDK::EPSILON ? localNormal.x() / scale.x() : 0.0,
            std::abs(scale.y()) > VSDK::EPSILON ? localNormal.y() / scale.y() : 0.0,
            std::abs(scale.z()) > VSDK::EPSILON ? localNormal.z() / scale.z() : 0.0);
        worldNormal = body->getRotation().withoutTranslation().transformDirection(
            scaledNormal).normalized();
    }

    if (worldNormal.length() <= VSDK::EPSILON) {
        return 0;
    }

    return new InfinitePlane(worldNormal.normalized(), worldPoint);
}

int isFaceVisibleFromCameraTransformed(
    _PolyhedralBoundedSolidFace* face,
    SimpleBody* body,
    const Camera* camera)
{
    if (face == 0 || camera == 0) {
        return 0;
    }

    Vector3Dd iv(1, 0, 0);
    Vector3Dd viewingVector = camera->getRotation().multiply(iv);
    InfinitePlane* plane = getWorldContainingPlane(face, body);
    if (plane == 0) {
        return 0;
    }
    Vector3Dd n = plane->getNormal().normalized();
    delete plane;

    if (camera->getProjectionMode() == Camera::PROJECTION_MODE_ORTHOGONAL) {
        viewingVector = viewingVector.normalized();
        double dot = n.dotProduct(viewingVector);
        if (dot > VSDK::EPSILON) {
            return -1;
        }
        else if (dot < -VSDK::EPSILON) {
            return 1;
        }
        return 0;
    }

    Vector3Dd cameraPosition = camera->getPosition();
    for (long int i = 0; i < face->boundariesList.size(); ++i) {
        _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(i);
        if (loop == 0 || loop->boundaryStartHalfEdge == 0) {
            continue;
        }
        _PolyhedralBoundedSolidHalfEdge* he = loop->boundaryStartHalfEdge;
        _PolyhedralBoundedSolidHalfEdge* heStart = he;
        do {
            he = he->next();
            if (he == 0 || he->startingVertex == 0) {
                break;
            }
            Vector3Dd p = transformToWorld(body, he->startingVertex->position);
            Vector3Dd t = p.subtract(cameraPosition).multiply(-1).normalized();
            if (t.dotProduct(n) > 0.0) {
                return 1;
            }
        } while (he != heStart);
    }
    return -1;
}

void buildCache(
    java::ArrayList<HiddenLineQuerySolid>& solids,
    SimpleBody* body,
    PolyhedralBoundedSolid* solid,
    java::ArrayList<AppelEdgeCache*>& cache,
    java::ArrayList<AppelEdgeCache*>& contourCache,
    const Camera* camera,
    bool ownsSolid)
{
    if (body == 0 || solid == 0) {
        return;
    }

    HiddenLineQuerySolid querySolid;
    querySolid.body = body;
    querySolid.solid = solid;
    querySolid.ownsSolid = ownsSolid;
    solids.add(querySolid);

    Vector3Dd prevEnd;
    bool hasPrevEnd = false;
    java::ArrayList<_PolyhedralBoundedSolidEdge*>& edges = solid->getEdgesList();

    for (long int i = 0; i < edges.size(); ++i) {
        _PolyhedralBoundedSolidEdge* edge = edges.get(i);
        if (edge == 0 || edge->leftHalf == 0 || edge->rightHalf == 0 ||
            edge->leftHalf->startingVertex == 0 ||
            edge->rightHalf->startingVertex == 0) {
            continue;
        }

        _PolyhedralBoundedSolidFace* face1 = edge->leftHalf->parentLoop != 0 ?
            edge->leftHalf->parentLoop->parentFace : 0;
        _PolyhedralBoundedSolidFace* face2 = edge->rightHalf->parentLoop != 0 ?
            edge->rightHalf->parentLoop->parentFace : 0;
        if (face1 == 0 || face2 == 0) {
            continue;
        }

        Vector3Dd startPosition =
            transformToWorld(body, edge->leftHalf->startingVertex->position);
        Vector3Dd endPosition =
            transformToWorld(body, edge->rightHalf->startingVertex->position);
        bool f1 = isFaceVisibleFromCameraTransformed(face1, body, camera) >= 0;
        bool f2 = isFaceVisibleFromCameraTransformed(face2, body, camera) >= 0;

        // On the heap, as the references of the Java version: contourCache
        // keeps pointers to them while cache grows.
        cache.add(new AppelEdgeCache());
        AppelEdgeCache& materialLine = *cache[cache.size() - 1];
        materialLine.setStart(startPosition);
        materialLine.setEnd(endPosition);
        materialLine.direction = endPosition.subtract(startPosition);
        materialLine.ownerBody = body;
        materialLine.ownerSolid = solid;
        materialLine.leftFace = face1;
        materialLine.rightFace = face2;
        materialLine.edgeIndex = static_cast<int>(i);
        materialLine.onSequence = hasPrevEnd &&
            prevEnd.subtract(startPosition).length() < VSDK::EPSILON;

        if (!f1 && !f2) {
            materialLine.edgeType = AppelEdgeCache::HIDDEN_LINE;
        }
        else if ((f1 && !f2) || (!f1 && f2)) {
            materialLine.edgeType = AppelEdgeCache::CONTOUR_LINE;
            materialLine.visibleEdgeForContourLine = f1 ? face1 : face2;
            materialLine.visibleEdgeBody = body;
            contourCache.add(&materialLine);
        }
        else {
            materialLine.edgeType = AppelEdgeCache::VISIBLE_LINE;
        }

        prevEnd = Vector3Dd::copyOf(endPosition);
        hasPrevEnd = true;
    }
}

int computeMidpointQuantitativeInvisibility(
    java::ArrayList<HiddenLineQuerySolid>& solids,
    const Camera* camera,
    const Vector3Dd& midpoint)
{
    int qi = 0;
    for (long int i = 0; i < solids.size(); ++i) {
        HiddenLineQuerySolid& querySolid = solids[i];
        if (querySolid.body == 0 || querySolid.solid == 0) {
            continue;
        }
        Vector3Dd localEye =
            transformToLocal(querySolid.body, camera->getPosition());
        Vector3Dd localPoint =
            transformToLocal(querySolid.body, midpoint);
        qi += querySolid.solid->computeQuantitativeInvisibility(
            localEye, localPoint);
    }
    return qi;
}

bool isUnitInterval(double t)
{
    return t >= VSDK::EPSILON && t <= 1.0 - VSDK::EPSILON;
}

void processLineToBeDrawn(
    java::ArrayList<HiddenLineQuerySolid>& solids,
    AppelEdgeCache& inEdge,
    const Camera* inCamera,
    Calligraphic2DBuffer* outVisibleContourLineSet,
    Calligraphic2DBuffer* outVisibleNonContourLineSet,
    Calligraphic2DBuffer* outHiddenLineSet,
    java::ArrayList<AppelEdgeCache*>& contourCache)
{
    Vector3Dd sp1a = inEdge.start;
    Vector3Dd sp1b = inEdge.end;
    Vector3Dd sp1c = inCamera->getPosition();
    Vector3Dd sp2c = inCamera->getPosition();

    java::ArrayList<AppelEdgeSegment> segments;
    segments.add(AppelEdgeSegment());

    for (long int i = 0; i < contourCache.size(); ++i) {
        AppelEdgeCache* cl = contourCache[i];
        if (cl == 0 || cl == &inEdge) {
            continue;
        }

        Ray ray(
            cl->start.add(cl->direction.multiply(3 * VSDK::EPSILON)),
            cl->direction);
        double t0 = ray.getDirection().length() - 6 * VSDK::EPSILON;
        ray = ray.withDirection(ray.getDirection().normalized());
        Intersection* hit = Triangle::doIntersectionWithTriangle(
            ray, sp1a, sp1b, sp1c);
        if (hit != 0 && hit->t < t0) {
            InfinitePlane plane(cl->start, cl->end, sp2c);
            Ray edgeRay(inEdge.start, inEdge.direction.normalized());
            Ray* planeHit = plane.doIntersectionFirstHit(edgeRay);
            if (planeHit != 0) {
                AppelEdgeSegment segment;
                segment.lineParameter = planeHit->getT() / inEdge.direction.length();
                if (isUnitInterval(segment.lineParameter)) {
                    segments.add(segment);
                }
                delete planeHit;
            }
        }
        delete hit;
    }

    AppelEdgeSegment endSegment;
    endSegment.lineParameter = 1.0;
    segments.add(endSegment);

    segments.sort([](const AppelEdgeSegment& a, const AppelEdgeSegment& b) {
            if (a.lineParameter < b.lineParameter - VSDK::EPSILON) return true;
            if (a.lineParameter > b.lineParameter + VSDK::EPSILON) return false;
            return false;
        });

    for (size_t i = 0; i + 1 < segments.size(); ) {
        if (segments[i].samePosition(segments[i + 1])) {
            segments[i + 1].deltaQI += segments[i].deltaQI;
            segments.remove(static_cast<long>(i));
        }
        else {
            ++i;
        }
    }

    for (size_t i = 0; i + 1 < segments.size(); ++i) {
        double t1 = segments[i].lineParameter;
        double t2 = segments[i + 1].lineParameter;
        Vector3Dd pos1 = inEdge.start.add(inEdge.direction.multiply(t1));
        Vector3Dd pos2 = inEdge.start.add(inEdge.direction.multiply(t2));
        Vector3Dd midpoint = inEdge.start.add(
            inEdge.direction.multiply((t1 + t2) * 0.5));

        int midpointQi =
            computeMidpointQuantitativeInvisibility(solids, inCamera, midpoint);

        if (midpointQi == 0) {
            if (inEdge.edgeType == AppelEdgeCache::CONTOUR_LINE) {
                addProjectedLine(outVisibleContourLineSet, pos1, pos2, inCamera);
            }
            else {
                addProjectedLine(outVisibleNonContourLineSet, pos1, pos2, inCamera);
            }
        }
        else {
            addProjectedLine(outHiddenLineSet, pos1, pos2, inCamera);
        }
    }
}

} // namespace

int HiddenLineRenderer::isFaceVisibleFromCamera(
    _PolyhedralBoundedSolidFace* face,
    const Camera* camera)
{
    if (face == 0 || camera == 0) {
        return 0;
    }

    Vector3Dd iv(1, 0, 0);
    Vector3Dd viewingVector = camera->getRotation().multiply(iv);
    InfinitePlane* plane = face->getContainingPlane();
    if (plane == 0) {
        return 0;
    }
    Vector3Dd n = plane->getNormal().normalized();
    delete plane;

    if (camera->getProjectionMode() == Camera::PROJECTION_MODE_ORTHOGONAL) {
        viewingVector = viewingVector.normalized();
        double dot = n.dotProduct(viewingVector);
        if (dot > VSDK::EPSILON) {
            return -1;
        }
        else if (dot < -VSDK::EPSILON) {
            return 1;
        }
        return 0;
    }

    Vector3Dd cameraPosition = camera->getPosition();
    for (long int i = 0; i < face->boundariesList.size(); ++i) {
        _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(i);
        if (loop == 0 || loop->boundaryStartHalfEdge == 0) {
            continue;
        }
        _PolyhedralBoundedSolidHalfEdge* he = loop->boundaryStartHalfEdge;
        _PolyhedralBoundedSolidHalfEdge* heStart = he;
        do {
            he = he->next();
            if (he == 0 || he->startingVertex == 0) {
                break;
            }
            Vector3Dd p = he->startingVertex->position;
            Vector3Dd t = p.subtract(cameraPosition).multiply(-1).normalized();
            if (t.dotProduct(n) > 0.0) {
                return 1;
            }
        } while (he != heStart);
    }
    return -1;
}

void HiddenLineRenderer::executeAppelAlgorithm(
    java::ArrayList<SimpleBody*>& inSimpleBodyArray,
    const Camera* inCamera,
    Calligraphic2DBuffer* outVisibleContourLineSet,
    Calligraphic2DBuffer* outVisibleNonContourLineSet,
    Calligraphic2DBuffer* outHiddenLineSet)
{
    if (inCamera == 0 || outVisibleContourLineSet == 0 ||
        outVisibleNonContourLineSet == 0 || outHiddenLineSet == 0) {
        return;
    }

    java::ArrayList<HiddenLineQuerySolid> querySolids;
    java::ArrayList<AppelEdgeCache*> cache;
    java::ArrayList<AppelEdgeCache*> contourCache;

    for (long int i = 0; i < inSimpleBodyArray.size(); ++i) {
        SimpleBody* body = inSimpleBodyArray.get(i);
        if (body == 0 || body->getGeometry() == 0) {
            continue;
        }
        Volume* volume = dynamic_cast<Volume*>(body->getGeometry());
        if (volume == 0) {
            continue;
        }

        PolyhedralBoundedSolid* solid = volume->exportToPolyhedralBoundedSolid();
        if (solid == 0) {
            continue;
        }

        bool ownsSolid = static_cast<Geometry*>(solid) != body->getGeometry();
        buildCache(
            querySolids,
            body,
            solid,
            cache,
            contourCache,
            inCamera,
            ownsSolid);
    }

    for (long int i = 0; i < querySolids.size(); ++i) {
        if (querySolids[i].solid != 0) {
            querySolids[i].solid->beginVisibilityQueries();
        }
    }

    for (long int i = 0; i < cache.size(); ++i) {
        AppelEdgeCache& edge = *cache[i];
        switch (edge.edgeType) {
          case AppelEdgeCache::HIDDEN_LINE:
          case AppelEdgeCache::CONTOUR_LINE:
          case AppelEdgeCache::VISIBLE_LINE:
            processLineToBeDrawn(querySolids, edge, inCamera,
                outVisibleContourLineSet, outVisibleNonContourLineSet,
                outHiddenLineSet, contourCache);
            break;
          default:
            break;
        }
    }

    for (long int i = 0; i < querySolids.size(); ++i) {
        if (querySolids[i].solid != 0) {
            querySolids[i].solid->endVisibilityQueries();
        }
        if (querySolids[i].ownsSolid) {
            delete querySolids[i].solid;
            querySolids[i].solid = 0;
        }
    }
    for (long int i = 0; i < cache.size(); ++i) {
        delete cache[i];
    }
}
