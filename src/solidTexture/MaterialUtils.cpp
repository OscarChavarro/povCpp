/**
Material utilities: global default texture management.
*/

#include "solidTexture/MaterialUtils.h"
#include "solidTexture/Material.h"

static Material *defaultTextureInstance;

MaterialUtils* MaterialUtils::materialInstance = nullptr;

MaterialUtils::MaterialUtils()
{
}

void
MaterialUtils::initialize()
{
    static MaterialUtils inst;
    materialInstance = &inst;
}

MaterialUtils&
MaterialUtils::instance()
{
    return *materialInstance;
}

Material *
MaterialUtils::defaultTexture()
{
    return defaultTextureInstance;
}

void
MaterialUtils::setDefaultTexture(Material *texture)
{
    defaultTextureInstance = texture;
}
