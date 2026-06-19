#ifndef __PARSE_REPORTING_CONFIGURATION__
#define __PARSE_REPORTING_CONFIGURATION__

class RenderingConfiguration;

class ParseReportingConfiguration {
  public:
    static bool writesVerboseErrors(const RenderingConfiguration *config);
    static const char *statFileName(const RenderingConfiguration *config);
};

#endif
