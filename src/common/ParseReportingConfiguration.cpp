#include "common/ParseReportingConfiguration.h"

#include "environment/material/RendererConfiguration.h"

bool
ParseReportingConfiguration::writesVerboseErrors(const RenderingConfiguration &config)
{
    return config.hasOptionFlags(RenderingConfiguration::VERBOSE_FILE);
}

const char *
ParseReportingConfiguration::statFileName(const RenderingConfiguration &config)
{
    return config.getStatFileName();
}
