#include <cmath>
#include <cstdio>
#include <cstring>

#include "java/lang/String.h"
#include "java/lang/Double.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDKFatalException.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/material/SimpleMaterial.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/volume/PlanarHalfSpace.h"
#include "vsdk/toolkit/environment/geometry/volume/QuadricSurface.h"
#include "vsdk/toolkit/environment/geometry/volume/PolynomialSurface.h"
#include "vsdk/toolkit/environment/geometry/volume/BlobSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/HeightFieldSolid.h"
#include "vsdk/toolkit/environment/geometry/surface/SubdividedBezierPatch.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/environment/background/SimpleBackground.h"
#include "vsdk/toolkit/environment/camera/Camera.h"
#include "vsdk/toolkit/environment/light/AmbientLight.h"
#include "vsdk/toolkit/environment/light/DirectionalLight.h"
#include "vsdk/toolkit/environment/light/PointLight.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/environment/scene/SimpleScene.h"
#include "vsdk/toolkit/environment/scene/csg/CsgBody.h"
#include "vsdk/toolkit/io/geometry/ReaderAltimetry.h"
#include "vsdk/toolkit/io/geometry/ReaderMitScene.h"
#include "vsdk/toolkit/io/geometry/ReaderMitTriangleGroup.h"
ReaderMitScene::ReaderMitScene()
{
}

static java::String trimLine(const java::String& in)
{
    size_t a = 0;
    while ( a < in.size() && (in[a] == ' ' || in[a] == '\t' || in[a] == '\r' || in[a] == '\n') ) a++;
    size_t b = in.size();
    while ( b > a && (in[b-1] == ' ' || in[b-1] == '\t' || in[b-1] == '\r' || in[b-1] == '\n') ) b--;
    return in.substr(a, b - a);
}

void ReaderMitScene::importEnvironment(const char* fileName, SimpleScene* outScene)
{
    if ( outScene == 0 ) {
        Logger::reportMessage("ReaderMitScene", Logger::ERROR, "importEnvironment", "outScene is null");
        throw VSDKFatalException("ReaderMitScene::importEnvironment: outScene is null");
    }

    FILE* file = fopen(fileName, "r");
    if ( !file ) {
        Logger::reportMessage("ReaderMitScene", Logger::ERROR, "importEnvironment", "could not open file");
        throw VSDKFatalException("ReaderMitScene::importEnvironment: could not open file");
    }

    outScene->clearOwnedElements();

    Camera* camera =
        new Camera();
    SimpleBackground* background = new SimpleBackground();
    outScene->addCamera(camera);
    outScene->addBackground(background);
    outScene->setActiveCameraIndex(0);
    outScene->setActiveBackgroundIndex(0);

    Vector3Dd eye = Vector3Dd(0, 0, 10);
    Vector3Dd up = Vector3Dd(0, 1, 0);
    Vector3Dd lookat = camera->getFocusedPosition();
    bool hasEye = false;
    bool hasUp = false;
    bool hasLookat = false;
    bool hasViewport = false;
    int viewportXSize = 320;
    int viewportYSize = 240;
    double importedHorizontalFov = 30.0;

    SimpleMaterial* currentMaterial = new SimpleMaterial();
    ReaderMitTriangleGroup triangleGroup;
    bool inTriangles = false;
    // Open CSG body and its node stack; primitives inside become its leaves.
    CsgBody* csgBody = nullptr;
    java::ArrayList<long long> csgNodes;
    // Open polynomial or blob block.
    PolynomialSurface* polynomial = nullptr;
    BlobSolid* blob = nullptr;
    // Open patch block: its subdivision settings and control points so far.
    bool inPatch = false;
    BezierPatchSubdivision patchSubdivision = BezierPatchSubdivision::UniformGrid;
    int patchUSteps = 0;
    int patchVSteps = 0;
    double patchFlatness = 0;
    int patchPointCount = 0;
    Vector3Dd patchPoints[4][4];
    // Pending placement of the next primitive or outermost csg block.
    bool hasPlacement = false;
    Matrix4x4d placement;
    auto fail = [&](const char* message) {
        fclose(file);
        delete currentMaterial;
        delete csgBody;
        delete polynomial;
        delete blob;
        throw VSDKFatalException(message);
    };
    auto place = [&](SimpleBody* body) {
        if (!hasPlacement) return true;
        hasPlacement = false;
        const Matrix4x4d forward = placement.multiply(body->getAffinePlacement());
        return body->setAffinePlacement(forward, forward.inverse()) == AffinePlacementStatus::Valid;
    };
    auto attach = [&](SimpleBody* body) {
        if (!place(body)) { delete body; fail("ReaderMitScene: transform is not an invertible affine map"); }
        if (csgBody == nullptr) { outScene->addBody(body); return; }
        if (!csgBody->addLeaf((long int)csgNodes[csgNodes.size()-1], body))
            fail("ReaderMitScene: invalid csg operand");
    };

    char lineBuf[1024];
    while ( fgets(lineBuf, sizeof(lineBuf), file) ) {
        // Remove comment
        char* sharp = strchr(lineBuf, '#');
        if ( sharp ) {
            *sharp = '\0';
        }
        java::String javaLine(lineBuf);
        javaLine = trimLine(javaLine);
        if ( javaLine.isEmpty() ) {
            continue;
        }

        char cmdStr[256] = {0};
        sscanf(javaLine.toCString(), "%255s", cmdStr);
        java::String cmd(cmdStr);

        if ( polynomial != nullptr && cmd != "term" && cmd != "end" )
            fail("ReaderMitScene: poly blocks only hold term lines");
        if ( blob != nullptr && cmd != "component" && cmd != "end" )
            fail("ReaderMitScene: blob blocks only hold component lines");
        if ( inPatch && cmd != "control" && cmd != "end" )
            fail("ReaderMitScene: patch blocks only hold control lines");
        if ( cmd == "transform" ) {
            double m[12];
            if (sscanf(javaLine.toCString(), "%*s %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf",
                    &m[0], &m[1], &m[2], &m[3], &m[4], &m[5], &m[6], &m[7], &m[8], &m[9],
                    &m[10], &m[11]) != 12) fail("ReaderMitScene: transform requires twelve numbers");
            for (int i = 0; i < 12; ++i)
                if (!java::Double::isFinite(m[i])) fail("ReaderMitScene: transform requires finite numbers");
            if (hasPlacement) fail("ReaderMitScene: transform must be followed by a primitive");
            const double rows[4][4] = {{m[0], m[1], m[2], m[3]}, {m[4], m[5], m[6], m[7]},
                {m[8], m[9], m[10], m[11]}, {0, 0, 0, 1}};
            placement = Matrix4x4d(rows);
            hasPlacement = true;
        }
        else if ( cmd == "poly" ) {
            int order = 0;
            char option[256] = {0};
            const int fields = sscanf(javaLine.toCString(), "%*s %d %255s", &order, option);
            if (fields < 1 || !PolynomialSurface::isValidOrder(order) ||
                (fields == 2 && java::String(option) != "sturm"))
                fail("ReaderMitScene: poly requires an order from 2 to 7 and optionally sturm");
            polynomial = new PolynomialSurface(order, fields == 2);
        }
        else if ( cmd == "term" && polynomial != nullptr ) {
            double coefficient;
            int x, y, z;
            if (sscanf(javaLine.toCString(), "%*s %lf %d %d %d", &coefficient, &x, &y, &z) != 4 ||
                !java::Double::isFinite(coefficient))
                fail("ReaderMitScene: term requires a coefficient and three powers");
            const int index = PolynomialSurface::getTermIndex(polynomial->getOrder(), x, y, z);
            if (index < 0) fail("ReaderMitScene: term powers exceed the polynomial order");
            polynomial->getCoefficients()[index] += coefficient;
        }
        else if ( cmd == "end" && polynomial != nullptr ) {
            SimpleBody* body = new SimpleBody();
            body->setGeometry(polynomial);
            polynomial = nullptr;
            body->setMaterial(new SimpleMaterial(*currentMaterial));
            attach(body);
        }
        else if ( cmd == "blob" ) {
            double threshold;
            char option[256] = {0};
            const int fields = sscanf(javaLine.toCString(), "%*s %lf %255s", &threshold, option);
            if (fields < 1 || !java::Double::isFinite(threshold) ||
                (fields == 2 && java::String(option) != "sturm"))
                fail("ReaderMitScene: blob requires a threshold and optionally sturm");
            blob = new BlobSolid(threshold, fields == 2);
        }
        else if ( cmd == "component" && blob != nullptr ) {
            double strength, radius, x, y, z;
            if (sscanf(javaLine.toCString(), "%*s %lf %lf %lf %lf %lf", &strength, &radius, &x, &y, &z) != 5)
                fail("ReaderMitScene: component requires strength, radius and center");
            const BlobComponent component(Vector3Dd(x, y, z), radius, strength);
            if (component.isDegenerate()) fail("ReaderMitScene: degenerate blob component");
            blob->addComponent(component);
        }
        else if ( cmd == "end" && blob != nullptr ) {
            if (blob->getComponentCount() == 0) fail("ReaderMitScene: blob requires a component");
            SimpleBody* body = new SimpleBody();
            body->setGeometry(blob);
            blob = nullptr;
            body->setMaterial(new SimpleMaterial(*currentMaterial));
            attach(body);
        }
        else if ( cmd == "patch" ) {
            if (csgBody) fail("ReaderMitScene: patches are open surfaces and cannot be csg operands");
            char method[256] = {0};
            char option[256] = {0};
            int uSteps = 0, vSteps = 0;
            double flatnessValue = 0.1;
            const int fields = sscanf(javaLine.toCString(), "%*s %255s %d %d %255s %lf", method, &uSteps,
                &vSteps, option, &flatnessValue);
            const java::String name(method);
            if (name == "uniform") patchSubdivision = BezierPatchSubdivision::UniformGrid;
            else if (name == "grid") patchSubdivision = BezierPatchSubdivision::PrecomputedGrid;
            else if (name == "adaptive") patchSubdivision = BezierPatchSubdivision::Adaptive;
            else if (name == "tree") patchSubdivision = BezierPatchSubdivision::AdaptiveTree;
            else if (name == "smooth") patchSubdivision = BezierPatchSubdivision::SmoothGrid;
            else fail("ReaderMitScene: patch requires uniform, grid, adaptive, tree or smooth");
            if (!(fields == 3 || (fields == 5 && java::String(option) == "flatness" &&
                    java::Double::isFinite(flatnessValue) && flatnessValue >= 0)) ||
                uSteps < 1 || vSteps < 1 || uSteps > 64 || vSteps > 64)
                fail("ReaderMitScene: patch requires steps from 1 to 64 and optionally flatness <value>");
            inPatch = true;
            patchUSteps = uSteps;
            patchVSteps = vSteps;
            patchFlatness = flatnessValue;
            patchPointCount = 0;
        }
        else if ( cmd == "control" && inPatch ) {
            double x, y, z;
            if (sscanf(javaLine.toCString(), "%*s %lf %lf %lf", &x, &y, &z) != 3 ||
                !java::Double::isFinite(x) || !java::Double::isFinite(y) || !java::Double::isFinite(z))
                fail("ReaderMitScene: control requires three finite coordinates");
            if (patchPointCount >= 16) fail("ReaderMitScene: patch takes 16 control points");
            patchPoints[patchPointCount / 4][patchPointCount % 4] = Vector3Dd(x, y, z);
            patchPointCount++;
        }
        else if ( cmd == "end" && inPatch ) {
            if (patchPointCount != 16) fail("ReaderMitScene: patch takes 16 control points");
            inPatch = false;
            SimpleBody* body = new SimpleBody();
            body->setGeometry(new SubdividedBezierPatch(patchSubdivision, patchUSteps, patchVSteps,
                patchFlatness, patchPoints));
            body->setMaterial(new SimpleMaterial(*currentMaterial));
            attach(body);
        }
        else if ( cmd == "heightfield" ) {
            char path[512] = {0};
            char option[256] = {0};
            double water = 0;
            const int fields = sscanf(javaLine.toCString(), "%*s %511s %255s %lf", path, option, &water);
            if (fields != 1 && !(fields == 3 && java::String(option) == "water" && java::Double::isFinite(water)))
                fail("ReaderMitScene: heightfield requires a file and optionally water <level>");
            // Relative files are resolved from the scene file's directory.
            java::String resolved(path);
            const char* slash = strrchr(fileName, '/');
            if (path[0] != '/' && slash != nullptr)
                resolved = java::String(fileName).substr(0, (size_t)(slash - fileName) + 1) + resolved;
            HeightFieldSolid* heightField = nullptr;
            try {
                heightField = ReaderAltimetry::importHeightField(resolved.toCString());
            }
            catch (const VSDKFatalException&) {
                fail("ReaderMitScene: could not read the heightfield altimetry file");
            }
            if (fields == 3) {
                Vector3Dd minimum = heightField->getMinimum().withY(water);
                Vector3Dd maximum = heightField->getMaximum();
                if (!(water < maximum.y())) {
                    delete heightField;
                    fail("ReaderMitScene: heightfield water level must lie below its highest point");
                }
                heightField->setBounds(minimum, maximum);
                heightField->updateBlocks();
            }
            SimpleBody* body = new SimpleBody();
            body->setGeometry(heightField);
            body->setMaterial(new SimpleMaterial(*currentMaterial));
            attach(body);
        }
        else if ( cmd == "triangles" ) {
            if (csgBody) fail("ReaderMitScene: triangles cannot be csg operands");
            if (hasPlacement) fail("ReaderMitScene: triangles cannot be transformed");
            if (inTriangles) {
                fclose(file);
                delete currentMaterial;
                throw VSDKFatalException("ReaderMitScene: nested triangles block");
            }
            triangleGroup.reset();
            inTriangles = true;
        }
        else if ( cmd == "v" && inTriangles ) {
            double x, y, z;
            if (sscanf(javaLine.toCString(), "%*s %lf %lf %lf", &x, &y, &z) != 3 ||
                !triangleGroup.addVertex(Vector3Dd(x,y,z))) {
                fclose(file);
                delete currentMaterial;
                throw VSDKFatalException("ReaderMitScene: invalid triangle vertex");
            }
        }
        else if ( cmd == "f" && inTriangles ) {
            if (!triangleGroup.addFace(javaLine.toCString() + 1)) {
                fclose(file);
                delete currentMaterial;
                throw VSDKFatalException("ReaderMitScene: invalid triangle face");
            }
        }
        else if ( cmd == "end" && inTriangles ) {
            triangleGroup.flush(outScene, currentMaterial);
            triangleGroup.reset();
            inTriangles = false;
        }
        else if ( cmd == "csg" ) {
            char operationStr[256] = {0};
            char evaluatorStr[256] = {0};
            const int fields = sscanf(javaLine.toCString(), "%*s %255s %255s", operationStr, evaluatorStr);
            java::String operationName(operationStr);
            java::String evaluatorName(evaluatorStr);
            CsgOperation operation = CsgOperation::UNION;
            if ( operationName == "union" ) operation = CsgOperation::UNION;
            else if ( operationName == "intersection" ) operation = CsgOperation::INTERSECTION;
            else if ( operationName == "difference" ) operation = CsgOperation::DIFFERENCE;
            else fail("ReaderMitScene: csg requires union, intersection or difference");
            if ( csgBody == nullptr ) {
                CsgBodyEvaluator evaluator = CsgBodyEvaluator::ROTH;
                if ( fields == 2 && evaluatorName == "pov" ) evaluator = CsgBodyEvaluator::CROSSING_MEMBERSHIP;
                else if ( fields == 2 && evaluatorName != "roth" ) fail("ReaderMitScene: csg evaluator must be roth or pov");
                csgBody = new CsgBody(operation, evaluator);
                csgBody->setMaterial(new SimpleMaterial(*currentMaterial));
                if (!place(csgBody)) fail("ReaderMitScene: transform is not an invertible affine map");
                csgNodes.add(0);
            }
            else {
                if ( fields == 2 ) fail("ReaderMitScene: only the outermost csg selects an evaluator");
                if ( hasPlacement ) fail("ReaderMitScene: nested csg blocks cannot be transformed");
                const long int node = csgBody->addNode((long int)csgNodes[csgNodes.size()-1], operation);
                if ( node < 0 ) fail("ReaderMitScene: invalid csg node");
                csgNodes.add(node);
            }
        }
        else if ( cmd == "end" && csgBody ) {
            csgNodes.remove(csgNodes.size()-1);
            if ( csgNodes.size() == 0 ) {
                outScene->addBody(csgBody);
                csgBody = nullptr;
            }
        }
        else if ( cmd == "viewport" ) {
            int w, h;
            if ( sscanf(javaLine.toCString(), "%*s %d %d", &w, &h) == 2 ) {
                viewportXSize = w;
                viewportYSize = h;
                hasViewport = true;
            }
        }
        else if ( cmd == "eye" ) {
            double x, y, z;
            if ( sscanf(javaLine.toCString(), "%*s %lf %lf %lf", &x, &y, &z) == 3 ) {
                eye = Vector3Dd(x, y, z);
                hasEye = true;
            }
        }
        else if ( cmd == "up" ) {
            double x, y, z;
            if ( sscanf(javaLine.toCString(), "%*s %lf %lf %lf", &x, &y, &z) == 3 ) {
                up = Vector3Dd(x, y, z);
                hasUp = true;
            }
        }
        else if ( cmd == "lookat" ) {
            double x, y, z;
            if ( sscanf(javaLine.toCString(), "%*s %lf %lf %lf", &x, &y, &z) == 3 ) {
                lookat = Vector3Dd(x, y, z);
                hasLookat = true;
            }
        }
        else if ( cmd == "fov" ) {
            double fov;
            if ( sscanf(javaLine.toCString(), "%*s %lf", &fov) == 1 ) {
                importedHorizontalFov = fov;
            }
        }
        else if ( cmd == "background" ) {
            double r, g, b;
            if ( sscanf(javaLine.toCString(), "%*s %lf %lf %lf", &r, &g, &b) == 3 ) {
                background->setColor(r, g, b);
            }
        }
        else if ( cmd == "surface" ) {
            if (inTriangles) triangleGroup.flush(outScene, currentMaterial);
            double r, g, b;
            double ka, kd, ks;
            double ns;
            double kr, kt, index;
            if ( sscanf(javaLine.toCString(), "%*s %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf",
                        &r, &g, &b, &ka, &kd, &ks, &ns, &kr, &kt, &index) == 10 ) {
                delete currentMaterial;
                currentMaterial = new SimpleMaterial(
                    "surface",
                    ColorRgb(r * ka, g * ka, b * ka),
                    ColorRgb(r * kd, g * kd, b * kd),
                    ColorRgb(ks, ks, ks),
                    true,
                    kr,
                    kt,
                    1.0,
                    ns);
            }
        }
        else if ( cmd == "box" ) {
            double xmin, ymin, zmin, xmax, ymax, zmax;
            if (sscanf(javaLine.toCString(), "%*s %lf %lf %lf %lf %lf %lf",
                    &xmin, &ymin, &zmin, &xmax, &ymax, &zmax) != 6 ||
                !java::Double::isFinite(xmin) || !java::Double::isFinite(ymin) || !java::Double::isFinite(zmin) ||
                !java::Double::isFinite(xmax) || !java::Double::isFinite(ymax) || !java::Double::isFinite(zmax) ||
                xmin >= xmax || ymin >= ymax || zmin >= zmax) {
                delete currentMaterial; fclose(file);
                throw VSDKFatalException("ReaderMitScene: box requires ordered minimum and maximum corners");
            }
            SimpleBody* b = new SimpleBody();
            b->setGeometry(new Box(Vector3Dd(xmin,ymin,zmin), Vector3Dd(xmax,ymax,zmax)));
            b->setMaterial(new SimpleMaterial(*currentMaterial));
            attach(b);
        }
        else if ( cmd == "halfspace" ) {
            double a, b, c, d;
            if (sscanf(javaLine.toCString(), "%*s %lf %lf %lf %lf", &a, &b, &c, &d) != 4 ||
                !java::Double::isFinite(a) || !java::Double::isFinite(b) ||
                !java::Double::isFinite(c) || !java::Double::isFinite(d) ||
                (a == 0 && b == 0 && c == 0)) {
                delete currentMaterial; fclose(file);
                throw VSDKFatalException("ReaderMitScene: halfspace requires a nonzero normal and constant");
            }
            SimpleBody* body = new SimpleBody();
            body->setGeometry(new PlanarHalfSpace(a,b,c,d));
            body->setMaterial(new SimpleMaterial(*currentMaterial));
            attach(body);
        }
        else if ( cmd == "quadric" ) {
            double xx, yy, zz, xy, xz, yz, x, y, z, c;
            if (sscanf(javaLine.toCString(), "%*s %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf",
                    &xx, &yy, &zz, &xy, &xz, &yz, &x, &y, &z, &c) != 10) {
                delete currentMaterial;
                fclose(file);
                throw VSDKFatalException("ReaderMitScene: quadric requires ten coefficients");
            }
            SimpleBody* b = new SimpleBody();
            b->setGeometry(new QuadricSurface(Vector3Dd(xx,yy,zz), Vector3Dd(xy,xz,yz),
                Vector3Dd(x,y,z), c));
            b->setMaterial(new SimpleMaterial(*currentMaterial));
            attach(b);
        }
        else if ( cmd == "sphere" ) {
            double x, y, z, r;
            if ( sscanf(javaLine.toCString(), "%*s %lf %lf %lf %lf", &x, &y, &z, &r) == 4 ) {
                SimpleBody* b = new SimpleBody();
                b->setGeometry(new Sphere(r));
                b->setPosition(Vector3Dd(x, y, z));
                b->setMaterial(new SimpleMaterial(*currentMaterial));
                attach(b);
            }
        }
        else if ( cmd == "light" ) {
            double r, g, b;
            char typeStr[256] = {0};
            if ( sscanf(javaLine.toCString(), "%*s %lf %lf %lf %255s", &r, &g, &b, typeStr) == 4 ) {
                java::String type(typeStr);
                if ( type == "ambient" ) {
                    outScene->addLight(new AmbientLight(ColorRgb(r,g,b)));
                }
                else if ( type == "point" ) {
                    double x, y, z;
                    if ( sscanf(javaLine.toCString(), "%*s %*f %*f %*f %*s %lf %lf %lf", &x, &y, &z) == 3 ) {
                        outScene->addLight(new PointLight(Vector3Dd(x,y,z), ColorRgb(r,g,b)));
                    }
                }
                else if ( type == "directional" ) {
                    double x, y, z;
                    if ( sscanf(javaLine.toCString(), "%*s %*f %*f %*f %*s %lf %lf %lf", &x, &y, &z) == 3 ) {
                        outScene->addLight(new DirectionalLight(Vector3Dd(x,y,z), ColorRgb(r,g,b)));
                    }
                }
            }
        }
    }

    if (polynomial) fail("ReaderMitScene: unterminated poly block");
    if (blob) fail("ReaderMitScene: unterminated blob block");
    if (inPatch) fail("ReaderMitScene: unterminated patch block");
    if (hasPlacement) fail("ReaderMitScene: transform must be followed by a primitive");
    if (csgBody) fail("ReaderMitScene: unterminated csg block");
    fclose(file);
    if (inTriangles) {
        delete currentMaterial;
        throw VSDKFatalException("ReaderMitScene: unterminated triangles block");
    }

    if ( hasEye ) {
        camera->setPosition(eye);
    }
    camera->setUpDirect(up);
    if ( hasLookat ) {
        camera->setFocusedPositionMaintainingOrthogonality(lookat);
    }

    if ( viewportXSize > 0 && viewportYSize > 0 ) {
        double aspect = ((double)viewportXSize) / ((double)viewportYSize);
        double horizontalHalfAngle = importedHorizontalFov * M_PI / 360.0;
        double verticalHalfAngle = std::atan(std::tan(horizontalHalfAngle) / aspect);
        double verticalFov = (verticalHalfAngle * 360.0) / M_PI;
        camera->setFov(verticalFov);
    }
    if ( hasViewport ) {
        camera->updateViewportResize(viewportXSize, viewportYSize);
    }
    else {
        camera->updateVectors();
    }

    delete currentMaterial;
}
