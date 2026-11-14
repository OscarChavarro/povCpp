cmake_minimum_required(VERSION 3.16)
project(vitral_base_cpp LANGUAGES CXX)
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(VITRAL_BASE_ISOLATED_OUTPUT "Keep base artifacts inside the caller's build tree" OFF)
if(NOT VITRAL_BASE_ISOLATED_OUTPUT)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/build")
endif()

# Optional image format support
option(WITH_JPEG "Enable JPEG image format support" OFF)
option(WITH_PNG "Enable PNG image format support" ON)
option(WITH_FREETYPE "Enable TrueType font reading through FreeType 2" ON)
option(WITH_POSIX_THREADS "Enable POSIX thread based java.util.concurrent emulation" ON)

# Try to find JPEG library if requested
if(WITH_JPEG)
    find_package(JPEG)
    if(NOT JPEG_FOUND)
        message(WARNING "JPEG library not found. JPEG support will be disabled.")
        set(WITH_JPEG OFF)
    endif()
endif()

# Try to find PNG library if requested
if(WITH_PNG)
    find_package(PNG)
    if(NOT PNG_FOUND)
        message(WARNING "PNG library not found. PNG support will be disabled.")
        set(WITH_PNG OFF)
    endif()
endif()

# Try to find FreeType 2 library if requested
if(WITH_FREETYPE)
    find_package(Freetype)
    if(NOT FREETYPE_FOUND)
        message(WARNING "FreeType 2 library not found. Font reading will be disabled.")
        set(WITH_FREETYPE OFF)
    endif()
endif()

# Print configuration summary
message(STATUS "Vitral C++ Base Configuration:")
message(STATUS "  JPEG Support: ${WITH_JPEG}")
message(STATUS "  PNG Support:  ${WITH_PNG}")
message(STATUS "  FreeType Support: ${WITH_FREETYPE}")
message(STATUS "  POSIX Threads: ${WITH_POSIX_THREADS}")

# Source files for the base library
set(VITRAL_BASE_SOURCES
    src/main/vsdk/toolkit/common/numerics/FloatingInterval.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/QuadraticInteriorBound.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/AffineEllipsoidMinimumDistance.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/AffineEllipsoidDualBound.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/AffineEllipsoidDistanceBounds.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/AffineEllipsoidDistance.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/OrthogonalEllipsoidDistance.cpp
    src/main/vsdk/toolkit/environment/scene/AffinePlacement.cpp
    src/main/vsdk/toolkit/environment/scene/InstanceRayCrossingBuffer.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgExpression.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgExpressionEquivalence.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgOperandView.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgBody.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgBodyOperand.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgExactArithmetic.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgDyadic.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgDyadicTransform.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgDyadicSolid.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgExactPlane.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgExactTransform.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgExactQuadric.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgSolidEquivalence.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgTree.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgPointClassification.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgSolidBounds.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgPointWitness.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgPointAttribution.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgPointCandidate.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgPointEvidence.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgPointProof.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgRayLeaf.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgRayPlacement.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgRayLeafBatch.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgRayLeafQuery.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgRayEventReference.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgRayEventGroup.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgRayEventBatch.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgRayEventQuery.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgRayEventCursor.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgRayWorkspace.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgVersionStamp.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgRayClassification.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgRothRayClassifier.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgRaySurfaceEvents.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgCrossingMembershipFilter.cpp
    src/main/vsdk/toolkit/environment/scene/AffineToleranceMap.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgPlacedExpression.cpp
    src/main/vsdk/toolkit/environment/geometry/element/GeometryBounds.cpp
    src/main/vsdk/toolkit/environment/geometry/element/GeometryQueryWorkspace.cpp
    src/main/vsdk/toolkit/environment/geometry/element/RayInstanceStep.cpp
    src/main/vsdk/toolkit/environment/geometry/element/RayInstancePath.cpp
    src/main/vsdk/toolkit/environment/geometry/element/RayBoundary.cpp
    src/main/vsdk/toolkit/environment/geometry/element/RaySolidBoundary.cpp
    src/main/vsdk/toolkit/environment/geometry/element/RaySolidClassification.cpp
    src/main/vsdk/toolkit/environment/geometry/element/RayCrossingQuery.cpp
    src/main/vsdk/toolkit/environment/geometry/element/RayCrossing.cpp
    src/main/vsdk/toolkit/environment/geometry/element/RayCrossingSink.cpp
    src/main/vsdk/toolkit/render/dithering/Dithering.cpp
    src/main/vsdk/toolkit/common/VSDK.cpp
    src/main/vsdk/toolkit/common/Entity.cpp
    src/main/vsdk/toolkit/common/EntityControlAccessor.cpp
    src/main/vsdk/toolkit/gui/editor/ControlSpecification.cpp
    src/main/vsdk/toolkit/gui/editor/GenericEditor.cpp
    src/main/vsdk/toolkit/common/logging/Logger.cpp
    src/main/vsdk/toolkit/common/statistics/RaytraceStatistics.cpp
    src/main/vsdk/toolkit/common/statistics/SolidTextureStatistics.cpp
    src/main/vsdk/toolkit/common/statistics/GeometryStatistics.cpp
    src/main/vsdk/toolkit/common/color/ColorRgb.cpp
    src/main/vsdk/toolkit/common/linealAlgebra/Complex.cpp
    src/main/vsdk/toolkit/common/linealAlgebra/MatrixNxM.cpp
    src/main/vsdk/toolkit/common/linealAlgebra/Matrix4x4d.cpp
    src/main/vsdk/toolkit/common/linealAlgebra/Matrix4x4f.cpp
    src/main/vsdk/toolkit/common/linealAlgebra/AlgebraPrimitives.cpp
    src/main/vsdk/toolkit/common/linealAlgebra/processing/LinearAlgebraEngine.cpp
    src/main/vsdk/toolkit/processing/ComputationalGeometry.cpp
    src/main/vsdk/toolkit/numericalAnalysis/lookUpTables/LookUpTableChecksum16.cpp
    src/main/vsdk/toolkit/numericalAnalysis/lookUpTables/LookUpTableSine.cpp
    src/main/vsdk/toolkit/numericalAnalysis/polynomial/PolynomialSolver.cpp
    src/main/vsdk/toolkit/numericalAnalysis/polynomial/CubicSolver.cpp
    src/main/vsdk/toolkit/numericalAnalysis/polynomial/QuadraticSolver.cpp
    src/main/vsdk/toolkit/numericalAnalysis/polynomial/QuarticSolver.cpp
    src/main/java/lang/Object.cpp
    src/main/java/lang/String.cpp
    src/main/java/lang/System.cpp
    src/main/java/lang/Character.cpp
    src/main/java/lang/Float.cpp
    src/main/java/lang/Double.cpp
    src/main/java/lang/StringBuilder.cpp
    src/main/java/lang/ProcessBuilder.cpp
    src/main/java/lang/Thread.cpp
    src/main/java/io/InputStream.cpp
    src/main/java/io/File.cpp
    src/main/java/io/FileInputStream.cpp
    src/main/java/io/FileOutputStream.cpp
    src/main/java/io/BufferedInputStream.cpp
    src/main/java/io/BufferedOutputStream.cpp
    src/main/java/io/PrintStream.cpp
    src/main/java/io/BufferedReader.cpp
    src/main/java/io/PrintWriter.cpp
    src/main/java/io/Reader.cpp
    src/main/java/io/InputStreamReader.cpp
    src/main/java/io/FileReader.cpp
    src/main/java/nio/file/Files.cpp
    src/main/java/io/StreamTokenizer.cpp
    src/main/java/net/ServerSocket.cpp
    src/main/java/net/SocketInputStream.cpp
    src/main/java/net/SocketOutputStream.cpp
    src/main/java/net/Socket.cpp
    src/main/java/net/WebSocket.cpp
    src/main/java/util/Formatter.cpp
    src/main/java/util/StringTokenizer.cpp
    src/main/java/util/regex/Pattern.cpp
    src/main/java/util/concurrent/Executors.cpp
    src/main/vsdk/toolkit/gui/KeyEvent.cpp
    src/main/vsdk/toolkit/gui/gizmo/LightGizmoOmniBillboard.cpp
    src/main/vsdk/toolkit/gui/gizmo/InfinitePlaneGizmo.cpp
    src/main/vsdk/toolkit/gui/gizmo/RayGizmo.cpp
    src/main/vsdk/toolkit/gui/gizmo/RayGizmoInteractionTechniques.cpp
    src/main/vsdk/toolkit/gui/tangibleInterfaces/TangibleInterfaceEvent.cpp
    src/main/vsdk/toolkit/gui/tangibleInterfaces/TangibleInterfaceEvent2InfinitePlaneGizmoMapper.cpp
    src/main/vsdk/toolkit/gui/tangibleInterfaces/TangibleInterfaceEvent2RayGizmoMapper.cpp
    src/main/vsdk/toolkit/gui/tangibleInterfaces/TangibleInterfaceNetworkClient.cpp
    src/main/vsdk/toolkit/gui/RendererConfigurationController.cpp
    src/main/vsdk/toolkit/gui/feedback/ProgressMonitorConsole.cpp
    src/main/vsdk/toolkit/gui/feedback/ProgressMonitorConsoleLongFormat.cpp
    src/main/vsdk/toolkit/gui/feedback/ProgressMonitorInRam.cpp
    src/main/vsdk/toolkit/gui/feedback/parallel/ParallelProgressMonitorProducer.cpp
    src/main/vsdk/toolkit/gui/feedback/parallel/ParallelProgressMonitorConsumer.cpp
    src/main/vsdk/toolkit/environment/camera/Camera.cpp
    src/main/vsdk/toolkit/environment/background/SimpleBackground.cpp
    src/main/vsdk/toolkit/environment/light/Light.cpp
    src/main/vsdk/toolkit/environment/light/AmbientLight.cpp
    src/main/vsdk/toolkit/environment/light/DirectionalLight.cpp
    src/main/vsdk/toolkit/environment/light/PointLight.cpp
    src/main/vsdk/toolkit/environment/light/SpotLight.cpp
    src/main/vsdk/toolkit/environment/scene/SimpleScene.cpp
    src/main/vsdk/toolkit/environment/scene/SimpleSceneSnapshot.cpp
    src/main/vsdk/toolkit/environment/scene/SimpleBodyGroup.cpp
    src/main/vsdk/toolkit/environment/scene/SimpleBody.cpp
    src/main/vsdk/toolkit/processing/StopWatch.cpp
    src/main/vsdk/toolkit/common/symbolicAlgebra/AlgebraicExpression.cpp
    src/main/vsdk/toolkit/environment/geometry/element/Ray.cpp
    src/main/vsdk/toolkit/environment/geometry/element/RayHit.cpp
    src/main/vsdk/toolkit/environment/geometry/element/Triangle.cpp
    src/main/vsdk/toolkit/environment/geometry/element/Vertex.cpp
    src/main/vsdk/toolkit/environment/geometry/element/Vertex2D.cpp
    src/main/vsdk/toolkit/environment/geometry/curve/Curve.cpp
    src/main/vsdk/toolkit/environment/geometry/curve/ParametricCurve.cpp
    src/main/vsdk/toolkit/environment/geometry/Geometry.cpp
    src/main/vsdk/toolkit/environment/geometry/surface/Surface.cpp
    src/main/vsdk/toolkit/environment/geometry/surface/HalfSpace.cpp
    src/main/vsdk/toolkit/environment/geometry/surface/InfinitePlane.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/PlanarHalfSpace.cpp
    src/main/vsdk/toolkit/environment/geometry/surface/FunctionalExplicitSurface.cpp
    src/main/vsdk/toolkit/environment/geometry/surface/Md2Mesh.cpp
    src/main/vsdk/toolkit/environment/geometry/surface/ParametricBiCubicPatch.cpp
    src/main/vsdk/toolkit/environment/geometry/surface/_ParametricBiCubicPatchIntersector.cpp
    src/main/vsdk/toolkit/environment/geometry/surface/TriangleStripMesh.cpp
    src/main/vsdk/toolkit/environment/geometry/surface/TriangleMesh.cpp
    src/main/vsdk/toolkit/environment/geometry/surface/SubdividedBezierPatch.cpp
    src/main/vsdk/toolkit/environment/geometry/surface/TriangleMeshIntersectionIndex.cpp
    src/main/vsdk/toolkit/environment/geometry/surface/QuadMesh.cpp
    src/main/vsdk/toolkit/environment/geometry/surface/TriangleMeshGroup.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/Voxelization.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/TriangleMeshVoxelization.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/TriangleMeshGroupVoxelization.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/FunctionalExplicitSurfaceVoxelization.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/SurfaceRayIntersection.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/GeometryTriangulator.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/MonotoneDecompositionTriangulator.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_ContourAwarePolygonTriangulator.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_Construct.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_IncrementalSegmentInserter.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_InsertionBatchSchedule.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_Monotone.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_RandomSegmentOrder.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_SegmentTableBuilder.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_TriangulationTrapezoid.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_TriangulationTrapezoidQueryNode.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polygonClipper/_Polygon2DWA.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polygonClipper/PolygonProcessor.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polygonClipper/PolygonTopologicalMerger.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polygonClipper/WeilerAthertonPolygonClipper.cpp
    src/main/vsdk/toolkit/environment/geometry/surface/polygon/_Polygon2DContour.cpp
    src/main/vsdk/toolkit/environment/geometry/surface/polygon/Polygon2D.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/Volume.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/Solid.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/Sphere.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/Box.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/Cone.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/Arrow.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/QuadricSurface.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/ImplicitSolid.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/PolynomialSurface.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/BlobComponent.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/BlobSolid.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/HeightFieldSolid.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/Torus.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/VoxelVolume.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidNumericPolicy.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidPredicates.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidGeometricValidator.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidValidationEngine.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidTopologyEditing.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/_PolyhedralBoundedSolidTopologicalValidator.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/_PolyhedralBoundedSolidBooleanTopologyPredicates.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/_PolyhedralBoundedSolidValidationStrategies.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidTopologySummary.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidNodesToString.cpp
    src/main/vsdk/toolkit/gui/CameraControllerAquynza.cpp
    src/main/vsdk/toolkit/gui/CameraControllerOrbiter.cpp
    src/main/vsdk/toolkit/gui/CameraControllerBlender.cpp
    src/main/vsdk/toolkit/gui/CameraControllerGoogleEarth.cpp
    src/main/vsdk/toolkit/render/TraceWorkspace.cpp
    src/main/vsdk/toolkit/render/SpherePolyhedralCache.cpp
    src/main/vsdk/toolkit/render/raytracing/DepthBufferEncoder.cpp
    src/main/vsdk/toolkit/render/raytracing/ParallelRaytracer.cpp
    src/main/vsdk/toolkit/render/raytracing/RasterTileArea.cpp
    src/main/vsdk/toolkit/render/raytracing/RasterTileGenerator.cpp
    src/main/vsdk/toolkit/render/raytracing/SimpleRaytracer.cpp
    src/main/vsdk/toolkit/render/AutoStereogramGenerator.cpp
    src/main/vsdk/toolkit/render/hiddenLine/HiddenLineRenderer.cpp
    src/main/vsdk/toolkit/render/raster/Rasterizer2D.cpp
    src/main/vsdk/toolkit/render/hiddenLine/WireframeRenderer.cpp
    src/main/vsdk/toolkit/render/PolyhedralBoundedSolidDebugger.cpp
    src/main/vsdk/toolkit/render/shaders/CpuTextureSamplingConfig.cpp
    src/main/vsdk/toolkit/render/shaders/LightingShader.cpp
    src/main/vsdk/toolkit/render/shaders/ConstantShader.cpp
    src/main/vsdk/toolkit/render/shaders/ConstantTextureShader.cpp
    src/main/vsdk/toolkit/render/shaders/CookTorranceShader.cpp
    src/main/vsdk/toolkit/render/shaders/ShaderSelector.cpp
    src/main/vsdk/toolkit/io/PersistenceElement.cpp
    src/main/vsdk/toolkit/io/FileLocator.cpp
    src/main/vsdk/toolkit/io/gui/DialogPersistence.cpp
    src/main/vsdk/toolkit/io/geometry/EnvironmentPersistence.cpp
    src/main/vsdk/toolkit/io/geometry/WriterObj.cpp
    src/main/vsdk/toolkit/io/geometry/WriterGts.cpp
    src/main/vsdk/toolkit/io/geometry/WriterVtk.cpp
    src/main/vsdk/toolkit/io/geometry/ReaderObj.cpp
    src/main/vsdk/toolkit/io/geometry/ReaderPly.cpp
    src/main/vsdk/toolkit/io/geometry/ReaderPlyResult.cpp
    src/main/vsdk/toolkit/io/geometry/ReaderAltimetry.cpp
    src/main/vsdk/toolkit/io/geometry/ReaderMitScene.cpp
    src/main/vsdk/toolkit/io/geometry/ReaderMitTriangleGroup.cpp
    src/main/vsdk/toolkit/io/image/ImagePersistence.cpp
    src/main/java/io/BufferedOutputStream.cpp
    src/main/vsdk/toolkit/media/MediaEntity.h
    src/main/vsdk/toolkit/media/solidTexture/TextureUtils.cpp
    src/main/vsdk/toolkit/media/solidTexture/from2d/ControlledRGBAImageHDRUncompressed.cpp
    src/main/vsdk/toolkit/media/solidTexture/from2d/ImageTexture.cpp
    src/main/vsdk/toolkit/media/solidTexture/from2d/SolidTextureCoordinateMapper.cpp
    src/main/vsdk/toolkit/media/solidTexture/procedural/BumpTextureFixture.cpp
    src/main/vsdk/toolkit/media/solidTexture/procedural/ColorTextureFixture.cpp
    src/main/vsdk/toolkit/media/solidTexture/procedural/ProceduralNoise.cpp
    src/main/vsdk/toolkit/media/RGBPixel.cpp
    src/main/vsdk/toolkit/media/RGBAPixel.cpp
    src/main/vsdk/toolkit/media/ShapeDescriptor.cpp
    src/main/vsdk/toolkit/media/RGBColorPalette.cpp
    src/main/vsdk/toolkit/media/RGBAColorPalette.cpp
    src/main/vsdk/toolkit/media/RGBProceduralColorPalette.cpp
    src/main/vsdk/toolkit/media/GrayScalePalette.cpp
    src/main/vsdk/toolkit/media/Image.cpp
    src/main/vsdk/toolkit/media/RGBImageUncompressed.cpp
    src/main/vsdk/toolkit/media/RGBAImageUncompressed.cpp
    src/main/vsdk/toolkit/media/RGBAImageHDRUncompressed.cpp
    src/main/vsdk/toolkit/media/IndexedColorImageUncompressed.cpp
    src/main/vsdk/toolkit/media/IndexedColorImageHDRUncompressed.cpp
    src/main/vsdk/toolkit/media/RGBAImageCompressed.cpp
    src/main/vsdk/toolkit/media/ZBuffer.cpp
    src/main/vsdk/toolkit/media/NormalMap.cpp
    src/main/vsdk/toolkit/media/Calligraphic2DBuffer.cpp
    src/main/vsdk/toolkit/media/PrimitiveCountShapeDescriptor.cpp
    src/main/vsdk/toolkit/media/FourierShapeDescriptor.cpp
    src/main/vsdk/toolkit/media/GeometryMetadata.cpp
    src/main/vsdk/toolkit/environment/material/SimpleMaterial.cpp
    src/main/vsdk/toolkit/environment/material/MicroFacetedMaterial.cpp
    src/main/vsdk/toolkit/environment/material/ShadingType.cpp
    src/main/vsdk/toolkit/environment/material/RendererConfiguration.cpp
    src/main/vsdk/toolkit/processing/CurveModeler.cpp
    src/main/vsdk/toolkit/processing/ImageProcessing.cpp
    src/main/vsdk/toolkit/environment/background/CubemapBackground.cpp
    src/main/vsdk/toolkit/environment/background/FixedBackground.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/PolyhedralBoundedSolidModeler.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/_PolyhedralBoundedSolidOperator.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fallbacks/_PolyhedralBoundedSolidIdNamespace.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fallbacks/_PolyhedralBoundedSolidFallbackGeometry.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fallbacks/_PolyhedralBoundedSolidAxisAlignedCellFallback.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fallbacks/_PolyhedralBoundedSolidOffsetCylinderFallback.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/construction/_PolyhedralBoundedSolidProfileDifferenceFallback.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/construction/_PolyhedralBoundedSolidOrthogonalProfileFallback.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/slicing/_PolyhedralBoundedSolidSplitter.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/_SetOperationTrace.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/topology/_PolyhedralBoundedSolidSetOperatorNullEdge.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/topology/_PolyhedralBoundedSolidSetNullEdgesConnector.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/topology/_PolyhedralBoundedSolidSetFinisher.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/intersection/_PolyhedralBoundedSolidSetIntersectionCurveBuilder.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/intersection/_PolyhedralBoundedSolidSetIntersector.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/_PolyhedralBoundedSolidSetOperator.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/intersection/_PolyhedralBoundedSolidSetGeometricPredicateProcessor.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetOperatorSectorClassification.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetVertexFaceClassifier.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetVertexVertexClassifier.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetClassifier.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetNonIntersectingClassifier.cpp
    src/main/vsdk/toolkit/common/statistics/PolyhedralBoundedSolidStatistics.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fixtures/SimpleTestGeometryLibrary.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fixtures/CsgKurlanderBowlFixture.cpp
    src/main/vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/validation/_PolyhedralBoundedSolidFaceValidator.cpp
    src/main/vsdk/toolkit/io/geometry/ParametricCurvePersistence.cpp
    src/main/vsdk/toolkit/io/geometry/ParametricBiCubicPatchPersistence.cpp
    src/main/vsdk/toolkit/io/image/RGBColorPalettePersistence.cpp
    src/main/org/w3c/dom/Node.cpp
    src/main/javax/xml/parsers/DocumentBuilder.cpp
    src/main/javax/xml/transform/Transformer.cpp
    src/main/vsdk/toolkit/gui/widget/Widget.cpp
    src/main/vsdk/toolkit/gui/widget/WidgetButtonGroup.cpp
    src/main/vsdk/toolkit/gui/widget/WidgetCommand.cpp
    src/main/vsdk/toolkit/gui/widget/WidgetDialog.cpp
    src/main/vsdk/toolkit/gui/widget/WidgetDialogElement.cpp
    src/main/vsdk/toolkit/gui/dialog/components/DialogFileFilter.cpp
    src/main/vsdk/toolkit/gui/dialog/components/DialogTreeNode.cpp
    src/main/vsdk/toolkit/gui/dialog/runtime/DialogSession.cpp
    src/main/jackson/databind/JsonNode.cpp
    src/main/jackson/databind/ObjectMapper.cpp
    src/main/vsdk/toolkit/gui/framebuffer/Framebuffer.cpp
    src/main/vsdk/toolkit/gui/framebuffer/FramebufferMenuBar.cpp
    src/main/vsdk/toolkit/gui/framebuffer/FramebufferMenuReader.cpp
    src/main/vsdk/toolkit/gui/framebuffer/FramebufferPainter.cpp
    src/main/vsdk/toolkit/gui/framebuffer/FramebufferSnapshot.cpp
    src/main/vsdk/toolkit/gui/framebuffer/FramebufferTheme.cpp
    src/main/vsdk/toolkit/gui/framebuffer/FramebufferWindowFrame.cpp
    src/main/vsdk/toolkit/gui/framebuffer/FramebufferButton.cpp
    src/main/vsdk/toolkit/gui/framebuffer/FramebufferDialog.cpp
    src/main/vsdk/toolkit/gui/framebuffer/FramebufferDialogForm.cpp
    src/main/vsdk/toolkit/gui/framebuffer/FramebufferFileDialog.cpp
    src/main/vsdk/toolkit/gui/framebuffer/FramebufferImageView.cpp
    src/main/vsdk/toolkit/gui/framebuffer/FramebufferLabel.cpp
    src/main/vsdk/toolkit/gui/framebuffer/FramebufferList.cpp
    src/main/vsdk/toolkit/gui/framebuffer/FramebufferText.cpp
    src/main/vsdk/toolkit/gui/framebuffer/FramebufferTextField.cpp
    src/main/vsdk/toolkit/gui/framebuffer/FramebufferToolBar.cpp
    src/main/vsdk/toolkit/gui/framebuffer/FramebufferSidePanel.cpp
    src/main/vsdk/toolkit/gui/widget/WidgetMenu.cpp
    src/main/vsdk/toolkit/gui/widget/WidgetMenuElement.cpp
    src/main/vsdk/toolkit/gui/widget/WidgetMenuItem.cpp
    src/main/vsdk/toolkit/gui/viewport/Viewport.cpp
    src/main/vsdk/toolkit/gui/viewport/ViewportElementScaler.cpp
    src/main/vsdk/toolkit/gui/viewport/ViewportSet.cpp
    src/main/vsdk/toolkit/gui/viewport/ViewportSetInteractionTechniques.cpp
    src/main/vsdk/toolkit/gui/viewport/ViewportInteractionTechniques.cpp
    src/main/vsdk/toolkit/gui/gizmo/GizmoSolidTessellator.cpp
    src/main/vsdk/toolkit/gui/gizmo/GizmoVertexArrayBuilder.cpp
    src/main/vsdk/toolkit/gui/gizmo/InputGizmo.cpp
    src/main/vsdk/toolkit/gui/gizmo/InputGizmoValueChangeRules.cpp
    src/main/vsdk/toolkit/gui/gizmo/ReferenceFrameGizmo.cpp
    src/main/vsdk/toolkit/gui/gizmo/TranslateGizmo.cpp
    src/main/vsdk/toolkit/gui/gizmo/TranslateGizmoInteractionTechnique.cpp
    src/main/vsdk/toolkit/gui/gizmo/RotateGizmo.cpp
    src/main/vsdk/toolkit/gui/gizmo/RotateGizmoInteractionTechnique.cpp
    src/main/vsdk/toolkit/gui/gizmo/ScaleGizmo.cpp
    src/main/vsdk/toolkit/gui/gizmo/ScaleGizmoInteractionTechnique.cpp
)

if(WITH_FREETYPE)
    list(APPEND VITRAL_BASE_SOURCES
        src/main/vsdk/toolkit/io/geometry/Freetype2FontReader.cpp)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  set_source_files_properties(
    src/main/vsdk/toolkit/common/numerics/FloatingInterval.cpp
    src/main/vsdk/toolkit/environment/scene/AffineToleranceMap.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgExactArithmetic.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgDyadic.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgDyadicTransform.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgDyadicSolid.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgExactPlane.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgExactTransform.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgExactQuadric.cpp
    src/main/vsdk/toolkit/environment/scene/csg/CsgSolidEquivalence.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/QuadraticInteriorBound.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/AffineEllipsoidMinimumDistance.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/AffineEllipsoidDualBound.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/AffineEllipsoidDistanceBounds.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/Sphere.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/Box.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/PlanarHalfSpace.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/QuadricSurface.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/ImplicitSolid.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/PolynomialSurface.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/BlobComponent.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/BlobSolid.cpp
    src/main/vsdk/toolkit/environment/geometry/volume/HeightFieldSolid.cpp
    src/main/vsdk/toolkit/environment/scene/SimpleBody.cpp
    src/main/vsdk/toolkit/environment/geometry/element/GeometryBounds.cpp
    src/main/java/lang/Double.cpp
    PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
add_library(vitral_base SHARED ${VITRAL_BASE_SOURCES})

# java::Thread runs over std::thread
find_package(Threads REQUIRED)
target_link_libraries(vitral_base PUBLIC Threads::Threads)

target_include_directories(vitral_base PUBLIC src src/main)

# Floating point parity with the Java port: Java never fuses a*b+c into a
# single fused multiply-add, while compilers do by default on some targets
# (e.g. clang on arm64). A fused operation rounds differently in the last
# bits, which changes exact comparisons (e.g. the null edges sort of the
# boolean set operations). PUBLIC because inline header math (Vector3Dd)
# is compiled in the client targets too.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(vitral_base PUBLIC -ffp-contract=off)
endif()

# Link optional libraries and set compile definitions
if(WITH_JPEG)
    target_include_directories(vitral_base PRIVATE ${JPEG_INCLUDE_DIR})
    target_link_libraries(vitral_base PRIVATE ${JPEG_LIBRARIES})
    target_compile_definitions(vitral_base PRIVATE VITRAL_WITH_JPEG)
    message(STATUS "JPEG support enabled: ${JPEG_LIBRARIES}")
endif()

if(WITH_FREETYPE)
    target_link_libraries(vitral_base PRIVATE Freetype::Freetype)
    target_compile_definitions(vitral_base PUBLIC VITRAL_WITH_FREETYPE)
    message(STATUS "FreeType support enabled: ${FREETYPE_LIBRARIES}")
endif()

if(WITH_PNG)
    target_include_directories(vitral_base PRIVATE ${PNG_INCLUDE_DIR})
    target_link_libraries(vitral_base PRIVATE ${PNG_LIBRARIES})
    target_compile_definitions(vitral_base PRIVATE VITRAL_WITH_PNG)
    message(STATUS "PNG support enabled: ${PNG_LIBRARIES}")
endif()

if(WITH_POSIX_THREADS)
    target_compile_definitions(vitral_base PUBLIC VITRAL_WITH_POSIX_THREADS)
    target_link_libraries(vitral_base PUBLIC pthread)
endif()

option(VITRAL_BUILD_TESTS "Build tests" ON)
if(VITRAL_BUILD_TESTS)
  include(CTest)
  enable_testing()

  include(FetchContent)
  set(_gtest_url "https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip")
  if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24")
    FetchContent_Declare(
      googletest
      URL "${_gtest_url}"
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
  else()
    FetchContent_Declare(
      googletest
      URL "${_gtest_url}"
    )
  endif()
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  add_executable(vitral_base_tests
    src/test/normal_map_texture_tests.cpp
    src/test/vector_tests.cpp
    src/test/quaternion_tests.cpp
    src/test/matrix_nxm_tests.cpp
    src/test/matrix4x4_tests.cpp
    src/test/parallel_raytracer_tests.cpp
    src/test/raytracer_depth_buffer_tests.cpp
    src/test/entity_tests.cpp
    src/test/generic_editor_tests.cpp
    src/test/dialog_persistence_tests.cpp
    src/test/java_emulation_tests.cpp
    src/test/image_persistence_tests.cpp
    src/test/GeometryWorkspaceTestWorker.cpp
    src/test/RayCrossingTestCollector.cpp
    src/test/CsgTestOperand.cpp
    src/test/UncertifiedTestBody.cpp
    src/test/CsgCycleTestOperand.cpp
    src/test/CsgPolicyTestOperand.cpp
    src/test/CsgPolicyTestTree.cpp
    src/test/CsgOpaqueTestExpression.cpp
    src/test/csg_placement_tests.cpp
    src/test/certified_containment_tests.cpp
    src/test/csg_evaluator_tests.cpp
    src/test/ImplicitSolidTestFactory.cpp
    src/test/implicit_solid_tests.cpp
    src/test/HeightFieldTestFactory.cpp
    src/test/heightfield_solid_tests.cpp
    src/test/subdivided_bezier_patch_tests.cpp
    src/test/microfaceted_material_csv_tests.cpp
    src/test/ray_affine_contract_tests.cpp
    src/test/ray_crossing_tests.cpp
    src/test/reader_mit_triangles_tests.cpp
    src/test/triangle_mesh_contract_tests.cpp
    src/test/sphere_polyhedral_cache_tests.cpp
    src/test/box_cone_containment_tests.cpp
    src/test/voxel_volume_tests.cpp
    src/test/polyhedral_bounded_solid_containment_tests.cpp
    src/test/parametric_bicubic_patch_tests.cpp
    src/test/polyhedral_bounded_solid_splitter_tests.cpp
    src/test/polyhedral_bounded_solid_set_intersector_tests.cpp
    src/test/polyhedral_bounded_solid_topology_editing_tests.cpp
    src/test/polyhedral_bounded_solid_non_intersecting_classifier_tests.cpp
    src/test/polyhedral_bounded_solid_set_connect_finish_tests.cpp
    src/test/polyhedral_bounded_solid_booleans_reference_pairs_tests.cpp
    src/test/polyhedral_bounded_solid_kurlander_bowl_tests.cpp
    src/test/freetype2_font_reader_tests.cpp
  )
  target_link_libraries(vitral_base_tests PRIVATE vitral_base GTest::gtest_main)
  # Repository root, for the test data under `etc/` (fonts)
  target_compile_definitions(vitral_base_tests PRIVATE
      VITRAL_REPOSITORY_ROOT="${CMAKE_CURRENT_LIST_DIR}/../..")
  include(GoogleTest)
  gtest_discover_tests(vitral_base_tests DISCOVERY_TIMEOUT 60)
endif()
