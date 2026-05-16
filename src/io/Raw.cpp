/****************************************************************************
 *                     raw.c
 *
 *  This module contains the code to read and write the RAW file format
 *  The format is as follows:
 *
 *
 *  (header:)
 *     wwww hhhh         - Width, Height (16 bits, LSB first)
 *
 *  (each scanline:)
 *     llll                - Line number (16 bits, LSB first)
 *     rr rr rr ...     - Red data for line (8 bits per pixel,
 *                              left to right, 0-255 (255=bright, 0=dark))
 *     gg gg gg ...     - Green data for line (8 bits per pixel,
 *                              left to right, 0-255 (255=bright, 0=dark))
 *     bb bb bb ...     - Blue data for line (8 bits per pixel,
 *                              left to right, 0-255 (255=bright, 0=dark))
 *
 *****************************************************************************/

#include "io/Raw.h"
#include "common/Frame.h"
#include "common/PovProto.h"

class RAW_FILE_HANDLE {
  public:
    FileHandle root;
    FILE *redFile, *greenFile, *blueFile;
    char *redBuffer, *greenBuffer, *blueBuffer;
    int lineNumber;
};

FileHandle *
getRawFileHandle()
{
    RAW_FILE_HANDLE *handle;

    handle = new RAW_FILE_HANDLE;
    if (handle == nullptr) {
        fprintf(stderr, "Cannot allocate memory for output file handle\n");
        return (nullptr);
    }

    handle->root.Default_File_Name_p = defaultRawFileName;
    handle->root.Open_File_p = openRawFile;
    handle->root.Write_Line_p = writeRawLine;
    handle->root.Read_Line_p = readRawLine;
    handle->root.Close_File_p = closeRawFile;
    return ((FileHandle *)handle);
}

static char defaultNameData[] = "data";

char *
defaultRawFileName()
{
    return defaultNameData;
}

int
openRawFile(FileHandle *handle, char *name, int *width, int *height,
    int bufferSize, int mode)
{
    RAW_FILE_HANDLE *rawHandle;
    char fileName[256];

    handle->mode = mode;
    handle->filename = name;
    rawHandle = (RAW_FILE_HANDLE *)handle;
    rawHandle->lineNumber = 0;

    switch (mode) {
    case READ_MODE:
        strcpy(fileName, name);
        strcat(fileName, RED_RAW_FILE_EXTENSION);

        if ((rawHandle->redFile = fopen(fileName, READ_FILE_STRING)) ==
            nullptr) {
            return (0);
        }

        strcpy(fileName, name);
        strcat(fileName, GREEN_RAW_FILE_EXTENSION);

        if ((rawHandle->greenFile = fopen(fileName, READ_FILE_STRING)) ==
            nullptr) {
            return (0);
        }

        strcpy(fileName, name);
        strcat(fileName, BLUE_RAW_FILE_EXTENSION);

        if ((rawHandle->blueFile = fopen(fileName, READ_FILE_STRING)) ==
            nullptr) {
            return (0);
        }

        if (bufferSize != 0) {
            rawHandle->redBuffer = new char[bufferSize];
            if (rawHandle->redBuffer == nullptr) {
                return (0);
            }

            setvbuf(
                rawHandle->redFile, rawHandle->redBuffer, _IOFBF, bufferSize);
        }

        if (bufferSize != 0) {
            rawHandle->greenBuffer = new char[bufferSize];
            if (rawHandle->greenBuffer == nullptr) {
                return (0);
            }

            setvbuf(rawHandle->greenFile, rawHandle->greenBuffer, _IOFBF,
                bufferSize);
        }

        if (bufferSize != 0) {
            rawHandle->blueBuffer = new char[bufferSize];
            if (rawHandle->blueBuffer == nullptr) {
                return (0);
            }

            setvbuf(
                rawHandle->blueFile, rawHandle->blueBuffer, _IOFBF, bufferSize);
        }

        handle->width = *width;
        handle->height = *height;
        handle->buffer_size = bufferSize;
        break;

    case WRITE_MODE:
        strcpy(fileName, name);
        strcat(fileName, RED_RAW_FILE_EXTENSION);

        if ((rawHandle->redFile = fopen(fileName, WRITE_FILE_STRING)) ==
            nullptr) {
            return (0);
        }

        if (bufferSize != 0) {
            rawHandle->redBuffer = new char[bufferSize];
            if (rawHandle->redBuffer == nullptr) {
                return (0);
            }

            setvbuf(
                rawHandle->redFile, rawHandle->redBuffer, _IOFBF, bufferSize);
        }

        strcpy(fileName, name);
        strcat(fileName, GREEN_RAW_FILE_EXTENSION);

        if ((rawHandle->greenFile = fopen(fileName, WRITE_FILE_STRING)) ==
            nullptr) {
            return (0);
        }

        if (bufferSize != 0) {
            rawHandle->greenBuffer = new char[bufferSize];
            if (rawHandle->greenBuffer == nullptr) {
                return (0);
            }

            setvbuf(rawHandle->greenFile, rawHandle->greenBuffer, _IOFBF,
                bufferSize);
        }

        strcpy(fileName, name);
        strcat(fileName, BLUE_RAW_FILE_EXTENSION);

        if ((rawHandle->blueFile = fopen(fileName, WRITE_FILE_STRING)) ==
            nullptr) {
            return (0);
        }

        if (bufferSize != 0) {
            rawHandle->blueBuffer = new char[bufferSize];
            if (rawHandle->blueBuffer == nullptr) {
                return (0);
            }

            setvbuf(
                rawHandle->blueFile, rawHandle->blueBuffer, _IOFBF, bufferSize);
        }

        handle->width = *width;
        handle->height = *height;
        handle->buffer_size = bufferSize;

        break;

    case APPEND_MODE:
        strcpy(fileName, name);
        strcat(fileName, RED_RAW_FILE_EXTENSION);

        if ((rawHandle->redFile = fopen(fileName, APPEND_FILE_STRING)) ==
            nullptr) {
            return (0);
        }

        if (bufferSize != 0) {
            rawHandle->redBuffer = new char[bufferSize];
            if (rawHandle->redBuffer == nullptr) {
                return (0);
            }

            setvbuf(
                rawHandle->redFile, rawHandle->redBuffer, _IOFBF, bufferSize);
        }

        strcpy(fileName, name);
        strcat(fileName, GREEN_RAW_FILE_EXTENSION);

        if ((rawHandle->greenFile = fopen(fileName, APPEND_FILE_STRING)) ==
            nullptr) {
            return (0);
        }

        if (bufferSize != 0) {
            rawHandle->greenBuffer = new char[bufferSize];
            if (rawHandle->greenBuffer == nullptr) {
                return (0);
            }

            setvbuf(rawHandle->greenFile, rawHandle->greenBuffer, _IOFBF,
                bufferSize);
        }

        strcpy(fileName, name);
        strcat(fileName, BLUE_RAW_FILE_EXTENSION);

        if ((rawHandle->blueFile = fopen(fileName, APPEND_FILE_STRING)) ==
            nullptr) {
            return (0);
        }

        if (bufferSize != 0) {
            rawHandle->blueBuffer = new char[bufferSize];
            if (rawHandle->blueBuffer == nullptr) {
                return (0);
            }

            setvbuf(
                rawHandle->blueFile, rawHandle->blueBuffer, _IOFBF, bufferSize);
        }

        handle->width = *width;
        handle->height = *height;
        handle->buffer_size = bufferSize;

        break;
    }
    return (1);
}

void
writeRawLine(FileHandle *handle, RGBAColor *lineData, int lineNumber)
{
    register int x;
    RAW_FILE_HANDLE *rawHandle;
    char fileName[256];

    rawHandle = (RAW_FILE_HANDLE *)handle;

    for (x = 0; x < handle->width; x++) {
        putc((int)floor(lineData[x].Red * 255.0), rawHandle->redFile);
    }

    for (x = 0; x < handle->width; x++) {
        putc((int)floor(lineData[x].Green * 255.0), rawHandle->greenFile);
    }

    for (x = 0; x < handle->width; x++) {
        putc((int)floor(lineData[x].Blue * 255.0), rawHandle->blueFile);
    }

    if (handle->buffer_size == 0) {
        fflush(rawHandle->redFile); /* close and reopen file for */
        strcpy(fileName, handle->filename);
        strcat(fileName, RED_RAW_FILE_EXTENSION);
        rawHandle->redFile = freopen(fileName, APPEND_FILE_STRING,
            rawHandle->redFile); /* integrity in case we crash*/

        fflush(rawHandle->greenFile); /* close and reopen file for */
        strcpy(fileName, handle->filename);
        strcat(fileName, GREEN_RAW_FILE_EXTENSION);
        rawHandle->greenFile = freopen(fileName, APPEND_FILE_STRING,
            rawHandle->greenFile); /* integrity in case we crash*/

        fflush(rawHandle->blueFile); /* close and reopen file for */
        strcpy(fileName, handle->filename);
        strcat(fileName, BLUE_RAW_FILE_EXTENSION);
        rawHandle->blueFile = freopen(fileName, APPEND_FILE_STRING,
            rawHandle->blueFile); /* integrity in case we crash*/
    }
}

int
readRawLine(FileHandle *handle, RGBAColor *lineData, int *lineNumber)
{
    int data;
    int i;
    RAW_FILE_HANDLE *rawHandle;

    rawHandle = (RAW_FILE_HANDLE *)handle;

    for (i = 0; i < handle->width; i++) {
        if ((data = getc(rawHandle->redFile)) == EOF) {
            if (i == 0) {
                return (0);
            }
            return (-1);
        }

        lineData[i].Red = (DBL)data / 255.0;
    }

    for (i = 0; i < handle->width; i++) {
        if ((data = getc(rawHandle->greenFile)) == EOF) {
            return (-1);
        }

        lineData[i].Green = (DBL)data / 255.0;
    }

    for (i = 0; i < handle->width; i++) {
        if ((data = getc(rawHandle->blueFile)) == EOF) {
            return (-1);
        }

        lineData[i].Blue = (DBL)data / 255.0;
    }

    *lineNumber = rawHandle->lineNumber++;
    return (1);
}

void
closeRawFile(FileHandle *handle)
{
    RAW_FILE_HANDLE *rawHandle;

    rawHandle = (RAW_FILE_HANDLE *)handle;
    if (rawHandle->redFile) {
        fclose(rawHandle->redFile);
    }
    if (rawHandle->greenFile) {
        fclose(rawHandle->greenFile);
    }
    if (rawHandle->blueFile) {
        fclose(rawHandle->blueFile);
    }

    if (handle->buffer_size != 0) {
        delete rawHandle->redBuffer;
        delete rawHandle->greenBuffer;
        delete rawHandle->blueBuffer;
    }
}
