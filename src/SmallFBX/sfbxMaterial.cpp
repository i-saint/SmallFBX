#include "pch.h"
#include "sfbxInternal.h"
#include "sfbxMaterial.h"

namespace sfbx {

ObjectClass Video::getClass() const { return ObjectClass::Video; }
ObjectSubClass Video::getSubClass() const { return ObjectSubClass::Clip; }


ObjectClass Texture::getClass() const { return ObjectClass::Texture; }



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


ObjectClass Implementation::getClass() const { return ObjectClass::Implementation; }

ObjectClass BindingTable::getClass() const { return ObjectClass::BindingTable; }

} // namespace sfbx
