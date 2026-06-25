#include "vsdk/toolkit/common/logging/Logger.h"
#include "environment/material/RenderOutput.h"
#include "environment/material/PovRayRendererConfiguration.h"
#include "render/RenderEngine.h"
#include "render/RenderImageWriter.h"

void
RenderImageWriter::readRenderedPart(ColorRgba *lineBuffer)
{
    PovRayRendererConfiguration &config = renderEngine->getMutableConfig();
    Scene &scene = renderEngine->getScene();
    int rc;
    int lineNumber;

    while ((rc = config.getOutputFileInputStream()->readLine(
                lineBuffer, &lineNumber)) == 1) {
    }

    config.setFirstLine(lineNumber + 1);

    if (rc == 0) {
        config.getOutputFileInputStream()->close();
        if (config.getOutputFileInputStream()->open(
                config.getOutputFileNameBuffer(),
                &scene.getScreenWidth(),
                &scene.getScreenHeight(),
                config.getFileBufferSize(), RenderOutput::APPEND_MODE,
                config.getFirstLine()) != 1) {
            Logger::reportMessage("RenderImageWriter", Logger::FATAL_ERROR, "",
                "Error opening output file\n");
        }
        return;
    }

    Logger::reportMessage("RenderImageWriter", Logger::ERROR, "",
        "Error reading aborted data file\n");
}
