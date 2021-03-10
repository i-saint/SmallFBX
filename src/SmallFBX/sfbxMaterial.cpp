#include "pch.h"
#include "sfbxInternal.h"
#include "sfbxMaterial.h"

namespace sfbx {

ObjectClass Video::getClass() const { return ObjectClass::Video; }
ObjectSubClass Video::getSubClass() const { return ObjectSubClass::Clip; }

void Video::importFBXObjects()
{
    super::importFBXObjects();
    // todo
}

void Video::exportFBXObjects()
{
    super::exportFBXObjects();
    // todo
}



ObjectClass Material::getClass() const { return ObjectClass::Material; }

void Material::importFBXObjects()
{
    super::importFBXObjects();
    // todo
}

void Material::exportFBXObjects()
{
    super::exportFBXObjects();
    // todo
}
} // namespace sfbx
