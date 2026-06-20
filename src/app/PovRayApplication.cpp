#include <cstdlib>

#include "app/ImageOutputAdapter.h"
#include "app/PovRayApplication.h"
#include "app/options/CommandLineOptions.h"
#include "common/statistics/Statistics.h"
#include "environment/material/PovrayMaterial.h"
#include "environment/material/RenderOutput.h"
#include "environment/material/RendererConfiguration.h"
#include "environment/scene/Scene.h"
#include "io/image/RawDumpFormat.h"
#include "io/image/RawFormat.h"
#include "io/image/TargaFormat.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/scene/SceneParser.h"
#include "vsdk/toolkit/common/logging/Logger.h"

void
PovRayApplication::printStatistics(
    const Statistics &stats,
    const Scene &frame,
    const RenderingConfiguration &inputConfiguration)
{
    char buffer[1024];
    const long pixelsInImage =
        (long)frame.getScreenWidth() * (long)frame.getScreenHeight();

    snprintf(buffer, sizeof(buffer), "\n%s statistics",
        inputConfiguration.getInputFileName());
    Logger::reportMessage("PovRayApplication", Logger::WARNING, "", buffer);

    if (pixelsInImage > stats.getNumberOfPixels()) {
        Logger::reportMessage("PovRayApplication", Logger::WARNING, "", "  Partial Image Rendered");
    }

    Logger::reportMessage("PovRayApplication", Logger::WARNING, "", "--------------------------------------");
    snprintf(buffer, sizeof(buffer), "Resolution %d x %d", frame.getScreenWidth(),
        frame.getScreenHeight());
    Logger::reportMessage("PovRayApplication", Logger::WARNING, "", buffer);

    snprintf(buffer, sizeof(buffer),
        "# Rays:  %10ld     # Pixels:  %10ld  # Pixels super-sampled: %10ld",
        stats.getNumberOfRays(), stats.getNumberOfPixels(),
        stats.getNumberOfPixelsSupersampled());
    Logger::reportMessage("PovRayApplication", Logger::WARNING, "", buffer);

    Logger::reportMessage("PovRayApplication", Logger::WARNING, "", "  Ray->Shape Intersection Tests:");
    Logger::reportMessage("PovRayApplication", Logger::WARNING, "", "    type                 Tests     Succeeded    Percentage");
    Logger::reportMessage("PovRayApplication", Logger::WARNING, "", "  -----------------------------------------------------------");

    auto logIntersectionRow = [&buffer](const char *label, long tests, long succeeded) {
        if (tests) {
            snprintf(buffer, sizeof(buffer), "%s %10ld  %10ld  %10.2f", label, tests, succeeded,
                (((double)succeeded / (double)tests) * 100.0));
            Logger::reportMessage("PovRayApplication", Logger::WARNING, "", buffer);
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
    logIntersectionRow("  Height field Box ", stats.getRayHtFieldBoxTests(), stats.getRayHFieldBoxTestsSucceeded());
    logIntersectionRow("  Bounds        ", stats.getBoundingRegionTests(), stats.getBoundingRegionTestsSucceeded());
    logIntersectionRow("  Clips         ", stats.getClippingRegionTests(), stats.getClippingRegionTestsSucceeded());

    if (stats.getSolidTextureStatistics()->callsToNoise) {
        snprintf(buffer, sizeof(buffer), "  Calls to Noise:    %10ld", stats.getSolidTextureStatistics()->callsToNoise);
        Logger::reportMessage("PovRayApplication", Logger::WARNING, "", buffer);
    }
    if (stats.getSolidTextureStatistics()->callsToDNoise) {
        snprintf(buffer, sizeof(buffer), "  Calls to DNoise:  %10ld", stats.getSolidTextureStatistics()->callsToDNoise);
        Logger::reportMessage("PovRayApplication", Logger::WARNING, "", buffer);
    }
    if (stats.getShadowRayTests()) {
        snprintf(buffer, sizeof(buffer),
            "  Shadow Ray Tests: %10ld      Blocking Objects Found:  %10ld",
            stats.getShadowRayTests(), stats.getShadowRaysSucceeded());
        Logger::reportMessage("PovRayApplication", Logger::WARNING, "", buffer);
    }
    if (stats.getReflectedRaysTraced()) {
        snprintf(buffer, sizeof(buffer), "  Reflected Rays:    %10ld", stats.getReflectedRaysTraced());
        Logger::reportMessage("PovRayApplication", Logger::WARNING, "", buffer);
    }
    if (stats.getRefractedRaysTraced()) {
        snprintf(buffer, sizeof(buffer), "  Refracted Rays:    %10ld", stats.getRefractedRaysTraced());
        Logger::reportMessage("PovRayApplication", Logger::WARNING, "", buffer);
    }
    if (stats.getTransmittedRaysTraced()) {
        snprintf(buffer, sizeof(buffer), "  Transmitted Rays: %10ld", stats.getTransmittedRaysTraced());
        Logger::reportMessage("PovRayApplication", Logger::WARNING, "", buffer);
    }

    if (stats.getUsedTime() != 0.0) {
        const int hours = (int)stats.getUsedTime() / 3600;
        const int minutes = (int)(stats.getUsedTime() - hours * 3600) / 60;
        const double seconds =
            stats.getUsedTime() - (double)(hours * 3600 + minutes * 60);
        snprintf(buffer, sizeof(buffer),
            "  Time For Trace:    %2d hours %2d minutes %4.2f seconds", hours,
            minutes, seconds);
        Logger::reportMessage("PovRayApplication", Logger::WARNING, "", buffer);
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
    if (!configuration.hasOptionFlags(RenderingConfiguration::DISK_WRITE)) {
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
    Logger::reportMessage("PovRayApplication", Logger::WARNING, "", "Parsing...");

    ctx.setDiagnostics(
        configuration.hasOptionFlags(RenderingConfiguration::VERBOSE_FILE),
        configuration.getStatFileName());
    ctx.setAntialiasThreshold(configuration.getAntialiasThreshold());
    ctx.setRuntimeState(&runtimeState);
    SceneParser::parse(&engine.getScene(), ctx);
    ctx.tokenizer().terminateTokenizer();
}

void
PovRayApplication::prepareRendering()
{
    if (configuration.hasOptionFlags(RenderingConfiguration::DISPLAY)) {
        Logger::reportMessage("PovRayApplication", Logger::WARNING, "", "Displaying...\n");
    }

    if (configuration.hasOptionFlags(RenderingConfiguration::DISK_WRITE)) {
        if (configuration.hasOptionFlags(RenderingConfiguration::CONTINUE_TRACE)) {
            if (configuration.getOutputFileInputStream()->open(
                    configuration.getOutputFileNameBuffer(),
                    &engine.getScene().getScreenWidth(), &engine.getScene().getScreenHeight(),
                    configuration.getFileBufferSize(), RenderOutput::READ_MODE,
                    configuration.getFirstLine()) != 1) {
                Logger::reportMessage("PovRayApplication", Logger::ERROR, "", "Error opening continue trace output file");
                char buffer[512];
                snprintf(buffer, sizeof(buffer), "Opening new output file %s.", configuration.getOutputFileName());
                Logger::reportMessage("PovRayApplication", Logger::WARNING, "", buffer);
                configuration.clearOptionFlags(RenderingConfiguration::CONTINUE_TRACE);

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
            if (configuration.hasOptionFlags(RenderingConfiguration::CONTINUE_TRACE)) {
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
    textureUtils.initializeNoise(PovrayMaterial::DEFAULT_NUMBER_OF_WAVES);
}

void
PovRayApplication::runRenderLoop()
{
    statistics.startTimer();

    if (configuration.hasOptionFlags(RenderingConfiguration::VERBOSE) && (configuration.getVerboseFormat() != '1')) {
        Logger::reportMessage("PovRayApplication", Logger::WARNING, "", "Rendering...");
    } else if (configuration.hasOptionFlags(RenderingConfiguration::VERBOSE) && (configuration.getVerboseFormat() == '1')) {
        char buffer[512];
        snprintf(buffer, sizeof(buffer), "POV-Ray rendering %s to %s :",
            configuration.getInputFileName(), configuration.getOutputFileName());
        Logger::reportMessage("PovRayApplication", Logger::WARNING, "", buffer);
    }

    engine.startTracing();
}

void
PovRayApplication::finalizeRun()
{
    statistics.stopTimer();

    closeAll();
    printStatistics(statistics, engine.getScene(), configuration);

    Logger::reportMessage("PovRayApplication", Logger::WARNING, "", "Done Tracing");
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

// Close all the stuff that has been opened
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
