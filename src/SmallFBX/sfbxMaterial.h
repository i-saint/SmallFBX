#pragma once
#include "sfbxObject.h"

namespace sfbx {

// texture & material

// Video represents texture data
class Video : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;
    ObjectSubClass getSubClass() const override;

protected:
    void importFBXObjects() override;
    void exportFBXObjects() override;
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


} // namespace sfbx
