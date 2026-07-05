#include "app/ImageOutputAdapter.h"
#include "app/PovRayApplication.h"
#include "app/options/CommandLineOptions.h"
#include "common/statistics/Statistics.h"
#include "environment/material/povray/PovRayMaterial.h"
#include "environment/material/RenderOutput.h"
#include "environment/material/PovRayRendererConfiguration.h"
#include "environment/scene/Scene.h"
#include "io/image/RawDumpFormat.h"
#include "io/image/RawFormat.h"
#include "io/image/TargaFormat.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/material/LoadedImageRegistry.h"
#include "io/pov/scene/SceneParser.h"
#include "render/bakedScene/BakedScene.h"
#include "vsdk/toolkit/common/logging/Logger.h"

void
PovRayApplication::printProgress(const char *message)
{
    Logger::reportMessage("PovRayApplication", Logger::WARNING, "", message);
}

void
PovRayApplication::printStatistics(
    const Statistics &stats,
    const Scene &frame,
    const BakedScene &bakedScene,
    const PovRayRendererConfiguration &inputConfiguration)
{
    char buffer[1024];
    const long pixelsInImage =
        (long)frame.getScreenWidth() * (long)frame.getScreenHeight();

    snprintf(buffer, sizeof(buffer), "\n%s statistics",
        inputConfiguration.getInputFileName());
    printProgress(buffer);

    if (pixelsInImage > stats.getNumberOfPixels()) {
        printProgress("  Partial Image Rendered");
    }

    printProgress("--------------------------------------");
    snprintf(buffer, sizeof(buffer), "Resolution %d x %d", frame.getScreenWidth(),
        frame.getScreenHeight());
    printProgress(buffer);

    snprintf(buffer, sizeof(buffer),
        "# Rays:  %10ld     # Pixels:  %10ld  # Pixels super-sampled: %10ld",
        stats.getNumberOfRays(), stats.getNumberOfPixels(),
        stats.getNumberOfPixelsSuperSampled());
    printProgress(buffer);

    printProgress("  Ray->Shape Intersection Tests:");
    printProgress("    type                 Tests     Succeeded    Percentage");
    printProgress("  -----------------------------------------------------------");

    auto logIntersectionRow = [this, &buffer](const char *label, long tests, long succeeded) {
        if (tests) {
            snprintf(buffer, sizeof(buffer), "%s %10ld  %10ld  %10.2f", label, tests, succeeded,
                (((double)succeeded / (double)tests) * 100.0));
            printProgress(buffer);
        }
    };

    logIntersectionRow("  Sphere        ", stats.getRaySphereTests(), stats.getRaySphereTestsSucceeded());
    logIntersectionRow("  Plane         ", stats.getRayPlaneTests(), stats.getRayPlaneTestsSucceeded());
    logIntersectionRow("  Triangle     ", stats.getRayTriangleTests(), stats.getRayTriangleTestsSucceeded());
    logIntersectionRow("  Quadric       ", stats.getRayQuadricTests(), stats.getRayQuadricTestsSucceeded());
    logIntersectionRow("  Blob          ", stats.getRayBlobTests(), stats.getRayBlobTestsSucceeded());
    logIntersectionRow("  Box           ", stats.getRayBoxTests(), stats.getRayBoxTestsSucceeded());
    logIntersectionRow("  Quartic\\Poly ", stats.getRayPolyTests(), stats.getRayPolyTestsSucceeded());
    logIntersectionRow("  Bezier Patch ", stats.getRayBicubicTests(), stats.getRayBicubicTestsSucceeded());
    logIntersectionRow("  Height field   ", stats.getRayHtFieldTests(), stats.getRayHtFieldTestsSucceeded());
    logIntersectionRow("  Bounds        ", stats.getBoundingRegionTests(), stats.getBoundingRegionTestsSucceeded());
    logIntersectionRow("  Clips         ", stats.getClippingRegionTests(), stats.getClippingRegionTestsSucceeded());

    if (stats.getSolidTextureStatistics()->callsToNoise) {
        snprintf(buffer, sizeof(buffer), "  Calls to Noise:    %10ld", stats.getSolidTextureStatistics()->callsToNoise);
        printProgress(buffer);
    }
    if (stats.getSolidTextureStatistics()->callsToDNoise) {
        snprintf(buffer, sizeof(buffer), "  Calls to DNoise:  %10ld", stats.getSolidTextureStatistics()->callsToDNoise);
        printProgress(buffer);
    }
    if (stats.getShadowRayTests()) {
        snprintf(buffer, sizeof(buffer),
            "  Shadow Ray Tests: %10ld      Blocking Objects Found:  %10ld",
            stats.getShadowRayTests(), 0L);
        printProgress(buffer);
    }
    if (stats.getReflectedRaysTraced()) {
        snprintf(buffer, sizeof(buffer), "  Reflected Rays:    %10ld", stats.getReflectedRaysTraced());
        printProgress(buffer);
    }
    if (stats.getRefractedRaysTraced()) {
        snprintf(buffer, sizeof(buffer), "  Refracted Rays:    %10ld", stats.getRefractedRaysTraced());
        printProgress(buffer);
    }
    if (stats.getTransmittedRaysTraced()) {
        snprintf(buffer, sizeof(buffer), "  Transmitted Rays: %10ld", stats.getTransmittedRaysTraced());
        printProgress(buffer);
    }

    {
        const BakedScene::Statistics &bakedSceneStats = bakedScene.statistics;
        snprintf(buffer, sizeof(buffer),
            "  Plan 6 baked model (built, not yet consumed): empty %ld  direct %ld  csg %ld  "
            "composite %ld  bounded/clipped %ld  generic-fallback %ld",
            bakedSceneStats.countByKind[(int)BakedScene::TraceKind::Empty],
            bakedSceneStats.countByKind[(int)BakedScene::TraceKind::DirectPrimitive],
            bakedSceneStats.countByKind[(int)BakedScene::TraceKind::Csg],
            bakedSceneStats.countByKind[(int)BakedScene::TraceKind::Composite],
            bakedSceneStats.countByKind[(int)BakedScene::TraceKind::BoundedGeneric],
            bakedSceneStats.countByKind[(int)BakedScene::TraceKind::GenericFallback]);
        printProgress(buffer);
        snprintf(buffer, sizeof(buffer),
            "  Plan 6 CSG programs: %ld total - generic-morgan %ld  ray-segments %ld  "
            "plane-union %ld  disjoint-union %ld  core-plane %ld  fallback %ld",
            bakedSceneStats.csgProgramCount,
            bakedSceneStats.csgPlanGenericMorgan,
            bakedSceneStats.csgPlanGenericRaySegments,
            bakedSceneStats.csgPlanTopLevelPlaneUnion,
            bakedSceneStats.csgPlanDisjointBoundedUnion,
            bakedSceneStats.csgPlanSingleCorePlaneIntersection,
            bakedSceneStats.csgPlanFallback);
        printProgress(buffer);
        snprintf(buffer, sizeof(buffer),
            "  Plan 6 residual (un-collapsed) operands: transformed %ld  "
            "(collapsed: quadric %ld  plane %ld)",
            bakedSceneStats.residualTransformedOperands,
            bakedSceneStats.residualBakedQuadricOperands,
            bakedSceneStats.residualBakedPlaneOperands);
        printProgress(buffer);
        snprintf(buffer, sizeof(buffer),
            "  Plan 8 R0 residual breakdown: cat1-nestedCsg %ld (pushdown-eligible %ld)  "
            "cat2-emptySteps %ld  cat3-unbakeable %ld",
            bakedSceneStats.residualCategory1NestedCsg,
            bakedSceneStats.residualCategory1PushdownEligible,
            bakedSceneStats.residualCategory2EmptySteps,
            bakedSceneStats.residualCategory3Unbakeable);
        printProgress(buffer);
        snprintf(buffer, sizeof(buffer),
            "  Plan 13 census: union-op histogram 1-4:%ld 5-16:%ld 17-64:%ld 65+:%ld  "
            "(cull-safe %ld/%ld)  top-level objects %ld (cull-safe %ld)",
            bakedSceneStats.unionProgramOperandHistogram[0],
            bakedSceneStats.unionProgramOperandHistogram[1],
            bakedSceneStats.unionProgramOperandHistogram[2],
            bakedSceneStats.unionProgramOperandHistogram[3],
            bakedSceneStats.unionProgramOperandCullSafeCount,
            bakedSceneStats.unionProgramOperandTotalCount,
            bakedSceneStats.topLevelObjectCount,
            bakedSceneStats.topLevelObjectCullSafeCount);
        printProgress(buffer);
    }

    if (stats.getUsedTime() != 0.0) {
        const int hours = (int)stats.getUsedTime() / 3600;
        const int minutes = (int)(stats.getUsedTime() - hours * 3600) / 60;
        const double seconds =
            stats.getUsedTime() - (double)(hours * 3600 + minutes * 60);
        snprintf(buffer, sizeof(buffer),
            "  Time For Trace:    %2d hours %2d minutes %4.2f seconds", hours,
            minutes, seconds);
        printProgress(buffer);
    }
}

void
PovRayApplication::run(int argc, char *argv[])
{
    initializeFromCommandLine(argc, argv);
    configureOutputTarget();
    parseSceneDescription();
    prepareRendering();
    runRenderLoop();
    finalizeRun();
}

void
PovRayApplication::initializeFromCommandLine(int argc, char *argv[])
{
    if (argc == 1) {
        CommandLineOptions::usage();
    }

    initVars();
    fileLocator.clearSearchPaths();

    CommandLineOptions::loadDefaults(configuration, fileLocator, scene);
    CommandLineOptions::parseArguments(argc, argv, configuration, fileLocator, scene);

    if (configuration.getLastLine() == -1) {
        configuration.setLastLine(
            engine.getScene().getScreenHeight());
    }
}

void
PovRayApplication::configureOutputTarget()
{
    if (!configuration.hasOptionFlags(PovRayRendererConfiguration::DISK_WRITE)) {
        return;
    }

    switch (configuration.getOutputFormat()) {
    case '\0':
    case 'd':
    case 'D':
        if ((selectedImageOutput = new RawDumpFormat()) == nullptr) {
            closeAll();
            exit(1);
        }
        break;
    case 'r':
    case 'R':
        if ((selectedImageOutput = new RawFormat()) == nullptr) {
            closeAll();
            exit(1);
        }
        break;
    case 't':
    case 'T':
        if ((selectedImageOutput = new TargaFormat()) == nullptr) {
            closeAll();
            exit(1);
        }
        break;
    default:
        {
            char _logMsg[1024];
            snprintf(_logMsg, sizeof(_logMsg), "Unrecognized output file format %c\n", configuration.getOutputFormat());
            Logger::reportMessage("PovRayApplication", Logger::FATAL_ERROR, "", _logMsg);
        }
    }

    selectedImageOutput->setFileLocator(&fileLocator);
    configuration.setOutputFileInputStream(
        new ImageOutputAdapter(selectedImageOutput));

    if (!configuration.hasOutputFileName()) {
        configuration.setOutputFileName(
            configuration.getOutputFileInputStream()->defaultFileName());
    }
}

void
PovRayApplication::parseSceneDescription()
{
    ParserContext ctx;
    ctx.tokenizer().setCaseSensitiveIdentifiers(configuration.getTokenizerCaseSensitiveMode());
    ctx.tokenizer().setMaxSymbols(configuration.getTokenizerMaxSymbols());
    ctx.tokenizer().setFileLocator(&fileLocator);
    ctx.tokenizer().initializeTokenizer(configuration.getInputFileName());
    printProgress("Parsing...");

    ctx.setDiagnostics(
        configuration.hasOptionFlags(PovRayRendererConfiguration::VERBOSE_FILE),
        configuration.getStatFileName());
    ctx.setCsgRoth(configuration.hasOptionFlags(PovRayRendererConfiguration::CSG_ROTH));
    ctx.setAntialiasThreshold(configuration.getAntialiasThreshold());
    ctx.setRuntimeState(&runtimeState);
    SceneParser::parse(&engine.getScene(), ctx);
    ctx.tokenizer().terminateTokenizer();
}

void
PovRayApplication::prepareRendering()
{
    if (configuration.hasOptionFlags(PovRayRendererConfiguration::DISPLAY)) {
        printProgress("Displaying...\n");
    }

    engine.buildBakedScene();

    if (configuration.hasOptionFlags(PovRayRendererConfiguration::DISK_WRITE)) {
        if (configuration.hasOptionFlags(PovRayRendererConfiguration::CONTINUE_TRACE)) {
            if (configuration.getOutputFileInputStream()->open(
                    configuration.getOutputFileNameBuffer(),
                    &engine.getScene().getScreenWidth(), &engine.getScene().getScreenHeight(),
                    configuration.getFileBufferSize(), RenderOutput::READ_MODE,
                    configuration.getFirstLine()) != 1) {
                Logger::reportMessage("PovRayApplication", Logger::ERROR, "", "Error opening continue trace output file");
                char buffer[512];
                snprintf(buffer, sizeof(buffer), "Opening new output file %s.", configuration.getOutputFileName());
                printProgress(buffer);
                configuration.clearOptionFlags(PovRayRendererConfiguration::CONTINUE_TRACE);

                if (configuration.getOutputFileInputStream()->open(
                        configuration.getOutputFileNameBuffer(),
                        &engine.getScene().getScreenWidth(), &engine.getScene().getScreenHeight(),
                        configuration.getFileBufferSize(), RenderOutput::WRITE_MODE,
                        configuration.getFirstLine()) != 1) {
                    Logger::reportMessage("PovRayApplication", Logger::ERROR, "", "Error opening output file\n");
                    closeAll();
                    exit(1);
                }
            }

            engine.initializeRenderer();
            if (configuration.hasOptionFlags(PovRayRendererConfiguration::CONTINUE_TRACE)) {
                engine.readRenderedPart();
            }
        } else {
            if (configuration.getOutputFileInputStream()->open(
                    configuration.getOutputFileNameBuffer(),
                    &engine.getScene().getScreenWidth(), &engine.getScene().getScreenHeight(),
                    configuration.getFileBufferSize(), RenderOutput::WRITE_MODE,
                    configuration.getFirstLine()) != 1) {
                Logger::reportMessage("PovRayApplication", Logger::ERROR, "", "Error opening output file\n");
                closeAll();
                exit(1);
            }

            engine.initializeRenderer();
        }
    } else {
        engine.initializeRenderer();
    }

    engine.getIntersectionQueuePool().init();
    textureUtils.initialize(statistics.getSolidTextureStatistics());
    textureUtils.initializeNoise(PovRayMaterial::DEFAULT_NUMBER_OF_WAVES);
}

void
PovRayApplication::runRenderLoop()
{
    if (configuration.hasOptionFlags(PovRayRendererConfiguration::VERBOSE) && (configuration.getVerboseFormat() != '1')) {
        printProgress("Rendering...");
    } else if (configuration.hasOptionFlags(PovRayRendererConfiguration::VERBOSE) && (configuration.getVerboseFormat() == '1')) {
        char buffer[512];
        snprintf(buffer, sizeof(buffer), "POV-Ray rendering %s to %s :",
            configuration.getInputFileName(), configuration.getOutputFileName());
        printProgress(buffer);
    }

    if (configuration.hasOptionFlags(PovRayRendererConfiguration::PARALLEL)) {
        engine.startTracingParallel();
    } else {
        engine.startTracing();
    }
}

void
PovRayApplication::finalizeRun()
{
    closeAll();
    printStatistics(statistics, engine.getScene(), engine.getBakedScene(), configuration);

    // image_map/bump_map/material_map images are intentionally shared (not
    // cloned) by every PovRayMaterial generation/clone that references them
    // (see LoadedImageRegistry), so they can only be freed once rendering -
    // the last thing that could still read one via a texture lookup - has
    // fully finished. runRenderLoop() (called before finalizeRun() in run())
    // is synchronous, so this is safe here.
    LoadedImageRegistry::freeAll();

    printProgress("Done Tracing");
}

void
PovRayApplication::initVars()
{
    configuration.reset();
    runtimeState.reset();
    statistics.reset();

    engine.getScene().setScreenHeight(100);
    engine.getScene().setScreenWidth(100);
}

void
PovRayApplication::closeAll()
{
    if (configuration.getOutputFileInputStream()) {
        configuration.getOutputFileInputStream()->close();
        delete configuration.getOutputFileInputStream();
        configuration.setOutputFileInputStream(nullptr);
    }
    if (selectedImageOutput) {
        delete selectedImageOutput;
        selectedImageOutput = nullptr;
    }
}
