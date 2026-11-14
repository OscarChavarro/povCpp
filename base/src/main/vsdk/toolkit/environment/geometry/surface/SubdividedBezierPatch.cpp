#include "java/lang/Double.h"
#include "java/lang/IllegalArgumentException.h"
#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayBoundary.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossing.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/surface/SubdividedBezierPatch.h"

bool SubdividedBezierPatch::HitList::add(double depth, const Vector3Dd& point, const Vector3Dd& normal)
{
    if (used + count >= maximum) return false;
    depths[count] = depth;
    points[count] = point;
    normals[count] = normal;
    count++;
    return true;
}

SubdividedBezierPatch::SubdividedBezierPatch(BezierPatchSubdivision subdivisionValue, int uStepsValue,
    int vStepsValue, double flatnessValue, const Vector3Dd (&net)[4][4]) :
    subdivision(subdivisionValue), uSteps(uStepsValue), vSteps(vStepsValue), flatness(flatnessValue),
    boundingCenter(), boundingRadiusSquared(0), gridPoints(), gridPlaneNormals(), gridPlaneOffsets(),
    gridSmoothNormals(), nodes()
{
    const bool adaptive = subdivision == BezierPatchSubdivision::Adaptive ||
        subdivision == BezierPatchSubdivision::AdaptiveTree;
    if (uSteps < (adaptive ? 0 : 1) || vSteps < (adaptive ? 0 : 1)) {
        throw java::IllegalArgumentException("SubdividedBezierPatch: invalid subdivision steps");
    }
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            const Vector3Dd& p = net[i][j];
            if (!java::Double::isFinite(p.x()) || !java::Double::isFinite(p.y()) || !java::Double::isFinite(p.z())) {
                throw java::IllegalArgumentException("SubdividedBezierPatch: control points must be finite");
            }
            controlPoints[i][j] = p;
        }
    }
    precompute();
}

SubdividedBezierPatch::SubdividedBezierPatch(const SubdividedBezierPatch& other) :
    Surface(other), subdivision(other.subdivision), uSteps(other.uSteps), vSteps(other.vSteps),
    flatness(other.flatness), boundingCenter(other.boundingCenter),
    boundingRadiusSquared(other.boundingRadiusSquared), gridPoints(other.gridPoints),
    gridPlaneNormals(other.gridPlaneNormals), gridPlaneOffsets(other.gridPlaneOffsets),
    gridSmoothNormals(other.gridSmoothNormals), nodes(other.nodes)
{
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) controlPoints[i][j] = other.controlPoints[i][j];
    }
}

SubdividedBezierPatch& SubdividedBezierPatch::operator=(const SubdividedBezierPatch& other)
{
    if (this != &other) {
        Surface::operator=(other);
        subdivision = other.subdivision;
        uSteps = other.uSteps;
        vSteps = other.vSteps;
        flatness = other.flatness;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) controlPoints[i][j] = other.controlPoints[i][j];
        }
        boundingCenter = other.boundingCenter;
        boundingRadiusSquared = other.boundingRadiusSquared;
        gridPoints = other.gridPoints;
        gridPlaneNormals = other.gridPlaneNormals;
        gridPlaneOffsets = other.gridPlaneOffsets;
        gridSmoothNormals = other.gridSmoothNormals;
        nodes = other.nodes;
    }
    return *this;
}

SubdividedBezierPatch::~SubdividedBezierPatch()
{
}

Geometry* SubdividedBezierPatch::copyGeometry() const { return new SubdividedBezierPatch(*this); }

Vector3Dd SubdividedBezierPatch::evaluate(double u, double v) const
{
    const double u2 = u * u;
    const double u3 = u * u2;
    const double v2 = v * v;
    const double v3 = v * v2;
    const double uu1 = 1.0 - u;
    const double uu2 = uu1 * uu1;
    const double uu3 = uu1 * uu2;
    const double vv1 = 1.0 - v;
    const double vv2 = vv1 * vv1;
    const double vv3 = vv1 * vv2;
    double t[4][4];
    t[0][0] = uu3 * vv3;
    t[0][1] = 3.0 * uu3 * v * vv2;
    t[0][2] = 3.0 * uu3 * v2 * vv1;
    t[0][3] = uu3 * v3;
    t[1][0] = 3.0 * u * uu2 * vv3;
    t[1][1] = 9.0 * u * uu2 * v * vv2;
    t[1][2] = 9.0 * u * uu2 * v2 * vv1;
    t[1][3] = 3.0 * u * uu2 * v3;
    t[2][0] = 3.0 * u2 * uu1 * vv3;
    t[2][1] = 9.0 * u2 * uu1 * v * vv2;
    t[2][2] = 9.0 * u2 * uu1 * v2 * vv1;
    t[2][3] = 3.0 * u2 * uu1 * v3;
    t[3][0] = u3 * vv3;
    t[3][1] = 3.0 * u3 * v * vv2;
    t[3][2] = 3.0 * u3 * v2 * vv1;
    t[3][3] = u3 * v3;
    double rx = 0.0;
    double ry = 0.0;
    double rz = 0.0;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            rx += t[i][j] * controlPoints[i][j].x();
            ry += t[i][j] * controlPoints[i][j].y();
            rz += t[i][j] * controlPoints[i][j].z();
        }
    }
    return Vector3Dd(rx, ry, rz);
}

Vector3Dd SubdividedBezierPatch::evaluateNormal(double u, double v) const
{
    const double u2 = u * u;
    const double u3 = u * u2;
    const double v2 = v * v;
    const double v3 = v * v2;
    double t[4][4];
    t[0][0] = 3.0 * (v3 - 3.0 * v2 + 3.0 * v - 1.0) * (u2 - 2.0 * u + 1.0);
    t[0][1] = 9.0 * v * (v2 - 2.0 * v + 1.0) * (u2 - 2.0 * u + 1.0);
    t[0][2] = 9.0 * v2 * (v - 1.0) * (u2 - 2.0 * u + 1.0);
    t[0][3] = 3.0 * v3 * (u2 - 2.0 * u + 1.0);
    t[1][0] = 3.0 * (v3 - 3.0 * v2 + 3.0 * v - 1) * (3.0 * u2 - 4.0 * u + 1);
    t[1][1] = 9.0 * v * (v2 - 2.0 * v + 1.0) * (3.0 * u2 - 4.0 * u + 1.0);
    t[1][2] = 9.0 * v2 * (v - 1.0) * (3.0 * u2 - 4.0 * u + 1.0);
    t[1][3] = 3.0 * v3 * (3.0 * u2 - 4.0 * u + 1.0);
    t[2][0] = 3.0 * u * (v3 - 3.0 * v2 + 3.0 * v - 1.0) * (3.0 * u - 2.0);
    t[2][1] = 9.0 * u * v * (v2 - 2.0 * v + 1.0) * (3.0 * u - 2.0);
    t[2][2] = 9.0 * u * v2 * (v - 1.0) * (3.0 * u - 2.0);
    t[2][3] = 3.0 * u * v3 * (3.0 * u - 2.0);
    t[3][0] = 3.0 * u2 * (v3 - 3.0 * v2 + 3.0 * v - 1.0);
    t[3][1] = 9.0 * u2 * v * (v2 - 2.0 * v + 1.0);
    t[3][2] = 9.0 * u2 * v2 * (v - 1.0);
    t[3][3] = 3.0 * u2 * v3;
    double ux = 0.0;
    double uy = 0.0;
    double uz = 0.0;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            ux += t[i][j] * controlPoints[i][j].x();
            uy += t[i][j] * controlPoints[i][j].y();
            uz += t[i][j] * controlPoints[i][j].z();
        }
    }
    double temp = ux * ux + uy * uy + uz * uz;
    if (temp < DEGENERACY_EPSILON) {
        return Vector3Dd(1.0, 0.0, 0.0);
    }
    temp = java::Math::sqrt(temp);
    const Vector3Dd uVec(ux / temp, uy / temp, uz / temp);

    t[0][0] = 3.0 * (v2 - 2.0 * v + 1.0) * (u3 - 3.0 * u2 + 3.0 * u - 1.0);
    t[0][1] = 3.0 * (3.0 * v2 - 4.0 * v + 1.0) * (u3 - 3.0 * u2 + 3.0 * u - 1.0);
    t[0][2] = 3.0 * v * (3.0 * v - 2.0) * (u3 - 3.0 * u2 + 3.0 * u - 1.0);
    t[0][3] = 3.0 * v2 * (u3 - 3.0 * u2 + 3.0 * u - 1.0);
    t[1][0] = 9.0 * u * (v2 - 2.0 * v + 1.0) * (u2 - 2.0 * u + 1.0);
    t[1][1] = 9.0 * u * (3.0 * v2 - 4.0 * v + 1.0) * (u2 - 2.0 * u + 1.0);
    t[1][2] = 9.0 * u * v * (3.0 * v - 2.0) * (u2 - 2.0 * u + 1.0);
    t[1][3] = 9.0 * u * v2 * (u2 - 2.0 * u + 1.0);
    t[2][0] = 9.0 * u2 * (v2 - 2.0 * v + 1.0) * (u - 1.0);
    t[2][1] = 9.0 * u2 * (3.0 * v2 - 4.0 * v + 1.0) * (u - 1.0);
    t[2][2] = 9.0 * u2 * v * (3.0 * v - 2.0) * (u - 1.0);
    t[2][3] = 9.0 * u2 * v2 * (u - 1.0);
    t[3][0] = 3.0 * u3 * (v2 - 2.0 * v + 1.0);
    t[3][1] = 3.0 * u3 * (3.0 * v2 - 4.0 * v + 1.0);
    t[3][2] = 3.0 * u3 * v * (3.0 * v - 2.0);
    t[3][3] = 3.0 * u3 * v2;
    double vx = 0.0;
    double vy = 0.0;
    double vz = 0.0;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            vx += t[i][j] * controlPoints[i][j].x();
            vy += t[i][j] * controlPoints[i][j].y();
            vz += t[i][j] * controlPoints[i][j].z();
        }
    }
    temp = vx * vx + vy * vy + vz * vz;
    if (temp < DEGENERACY_EPSILON) {
        return Vector3Dd(1.0, 0.0, 0.0);
    }
    temp = java::Math::sqrt(temp);
    const Vector3Dd vVec(vx / temp, vy / temp, vz / temp);
    return uVec.crossProduct(vVec);
}

void SubdividedBezierPatch::sphereAround(int count, const Vector3Dd* points, Vector3Dd& center,
    double& radiusSquared)
{
    double xc = 0;
    double yc = 0;
    double zc = 0;
    for (int i = 0; i < count; i++) {
        xc += points[i].x();
        yc += points[i].y();
        zc += points[i].z();
    }
    xc /= (double)count;
    yc /= (double)count;
    zc /= (double)count;
    double r0 = 0.0;
    for (int i = 0; i < count; i++) {
        const double x0 = points[i].x() - xc;
        const double y0 = points[i].y() - yc;
        const double z0 = points[i].z() - zc;
        const double r1 = x0 * x0 + y0 * y0 + z0 * z0;
        if (r1 > r0) {
            r0 = r1;
        }
    }
    center = Vector3Dd(xc, yc, zc);
    radiusSquared = r0;
}

void SubdividedBezierPatch::netSphere(const Vector3Dd (&net)[4][4], Vector3Dd& center, double& radiusSquared)
{
    double xc = 0;
    double yc = 0;
    double zc = 0;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            xc += net[i][j].x();
            yc += net[i][j].y();
            zc += net[i][j].z();
        }
    }
    xc /= 16.0;
    yc /= 16.0;
    zc /= 16.0;
    double r0 = 0.0;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            const double x0 = net[i][j].x() - xc;
            const double y0 = net[i][j].y() - yc;
            const double z0 = net[i][j].z() - zc;
            const double r1 = x0 * x0 + y0 * y0 + z0 * z0;
            if (r1 > r0) {
                r0 = r1;
            }
        }
    }
    center = Vector3Dd(xc, yc, zc);
    radiusSquared = r0;
}

// Unit normal and offset d of the plane n.p + d = 0 through a triangle;
// false, with normal (1, 0, 0), for a degenerate triangle.
bool SubdividedBezierPatch::trianglePlane(const Vector3Dd& v1, const Vector3Dd& v2, const Vector3Dd& v3,
    Vector3Dd& normal, double& offset)
{
    const Vector3Dd edge1 = v1.subtract(v2);
    const Vector3Dd edge2 = v3.subtract(v2);
    normal = edge1.crossProduct(edge2);
    const double length = normal.length();
    if (length < DEGENERACY_EPSILON) {
        normal = Vector3Dd(1.0, 0.0, 0.0);
        offset = -1.0 * v1.x();
        return false;
    }
    normal = Vector3Dd(normal.x() / length, normal.y() / length, normal.z() / length);
    offset = normal.dotProduct(v1);
    offset = 0.0 - offset;
    return true;
}

double SubdividedBezierPatch::planeDistance(const Vector3Dd& p, const Vector3Dd& n, double d)
{
    double temp1 = p.dotProduct(n);
    temp1 += d;
    const double temp2 = n.length();
    if (java::Math::abs(temp2) < DEGENERACY_EPSILON) {
        return 0;
    }
    return temp1 / temp2;
}

bool SubdividedBezierPatch::rayMeetsSphere(const Vector3Dd& origin, const Vector3Dd& direction,
    const Vector3Dd& center, double radiusSquared)
{
    const double x = center.x() - origin.x();
    const double y = center.y() - origin.y();
    const double z = center.z() - origin.z();
    const double dist1 = x * x + y * y + z * z;
    if (dist1 < radiusSquared) {
        return true;
    }
    const double projection = x * direction.x() + y * direction.y() + z * direction.z();
    if (projection <= 0.0) {
        return false;
    }
    const double directionLengthSquared = direction.dotProduct(direction);
    if (directionLengthSquared <= DEGENERACY_EPSILON) {
        return false;
    }
    return dist1 - (projection * projection) / directionLengthSquared < radiusSquared;
}

void SubdividedBezierPatch::splitLeftRight(const Vector3Dd (&net)[4][4], Vector3Dd (&left)[4][4],
    Vector3Dd (&right)[4][4])
{
    Vector3Dd temp1[4];
    Vector3Dd temp2[4];
    for (int i = 0; i < 4; i++) {
        temp1[0] = net[i][0];
        temp1[1] = net[i][0].midpoint(net[i][1]);
        const Vector3Dd half = net[i][1].midpoint(net[i][2]);
        temp1[2] = temp1[1].midpoint(half);
        temp2[2] = net[i][2].midpoint(net[i][3]);
        temp2[1] = half.midpoint(temp2[2]);
        temp1[3] = temp1[2].midpoint(temp2[1]);
        temp2[0] = temp1[3];
        temp2[3] = net[i][3];
        for (int j = 0; j < 4; j++) {
            left[i][j] = temp1[j];
            right[i][j] = temp2[j];
        }
    }
}

// Splits in u: bottom receives the first half and top the second. The net
// may alias either output; each column is read before it is written.
void SubdividedBezierPatch::splitUpDown(const Vector3Dd (&net)[4][4], Vector3Dd (&top)[4][4],
    Vector3Dd (&bottom)[4][4])
{
    Vector3Dd temp1[4];
    Vector3Dd temp2[4];
    for (int i = 0; i < 4; i++) {
        temp1[0] = net[0][i];
        temp1[1] = net[0][i].midpoint(net[1][i]);
        const Vector3Dd half = net[1][i].midpoint(net[2][i]);
        temp1[2] = temp1[1].midpoint(half);
        temp2[2] = net[2][i].midpoint(net[3][i]);
        temp2[1] = half.midpoint(temp2[2]);
        temp1[3] = temp1[2].midpoint(temp2[1]);
        temp2[0] = temp1[3];
        temp2[3] = net[3][i];
        for (int j = 0; j < 4; j++) {
            bottom[j][i] = temp1[j];
            top[j][i] = temp2[j];
        }
    }
}

// Largest distance of the net from the plane of three distinct corners, or
// a negative value when no three distinct corners are found.
double SubdividedBezierPatch::netFlatness(const Vector3Dd (&net)[4][4])
{
    Vector3Dd vertices[3];
    vertices[0] = net[0][0];
    vertices[1] = net[0][3];
    double temp1 = vertices[0].subtract(vertices[1]).length();
    if (java::Math::abs(temp1) < DEGENERACY_EPSILON) {
        vertices[1] = net[3][3];
        temp1 = vertices[0].subtract(vertices[1]).length();
        if (java::Math::abs(temp1) < DEGENERACY_EPSILON) {
            return -1.0;
        }
        vertices[2] = net[3][0];
        temp1 = vertices[0].subtract(vertices[1]).length();
        if (java::Math::abs(temp1) < DEGENERACY_EPSILON) {
            return -1.0;
        }
        temp1 = vertices[1].subtract(vertices[2]).length();
        if (java::Math::abs(temp1) < DEGENERACY_EPSILON) {
            return -1.0;
        }
    } else {
        vertices[2] = net[3][0];
        temp1 = vertices[0].subtract(vertices[1]).length();
        if (java::Math::abs(temp1) < DEGENERACY_EPSILON) {
            vertices[2] = net[3][3];
            temp1 = vertices[0].subtract(vertices[2]).length();
            if (java::Math::abs(temp1) < DEGENERACY_EPSILON) {
                return -1.0;
            }
            temp1 = vertices[1].subtract(vertices[2]).length();
            if (java::Math::abs(temp1) < DEGENERACY_EPSILON) {
                return -1.0;
            }
        } else {
            temp1 = vertices[1].subtract(vertices[2]).length();
            if (java::Math::abs(temp1) < DEGENERACY_EPSILON) {
                return -1.0;
            }
        }
    }
    Vector3Dd n;
    double d;
    if (trianglePlane(vertices[0], vertices[1], vertices[2], n, d)) {
        double dist = 0.0;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                temp1 = java::Math::abs(planeDistance(net[i][j], n, d));
                if (temp1 > dist) {
                    dist = temp1;
                }
            }
        }
        return dist;
    }
    return -1.0;
}

bool SubdividedBezierPatch::isFlatEnough(const Vector3Dd (&net)[4][4]) const
{
    const double dist = netFlatness(net);
    if (dist < 0.0) {
        return false;
    }
    return dist < flatness;
}

// Ray against the triangle v1 v2 v3 of plane n.p + d = 0, by a crossing
// count in its dominant projection.
bool SubdividedBezierPatch::intersectTriangle(const Vector3Dd& origin, const Vector3Dd& direction,
    double minimumDepth, bool orientNormal, const Vector3Dd& v1, const Vector3Dd& v2, const Vector3Dd& v3,
    const Vector3Dd& n, double d, const Vector3Dd* n1, const Vector3Dd* n2, const Vector3Dd* n3,
    double& depth, Vector3Dd& point, Vector3Dd& normal) const
{
    double s = direction.dotProduct(n);
    if (s == 0.0) {
        return false;
    }
    double t = origin.dotProduct(n);
    depth = 0.0 - (d + t) / s;
    if (depth < minimumDepth) {
        return false;
    }
    const Vector3Dd ip = direction.multiply(depth).add(origin);
    double x = java::Math::abs(n.x());
    double y = java::Math::abs(n.y());
    double z = java::Math::abs(n.z());
    double x1, y1, x2, y2, x3, y3;
    if (x > y) {
        if (x > z) {
            x1 = v1.y() - ip.y(); y1 = v1.z() - ip.z();
            x2 = v2.y() - ip.y(); y2 = v2.z() - ip.z();
            x3 = v3.y() - ip.y(); y3 = v3.z() - ip.z();
        } else {
            x1 = v1.x() - ip.x(); y1 = v1.y() - ip.y();
            x2 = v2.x() - ip.x(); y2 = v2.y() - ip.y();
            x3 = v3.x() - ip.x(); y3 = v3.y() - ip.y();
        }
    } else if (y > z) {
        x1 = v1.x() - ip.x(); y1 = v1.z() - ip.z();
        x2 = v2.x() - ip.x(); y2 = v2.z() - ip.z();
        x3 = v3.x() - ip.x(); y3 = v3.z() - ip.z();
    } else {
        x1 = v1.x() - ip.x(); y1 = v1.y() - ip.y();
        x2 = v2.x() - ip.x(); y2 = v2.y() - ip.y();
        x3 = v3.x() - ip.x(); y3 = v3.y() - ip.y();
    }
    int crossings = 0;
    int signHolder = (y1 < 0) ? -1 : 1;
    int nextSign = (y2 < 0) ? -1 : 1;
    if (signHolder != nextSign) {
        if (x1 > 0.0) {
            if (x2 > 0.0) {
                crossings++;
            } else if ((x1 - y1 * (x2 - x1) / (y2 - y1)) > 0.0) {
                crossings++;
            }
        } else if (x2 > 0.0) {
            if ((x1 - y1 * (x2 - x1) / (y2 - y1)) > 0.0) {
                crossings++;
            }
        }
        signHolder = nextSign;
    }
    nextSign = (y3 < 0) ? -1 : 1;
    if (signHolder != nextSign) {
        if (x2 > 0.0) {
            if (x3 > 0.0) {
                crossings++;
            } else if ((x2 - y2 * (x3 - x2) / (y3 - y2)) > 0.0) {
                crossings++;
            }
        } else if (x3 > 0.0) {
            if ((x2 - y2 * (x3 - x2) / (y3 - y2)) > 0.0) {
                crossings++;
            }
        }
        signHolder = nextSign;
    }
    nextSign = (y1 < 0) ? -1 : 1;
    if (signHolder != nextSign) {
        if (x3 > 0.0) {
            if (x1 > 0.0) {
                crossings++;
            } else if ((x3 - y3 * (x1 - x3) / (y1 - y3)) > 0.0) {
                crossings++;
            }
        } else if (x1 > 0.0) {
            if ((x3 - y3 * (x1 - x3) / (y1 - y3)) > 0.0) {
                crossings++;
            }
        }
    }
    if (crossings != 1) {
        return false;
    }
    point = ip;
    if (subdivision != BezierPatchSubdivision::SmoothGrid) {
        normal = n;
        return true;
    }
    // Normal interpolated from the corner normals n1, n2, n3.
    Vector3Dd tempV1 = v2.subtract(v3).normalizedFast();
    Vector3Dd tempV2 = v1.subtract(v3);
    const double proj = tempV2.dotProduct(tempV1);
    tempV1 = tempV1.multiply(proj);
    Vector3Dd perp = tempV1.subtract(tempV2).normalizedFast();
    double mu = tempV2.dotProduct(perp);
    mu = -1.0 / mu;
    perp = perp.multiply(mu);
    tempV1 = ip.subtract(v1);
    s = tempV1.dotProduct(perp);
    if (s < DEGENERACY_EPSILON) {
        normal = *n1;
        if (orientNormal && normal.dotProduct(n) < 0) normal = normal.multiply(-1.0);
        return true;
    }
    tempV1 = v3.subtract(v2);
    x = java::Math::abs(tempV1.x());
    y = java::Math::abs(tempV1.y());
    z = java::Math::abs(tempV1.z());
    if (x > y) {
        if (x > z) {
            t = (tempV1.x() / s + v1.x() - v2.x()) / tempV1.x();
        } else {
            t = (tempV1.z() / s + v1.z() - v2.z()) / tempV1.z();
        }
    } else if (y > z) {
        t = (tempV1.y() / s + v1.y() - v2.y()) / tempV1.y();
    } else {
        t = (tempV1.z() / s + v1.z() - v2.z()) / tempV1.z();
    }
    tempV1 = n2->subtract(*n1).multiply(s).add(*n1);
    tempV2 = n3->subtract(*n1).multiply(s).add(*n1);
    normal = tempV2.subtract(tempV1).multiply(t).add(tempV1).normalizedFast();
    if (orientNormal) {
        // The interpolation can divide by zero; then use the triangle normal.
        if (!java::Double::isFinite(normal.x()) || !java::Double::isFinite(normal.y()) ||
            !java::Double::isFinite(normal.z())) normal = n;
        if (normal.dotProduct(n) < 0) normal = normal.multiply(-1.0);
    }
    return true;
}

long SubdividedBezierPatch::buildTree(const Vector3Dd (&net)[4][4], int depth)
{
    const long index = nodes.size();
    nodes.add(SubdividedBezierPatchNode());
    Vector3Dd center;
    double radiusSquared;
    netSphere(net, center, radiusSquared);
    nodes[index].center = center;
    nodes[index].radiusSquared = radiusSquared;
    Vector3Dd lowerLeft[4][4];
    Vector3Dd lowerRight[4][4];
    Vector3Dd upperLeft[4][4];
    Vector3Dd upperRight[4][4];
    long children[4] = {-1, -1, -1, -1};
    int count = 0;
    if (isFlatEnough(net) || (depth >= uSteps && depth >= vSteps)) {
        nodes[index].leaf = true;
        nodes[index].corners[0] = net[0][0];
        nodes[index].corners[1] = net[0][3];
        nodes[index].corners[2] = net[3][3];
        nodes[index].corners[3] = net[3][0];
        return index;
    }
    if (depth >= uSteps) {
        splitUpDown(net, lowerLeft, upperLeft);
        children[0] = buildTree(lowerLeft, depth + 1);
        children[1] = buildTree(upperLeft, depth + 1);
        count = 2;
    } else if (depth >= vSteps) {
        splitLeftRight(net, lowerLeft, lowerRight);
        children[0] = buildTree(lowerLeft, depth + 1);
        children[1] = buildTree(lowerRight, depth + 1);
        count = 2;
    } else {
        splitLeftRight(net, lowerLeft, lowerRight);
        splitUpDown(lowerLeft, lowerLeft, upperLeft);
        splitUpDown(lowerRight, lowerRight, upperRight);
        children[0] = buildTree(lowerLeft, depth + 1);
        children[1] = buildTree(upperLeft, depth + 1);
        children[2] = buildTree(lowerRight, depth + 1);
        children[3] = buildTree(upperRight, depth + 1);
        count = 4;
    }
    nodes[index].leaf = false;
    nodes[index].childCount = count;
    for (int i = 0; i < 4; i++) nodes[index].children[i] = children[i];
    return index;
}

void SubdividedBezierPatch::precompute()
{
    Vector3Dd netPoints[16];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            netPoints[4 * i + j] = controlPoints[i][j];
        }
    }
    sphereAround(16, netPoints, boundingCenter, boundingRadiusSquared);
    if (subdivision == BezierPatchSubdivision::UniformGrid ||
        subdivision == BezierPatchSubdivision::Adaptive) {
        return;
    }
    if (subdivision == BezierPatchSubdivision::AdaptiveTree) {
        buildTree(controlPoints, 0);
        return;
    }
    const double deltaU = 1.0 / (double)uSteps;
    const double deltaV = 1.0 / (double)vSteps;
    for (long i = 0; i < (long)(uSteps + 1)*(vSteps + 1); i++) gridPoints.add(Vector3Dd());
    for (long i = 0; i < (long)(uSteps + 1)*2*(vSteps + 1); i++) {
        gridPlaneNormals.add(Vector3Dd());
        gridPlaneOffsets.add(0.0);
    }
    // The PovRay fill order, kept because later writes overwrite earlier
    // ones with differently rounded parameters.
    for (int i = 0; i <= uSteps; i++) {
        const double u = (double)i / (double)uSteps;
        for (int j = 0; j < vSteps; j++) {
            gridPoints[gridIndex(i, j)] = evaluate(u, (double)j / (double)vSteps);
        }
    }
    for (int i = 0; i < uSteps; i++) {
        const double u = (double)i / (double)uSteps;
        for (int j = 0; j < vSteps; j++) {
            const double v = (double)j / (double)vSteps;
            const Vector3Dd v0 = evaluate(u, v);
            const Vector3Dd v1 = evaluate(u + deltaU, v);
            const Vector3Dd v2 = evaluate(u, v + deltaV);
            const Vector3Dd v3 = evaluate(u + deltaU, v + deltaV);
            gridPoints[gridIndex(i, j)] = v0;
            gridPoints[gridIndex(i + 1, j)] = v1;
            gridPoints[gridIndex(i, j + 1)] = v2;
            gridPoints[gridIndex(i + 1, j + 1)] = v3;
            Vector3Dd n;
            double d;
            if (trianglePlane(v0, v2, v1, n, d)) {
                gridPlaneNormals[planeIndex(i, 2 * j)] = n;
                gridPlaneOffsets[planeIndex(i, 2 * j)] = d;
            } else {
                gridPlaneNormals[planeIndex(i, 2 * j)] = Vector3Dd(0.0, 0.0, 0.0);
                gridPlaneOffsets[planeIndex(i, 2 * j)] = 0.0;
            }
            if (trianglePlane(v1, v2, v3, n, d)) {
                gridPlaneNormals[planeIndex(i, 2 * j + 1)] = n;
                gridPlaneOffsets[planeIndex(i, 2 * j + 1)] = d;
            } else {
                gridPlaneNormals[planeIndex(i, 2 * j + 1)] = Vector3Dd(0.0, 0.0, 0.0);
                gridPlaneOffsets[planeIndex(i, 2 * j + 1)] = 0.0;
            }
        }
    }
    if (subdivision == BezierPatchSubdivision::SmoothGrid) {
        for (int i = 0; i <= uSteps; i++) {
            const double u = (double)i / (double)uSteps;
            for (int j = 0; j <= vSteps; j++) {
                gridSmoothNormals.add(evaluateNormal(u, (double)j / (double)vSteps));
            }
        }
    }
}

int SubdividedBezierPatch::intersectUniform(const Vector3Dd& origin, const Vector3Dd& direction,
    double minimumDepth, HitList& hits) const
{
    if (!rayMeetsSphere(origin, direction, boundingCenter, boundingRadiusSquared)) {
        return 0;
    }
    const double deltaU = 1.0 / (double)uSteps;
    const double deltaV = 1.0 / (double)vSteps;
    for (int i = 0; i < uSteps; i++) {
        const double u = (double)i / (double)uSteps;
        for (int j = 0; j < vSteps; j++) {
            const double v = (double)j / (double)vSteps;
            const Vector3Dd v0 = evaluate(u, v);
            const Vector3Dd v1 = evaluate(u + deltaU, v);
            const Vector3Dd v2 = evaluate(u, v + deltaV);
            const Vector3Dd v3 = evaluate(u + deltaU, v + deltaV);
            Vector3Dd n, ip, normal;
            double d, depth;
            if (trianglePlane(v0, v2, v1, n, d) && intersectTriangle(origin, direction, minimumDepth, hits.orientNormals,
                    v0, v2, v1, n, d, nullptr, nullptr, nullptr, depth, ip, normal)) {
                hits.add(depth, ip, normal);
                if (hits.isFull()) return hits.count;
            }
            if (trianglePlane(v1, v2, v3, n, d) && intersectTriangle(origin, direction, minimumDepth, hits.orientNormals,
                    v1, v2, v3, n, d, nullptr, nullptr, nullptr, depth, ip, normal)) {
                hits.add(depth, ip, normal);
                if (hits.isFull()) return hits.count;
            }
        }
    }
    return hits.count;
}

int SubdividedBezierPatch::intersectGrid(const Vector3Dd& origin, const Vector3Dd& direction,
    double minimumDepth, HitList& hits) const
{
    if (!rayMeetsSphere(origin, direction, boundingCenter, boundingRadiusSquared)) {
        return 0;
    }
    for (int i = 0; i < uSteps; i++) {
        for (int j = 0; j < vSteps; j++) {
            Vector3Dd v[4];
            v[0] = gridPoints[gridIndex(i, j)];
            v[1] = gridPoints[gridIndex(i + 1, j)];
            v[2] = gridPoints[gridIndex(i, j + 1)];
            v[3] = gridPoints[gridIndex(i + 1, j + 1)];
            Vector3Dd center;
            double radiusSquared;
            sphereAround(4, v, center, radiusSquared);
            if (!rayMeetsSphere(origin, direction, center, radiusSquared)) {
                continue;
            }
            Vector3Dd ip, normal;
            double depth;
            Vector3Dd n = gridPlaneNormals[planeIndex(i, 2 * j)];
            if (!(n.x() == 0.0 && n.y() == 0.0 && n.z() == 0.0) &&
                intersectTriangle(origin, direction, minimumDepth, hits.orientNormals, v[0], v[2], v[1], n,
                    gridPlaneOffsets[planeIndex(i, 2 * j)], nullptr, nullptr, nullptr, depth, ip, normal)) {
                hits.add(depth, ip, normal);
                if (hits.isFull()) return hits.count;
            }
            n = gridPlaneNormals[planeIndex(i, 2 * j + 1)];
            if (n.x() == 0.0 && n.y() == 0.0 && n.z() == 0.0) {
                continue;
            }
            if (intersectTriangle(origin, direction, minimumDepth, hits.orientNormals, v[1], v[2], v[3], n,
                    gridPlaneOffsets[planeIndex(i, 2 * j + 1)], nullptr, nullptr, nullptr, depth, ip, normal)) {
                hits.add(depth, ip, normal);
                if (hits.isFull()) return hits.count;
            }
        }
    }
    return hits.count;
}

int SubdividedBezierPatch::intersectSmooth(const Vector3Dd& origin, const Vector3Dd& direction,
    double minimumDepth, HitList& hits) const
{
    if (!rayMeetsSphere(origin, direction, boundingCenter, boundingRadiusSquared)) {
        return 0;
    }
    for (int i = 0; i < uSteps; i++) {
        for (int j = 0; j < vSteps; j++) {
            const Vector3Dd v0 = gridPoints[gridIndex(i, j)];
            const Vector3Dd v1 = gridPoints[gridIndex(i + 1, j)];
            const Vector3Dd v2 = gridPoints[gridIndex(i, j + 1)];
            const Vector3Dd v3 = gridPoints[gridIndex(i + 1, j + 1)];
            Vector3Dd n0 = gridSmoothNormals[gridIndex(i, j)];
            Vector3Dd n1 = gridSmoothNormals[gridIndex(i + 1, j)];
            Vector3Dd n2 = gridSmoothNormals[gridIndex(i, j + 1)];
            Vector3Dd n3 = gridSmoothNormals[gridIndex(i + 1, j + 1)];
            Vector3Dd ip, normal;
            double depth;
            Vector3Dd n = gridPlaneNormals[planeIndex(i, 2 * j)];
            if (!(n.x() == 0.0 && n.y() == 0.0 && n.z() == 0.0)) {
                // Corner normals turned to the side of the triangle normal.
                if (n0.dotProduct(n) < 0) n0 = n0.multiply(-1.0);
                if (n1.dotProduct(n) < 0) n1 = n1.multiply(-1.0);
                if (n2.dotProduct(n) < 0) n2 = n2.multiply(-1.0);
                if (intersectTriangle(origin, direction, minimumDepth, hits.orientNormals, v0, v2, v1, n,
                        gridPlaneOffsets[planeIndex(i, 2 * j)], &n0, &n2, &n1, depth, ip, normal)) {
                    hits.add(depth, ip, normal);
                    if (hits.isFull()) return hits.count;
                }
            }
            n = gridPlaneNormals[planeIndex(i, 2 * j + 1)];
            if (n.x() == 0.0 && n.y() == 0.0 && n.z() == 0.0) {
                continue;
            }
            // PovRay replaces, rather than turns, these corner normals.
            if (n1.dotProduct(n) > 0) n1 = n0.multiply(-1.0);
            if (n2.dotProduct(n) > 0) n2 = n1.multiply(-1.0);
            if (n3.dotProduct(n) > 0) n3 = n2.multiply(-1.0);
            if (intersectTriangle(origin, direction, minimumDepth, hits.orientNormals, v1, v2, v3, n,
                    gridPlaneOffsets[planeIndex(i, 2 * j + 1)], &n1, &n2, &n3, depth, ip, normal)) {
                hits.add(depth, ip, normal);
                if (hits.isFull()) return hits.count;
            }
        }
    }
    return hits.count;
}

void SubdividedBezierPatch::intersectPiece(const Vector3Dd& origin, const Vector3Dd& direction,
    double minimumDepth, const Vector3Dd& c0, const Vector3Dd& c1, const Vector3Dd& c2,
    const Vector3Dd& c3, HitList& hits) const
{
    Vector3Dd n, ip, normal;
    double d, depth;
    if (trianglePlane(c0, c1, c2, n, d) && intersectTriangle(origin, direction, minimumDepth, hits.orientNormals,
            c0, c1, c2, n, d, nullptr, nullptr, nullptr, depth, ip, normal)) {
        hits.add(depth, ip, normal);
    }
    if (hits.isFull()) {
        return;
    }
    if (trianglePlane(c0, c2, c3, n, d) && intersectTriangle(origin, direction, minimumDepth, hits.orientNormals,
            c0, c2, c3, n, d, nullptr, nullptr, nullptr, depth, ip, normal)) {
        hits.add(depth, ip, normal);
    }
}

void SubdividedBezierPatch::subdivide(const Vector3Dd& origin, const Vector3Dd& direction,
    double minimumDepth, const Vector3Dd (&net)[4][4], int depth, HitList& hits) const
{
    if (hits.used >= hits.maximum) {
        return;
    }
    Vector3Dd center;
    double radiusSquared;
    netSphere(net, center, radiusSquared);
    if (!rayMeetsSphere(origin, direction, center, radiusSquared)) {
        return;
    }
    // PovRay tests a flat piece and still subdivides it.
    if (isFlatEnough(net) && !hits.isFull()) {
        intersectPiece(origin, direction, minimumDepth, net[0][0], net[0][3], net[3][3], net[3][0], hits);
    }
    Vector3Dd lowerLeft[4][4];
    Vector3Dd lowerRight[4][4];
    Vector3Dd upperLeft[4][4];
    Vector3Dd upperRight[4][4];
    if (depth >= uSteps) {
        if (depth >= vSteps) {
            if (!hits.isFull()) {
                intersectPiece(origin, direction, minimumDepth, net[0][0], net[0][3], net[3][3], net[3][0], hits);
            }
        } else {
            splitUpDown(net, lowerLeft, upperLeft);
            subdivide(origin, direction, minimumDepth, lowerLeft, depth + 1, hits);
            subdivide(origin, direction, minimumDepth, upperLeft, depth + 1, hits);
        }
    } else if (depth >= vSteps) {
        splitLeftRight(net, lowerLeft, lowerRight);
        subdivide(origin, direction, minimumDepth, lowerLeft, depth + 1, hits);
        subdivide(origin, direction, minimumDepth, lowerRight, depth + 1, hits);
    } else {
        splitLeftRight(net, lowerLeft, lowerRight);
        splitUpDown(lowerLeft, lowerLeft, upperLeft);
        splitUpDown(lowerRight, lowerRight, upperRight);
        subdivide(origin, direction, minimumDepth, lowerLeft, depth + 1, hits);
        subdivide(origin, direction, minimumDepth, upperLeft, depth + 1, hits);
        subdivide(origin, direction, minimumDepth, lowerRight, depth + 1, hits);
        subdivide(origin, direction, minimumDepth, upperRight, depth + 1, hits);
    }
}

void SubdividedBezierPatch::walkTree(const Vector3Dd& origin, const Vector3Dd& direction,
    double minimumDepth, long index, HitList& hits) const
{
    if (hits.used >= hits.maximum) {
        return;
    }
    const SubdividedBezierPatchNode& node = nodes[index];
    if (!rayMeetsSphere(origin, direction, node.center, node.radiusSquared)) {
        return;
    }
    if (!node.leaf) {
        for (int i = 0; i < node.childCount; i++) {
            walkTree(origin, direction, minimumDepth, node.children[i], hits);
        }
        return;
    }
    intersectPiece(origin, direction, minimumDepth, node.corners[0], node.corners[1],
        node.corners[2], node.corners[3], hits);
}

int SubdividedBezierPatch::intersect(const Vector3Dd& origin, const Vector3Dd& direction,
    double minimumDepth, int alreadyUsed, int maximumHits, double* depths, Vector3Dd* points,
    Vector3Dd* normals) const
{
    return intersectWith(origin, direction, minimumDepth, false, alreadyUsed, maximumHits, depths,
        points, normals);
}

int SubdividedBezierPatch::intersectOriented(const Vector3Dd& origin, const Vector3Dd& direction,
    double minimumDepth, int alreadyUsed, int maximumHits, double* depths, Vector3Dd* points,
    Vector3Dd* normals) const
{
    return intersectWith(origin, direction, minimumDepth, true, alreadyUsed, maximumHits, depths,
        points, normals);
}

int SubdividedBezierPatch::intersectWith(const Vector3Dd& origin, const Vector3Dd& direction,
    double minimumDepth, bool orient, int alreadyUsed, int maximumHits, double* depths,
    Vector3Dd* points, Vector3Dd* normals) const
{
    HitList hits;
    hits.used = alreadyUsed;
    hits.maximum = maximumHits;
    hits.count = 0;
    hits.depths = depths;
    hits.points = points;
    hits.normals = normals;
    hits.orientNormals = orient;
    switch (subdivision) {
    case BezierPatchSubdivision::UniformGrid:
        return intersectUniform(origin, direction, minimumDepth, hits);
    case BezierPatchSubdivision::PrecomputedGrid:
        return intersectGrid(origin, direction, minimumDepth, hits);
    case BezierPatchSubdivision::Adaptive:
        subdivide(origin, direction, minimumDepth, controlPoints, 0, hits);
        return hits.count;
    case BezierPatchSubdivision::AdaptiveTree:
        if (!nodes.isEmpty()) walkTree(origin, direction, minimumDepth, 0, hits);
        return hits.count;
    case BezierPatchSubdivision::SmoothGrid:
        return intersectSmooth(origin, direction, minimumDepth, hits);
    }
    return 0;
}

RayFirstHitStatus SubdividedBezierPatch::collectHits(const Ray& ray, const RayCrossingQuery& query,
    int& count, double* depths, Vector3Dd* normals) const
{
    count = 0;
    const Vector3Dd& o = ray.getOrigin();
    const Vector3Dd& d = ray.getDirection();
    if (!java::Double::isFinite(o.x()) || !java::Double::isFinite(o.y()) || !java::Double::isFinite(o.z()) ||
        !java::Double::isFinite(d.x()) || !java::Double::isFinite(d.y()) || !java::Double::isFinite(d.z()) ||
        (d.x() == 0 && d.y() == 0 && d.z() == 0) || java::Double::isNaN(query.minT) ||
        java::Double::isNaN(query.maxT) || query.minT > query.maxT) return RayFirstHitStatus::InvalidInput;
    // Hits behind the origin are not all reported, so the query must start
    // at or after it.
    if (!(query.minT >= 0) || query.minimumChordSquared != 0 ||
        query.minimumExteriorApproach != java::Double::NEGATIVE_INFINITY) return RayFirstHitStatus::Unsupported;
    Vector3Dd points[MAXIMUM_HITS];
    const int found = intersectOriented(o, d, query.minT, 0, MAXIMUM_HITS, depths, points, normals);
    // A full budget may have hidden further hits.
    if (found >= MAXIMUM_HITS) return RayFirstHitStatus::Indeterminate;
    for (int i = 0; i < found; i++) {
        if (!java::Double::isFinite(depths[i])) return RayFirstHitStatus::Indeterminate;
    }
    count = found;
    return RayFirstHitStatus::Hit;
}

RayFirstHitStatus SubdividedBezierPatch::queryFirstHit(const Ray& ray, const RayCrossingQuery& query,
    RayHit* hit, RayBoundary* boundary) const
{
    if (hit) hit->clear();
    if (boundary) *boundary = RayBoundary();
    double depths[MAXIMUM_HITS];
    Vector3Dd normals[MAXIMUM_HITS];
    int count = 0;
    const RayFirstHitStatus status = collectHits(ray, query, count, depths, normals);
    if (status != RayFirstHitStatus::Hit) return status;
    int best = -1;
    for (int i = 0; i < count; i++) {
        if (depths[i] <= query.maxT && (best < 0 || depths[i] < depths[best])) best = i;
    }
    if (best < 0) return RayFirstHitStatus::Miss;
    if (boundary) *boundary = RayBoundary(depths[best], this, false, best);
    if (hit) {
        hit->setHitDistance(depths[best]);
        if (hit->shouldStoreRay()) hit->setRay(ray.withT(depths[best]));
        if (hit->needsAnySurfaceData()) {
            hit->beginDetailEvaluation();
            if (hit->needsPoint()) {
                hit->point = ray.getOrigin().add(ray.getDirection().multiply(depths[best]));
                hit->markDetailAvailable(RayHit::DETAIL_POINT);
            }
            if (hit->needsNormal()) {
                hit->normal = normals[best];
                hit->markDetailAvailable(RayHit::DETAIL_NORMAL);
            }
        }
    }
    return RayFirstHitStatus::Hit;
}

RayFirstHitStatus SubdividedBezierPatch::querySurfaceDetail(const Ray& ray, const RayBoundary& boundary,
    RayHit* hit) const
{
    if (hit) hit->clear();
    if (!hit || boundary.geometry != this || !java::Double::isFinite(boundary.t) ||
        boundary.surfaceIndex < 0 || boundary.surfaceIndex >= MAXIMUM_HITS)
        return RayFirstHitStatus::InvalidInput;
    if (boundary.getInstanceCount() != 0) return RayFirstHitStatus::Unsupported;
    double depths[MAXIMUM_HITS];
    Vector3Dd normals[MAXIMUM_HITS];
    int count = 0;
    const RayFirstHitStatus status = collectHits(ray,
        RayCrossingQuery(0, java::Double::POSITIVE_INFINITY), count, depths, normals);
    if (status != RayFirstHitStatus::Hit) return status;
    const int index = (int)boundary.surfaceIndex;
    if (index >= count || depths[index] != boundary.t) return RayFirstHitStatus::Indeterminate;
    hit->setHitDistance(boundary.t);
    if (hit->shouldStoreRay()) hit->setRay(ray.withT(boundary.t));
    hit->beginDetailEvaluation();
    if (hit->needsPoint()) {
        hit->point = ray.getOrigin().add(ray.getDirection().multiply(boundary.t));
        hit->markDetailAvailable(RayHit::DETAIL_POINT);
    }
    if (hit->needsNormal()) {
        hit->normal = boundary.normalReversed ? normals[index].multiply(-1.0) : normals[index];
        hit->markDetailAvailable(RayHit::DETAIL_NORMAL);
    }
    return RayFirstHitStatus::Hit;
}

RayCrossingSupport SubdividedBezierPatch::emitRayCrossings(const Ray& ray, const RayCrossingQuery& query,
    RayCrossingSink& sink) const
{
    double depths[MAXIMUM_HITS];
    Vector3Dd normals[MAXIMUM_HITS];
    int count = 0;
    const RayFirstHitStatus status = collectHits(ray, query, count, depths, normals);
    if (status == RayFirstHitStatus::InvalidInput) return RayCrossingSupport::InvalidInput;
    if (status == RayFirstHitStatus::Unsupported) return RayCrossingSupport::Unsupported;
    if (status != RayFirstHitStatus::Hit) return RayCrossingSupport::Indeterminate;
    int order[MAXIMUM_HITS];
    int sorted = 0;
    for (int i = 0; i < count; i++) {
        if (depths[i] > query.maxT) continue;
        int at = sorted;
        while (at > 0 && depths[order[at - 1]] > depths[i]) {
            order[at] = order[at - 1];
            at--;
        }
        order[at] = i;
        sorted++;
    }
    for (int i = 0; i < sorted; i++) {
        RayCrossing event(depths[order[i]], this, RayCrossing::Kind::Unclassified);
        event.surfaceIndex = order[i];
        sink.crossing(event);
    }
    return RayCrossingSupport::Supported;
}

void SubdividedBezierPatch::getBounds(GeometryBounds& result) const
{
    Vector3Dd low = controlPoints[0][0];
    Vector3Dd high = controlPoints[0][0];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            const Vector3Dd& p = controlPoints[i][j];
            low = Vector3Dd(java::Math::min(low.x(), p.x()), java::Math::min(low.y(), p.y()),
                java::Math::min(low.z(), p.z()));
            high = Vector3Dd(java::Math::max(high.x(), p.x()), java::Math::max(high.y(), p.y()),
                java::Math::max(high.z(), p.z()));
        }
    }
    if (!result.setFinite(low, high)) result.setIndeterminate();
}

bool SubdividedBezierPatch::doIntersectionFirstHit(const Ray& ray, RayHit* hit)
{
    return queryFirstHit(ray, RayCrossingQuery(POVRAY_MINIMUM_DEPTH, java::Double::POSITIVE_INFINITY), hit) ==
        RayFirstHitStatus::Hit;
}

void SubdividedBezierPatch::doExtraInformation(const Ray& ray, double t, RayHit* hit)
{
    if (!hit) return;
    hit->beginDetailEvaluation();
    if (hit->needsPoint()) {
        hit->point = ray.getOrigin().add(ray.getDirection().multiply(t));
        hit->markDetailAvailable(RayHit::DETAIL_POINT);
    }
    if (!hit->needsNormal()) return;
    double depths[MAXIMUM_HITS];
    Vector3Dd normals[MAXIMUM_HITS];
    int count = 0;
    if (collectHits(ray, RayCrossingQuery(0, java::Double::POSITIVE_INFINITY), count, depths, normals) !=
        RayFirstHitStatus::Hit) return;
    // The caller's ray may be rebuilt from the hit, so match the nearest depth.
    int best = -1;
    for (int i = 0; i < count; i++) {
        if (best < 0 || java::Math::abs(depths[i] - t) < java::Math::abs(depths[best] - t)) best = i;
    }
    if (best >= 0 && java::Math::abs(depths[best] - t) <= 1.0e-6*(1.0 + java::Math::abs(t))) {
        hit->normal = normals[best];
        hit->markDetailAvailable(RayHit::DETAIL_NORMAL);
    }
}

double* SubdividedBezierPatch::getMinMax()
{
    GeometryBounds bounds;
    getBounds(bounds);
    if (bounds.getKind() == GeometryBounds::Kind::Finite)
        return new double[6]{bounds.getMinimum().x(), bounds.getMinimum().y(), bounds.getMinimum().z(),
            bounds.getMaximum().x(), bounds.getMaximum().y(), bounds.getMaximum().z()};
    return new double[6]{-1e308,-1e308,-1e308,1e308,1e308,1e308};
}
