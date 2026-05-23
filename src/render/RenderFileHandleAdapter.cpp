#include "render/RenderFileHandleAdapter.h"
#include "io/image/ImageFileHandle.h"

int
RenderFileInputStreamAdapter::readLine(
    ImageFileHandle *fileHandle, RGBAColor *lineData, int *lineNumber)
{
    return fileHandle->readLine(lineData, lineNumber);
}

int
RenderFileInputStreamAdapter::open(ImageFileHandle *fileHandle, char *name,
    int *imageWidth, int *imageHeight, int bufferSize, int openMode)
{
    return fileHandle->open(name, imageWidth, imageHeight, bufferSize, openMode);
}

void
RenderFileInputStreamAdapter::close(ImageFileHandle *fileHandle)
{
    fileHandle->close();
}
