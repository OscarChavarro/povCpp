#ifndef __VSDK_TOOLKIT_MEDIA_MEDIAENTITY_H__
#define __VSDK_TOOLKIT_MEDIA_MEDIAENTITY_H__

/**
A `MediaEntity` in VitralSDK is a software element with data
structures associated with multimedia information, and some minor basic
algorithms for supporting them.

The MediaEntity abstract class provides an interface for multimedia related
classes NOT related with 3D geometry. This serves two purposes:
  - To help in design level organization of multimedia related classes (this
    eases the study of the class hierarchy)
  - To provide a place to locate possible future operations, common to all
    multimedia objects. Note that currently none of such operations have been
    detected.
*/
class MediaEntity {

public:
    static const int BYTE_SIZE_IN_BYTES = 1;
    static const int INT_SIZE_IN_BYTES = 4;
    static const int LONG_SIZE_IN_BYTES = 8;
    static const int FLOAT_SIZE_IN_BYTES = 4;
    static const int DOUBLE_SIZE_IN_BYTES = 8;
    static const int VECTOR3D_SIZE_IN_BYTES = 24;
    static const int COLORRGB_SIZE_IN_BYTES = 24;
    static const int POINTER_SIZE_IN_BYTES = 8;

    virtual ~MediaEntity() = default;

    virtual int getSizeInBytes() const {
        return 0;
    }

};

#endif // __VSDK_TOOLKIT_MEDIA_MEDIAENTITY_H__
