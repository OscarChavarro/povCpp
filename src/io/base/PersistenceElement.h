#ifndef PERSISTENCE_ELEMENT__
#define PERSISTENCE_ELEMENT__

#include "java/io/File.h"
#include "java/io/InputStream.h"
#include "java/io/OutputStream.h"

class PersistenceElement {
  public:
    PersistenceElement() = delete;

    static int readByteInt(java::InputStream &is);
    static int readByteUnsignedInt(java::InputStream &is);
    static void writeByte(java::OutputStream &os, unsigned char value);
    static void writeBool(java::OutputStream &os, bool value);

    static void readBytes(java::InputStream &is, unsigned char *bytesBuffer, int length);
    static void writeBytes(java::OutputStream &os, const unsigned char *bytesBuffer, int length);

    static int byteArray2signedShortBE(const unsigned char *arr, int start);
    static void signedShort2byteArrayBE(unsigned char *arr, int start, int num);
    static void signedShort2byteArrayLE(unsigned char *arr, int start, int num);
    static int byteArray2signedShortLE(const unsigned char *arr, int start);

    static long byteArray2longBE(const unsigned char *arr, int start);
    static long byteArray2longLE(const unsigned char *arr, int start);

    static float byteArray2floatBE(const unsigned char *arr, int start);
    static void float2byteArrayBE(unsigned char *arr, int start, float num);
    static void float2byteArrayLE(unsigned char *arr, int start, float num);
    static float byteArray2floatLE(const unsigned char *arr, int start);

    static double byteArray2doubleLE(const unsigned char *arr, int start);
    static double byteArray2doubleBE(const unsigned char *arr, int start);

    static int readSignedShortLE(java::InputStream &is);
    static int readSignedShortBE(java::InputStream &is);
    static void writeSignedShortBE(java::OutputStream &os, int num);
    static void writeSignedShortLE(java::OutputStream &os, int num);

    static long readLongLE(java::InputStream &is);
    static void writeInt32LE(java::OutputStream &os, int num);
    static void writeInt64LE(java::OutputStream &os, long long num);
    static void writeDoubleLE(java::OutputStream &os, double num);

    static long readLongBE(java::InputStream &is);

    static float readFloatLE(java::InputStream &is);
    static double readDoubleLE(java::InputStream &is);
    static double readDoubleBE(java::InputStream &is);
    static float readFloatBE(java::InputStream &is);

    static void writeFloatBE(java::OutputStream &os, float num);
    static void writeFloatLE(java::OutputStream &os, float num);
    static void writeLongBE(java::OutputStream &os, long num);
    static void writeLongLE(java::OutputStream &os, long num);

    static char *readAsciiFixedSizeString(java::InputStream &is, int size);
    static char *readAsciiString(java::InputStream &is);
    static char *readUtf8String(java::InputStream &is);
    static char *readUtf8Line(java::InputStream &is);
    static char *readAsciiLine(java::InputStream &is);
    static char *readAsciiToken(java::InputStream &is, const unsigned char *separators, int separatorsLength);

    static void writeAsciiString(java::OutputStream &writer, const char *cad);
    static void writeUtf8String(java::OutputStream &writer, const char *cad);
    static void writeAsciiLine(java::OutputStream &writer, const char *cad);
    static void writeUtf8Line(java::OutputStream &writer, const char *cad);
    static bool checkDirectory(const char *dirName);

  protected:
    static char *extractExtensionFromFile(const java::File &fd);

  private:
    static const bool bigEndianArchitecture;

    static unsigned char byteBuffer1byte[1];
    static unsigned char byteBuffer2byte[2];
    static unsigned char byteBuffer4byte[4];
    static unsigned char byteBuffer8byte[8];
    static unsigned char bytesForLong[4];

    static void signedShort2byteArrayDirect(unsigned char *outArrayToBeExported, int inStartIndexInsideArray, int inNumberToConvert);
    static void signedShort2byteArrayInvert(unsigned char *outArrayToBeExported, int inStartIndexInsideArray, int inNumberToConvert);

    static int byteArray2signedShortDirect(const unsigned char *arr, int start);
    static int byteArray2signedShortInvert(const unsigned char *arr, int start);

    static long byteArray2longDirect(const unsigned char *arr, int start);
    static long byteArray2longInvert(const unsigned char *arr, int start);

    static void signedInt2byteArrayDirect(unsigned char *arr, int start, long num);
    static void signedInt2byteArrayInvert(unsigned char *arr, int start, long num);
    static int signedByte2unsignedInteger(unsigned char value);

    static float byteArray2floatDirect(const unsigned char *arr, int start);
    static double byteArray2doubleDirect(const unsigned char *arr, int start);
    static float byteArray2floatInvert(const unsigned char *arr, int start);
    static double byteArray2doubleInvert(const unsigned char *arr, int start);

    static bool isInSet(unsigned char key, const unsigned char *set, int setLength);
    static bool buildUtf8Char(const unsigned char arr[2], unsigned char outBytes[2]);
    static char *duplicateCString(const char *text);
    static char *joinCString2(const char *left, const char *right);
    static char *joinCString3(const char *first, const char *second, const char *third);
    static bool containsCString(const char *text, const char *fragment);
    static bool startsWithCString(const char *text, const char *prefix);
    static bool endsWithCString(const char *text, const char *suffix);
    static bool containsExistingLibrary(const char *pathList, char pathSeparator, const char *nativeLibname);
};

#endif
