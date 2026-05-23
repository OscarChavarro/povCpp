#include "render/RenderFileInputStreamAdapter.h"
#include "java/io/FileInputStream.h"

int
RenderFileInputStreamAdapter::readLine(
    FileInputStream *fileHandle, RGBAColor *lineData, int *lineNumber)
{
    return fileHandle->readLine(lineData, lineNumber);
}

int
RenderFileInputStreamAdapter::open(FileInputStream *fileHandle, char *name,
    int *imageWidth, int *imageHeight, int bufferSize, int openMode)
{
    return fileHandle->open(name, imageWidth, imageHeight, bufferSize, openMode);
}

void
RenderFileInputStreamAdapter::close(FileInputStream *fileHandle)
{
    fileHandle->close();
}
