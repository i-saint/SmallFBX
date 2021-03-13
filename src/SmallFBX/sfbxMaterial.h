#pragma once
#include "sfbxObject.h"

namespace sfbx {

// texture & material

// Video represents image data
class Video : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;
    ObjectSubClass getSubClass() const override;
};

class Texture : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;
};

class Material : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;

protected:
    void importFBXObjects() override;
    void exportFBXObjects() override;
};


class Implementation : public Object
    {
using super = Object;
public:
    ObjectClass getClass() const override;
};

class BindingTable : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;
};

} // namespace sfbx
