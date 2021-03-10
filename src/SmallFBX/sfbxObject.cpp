#include "pch.h"
#include "sfbxInternal.h"
#include "sfbxObject.h"
#include "sfbxModel.h"
#include "sfbxGeometry.h"
#include "sfbxDeformer.h"
#include "sfbxMaterial.h"
#include "sfbxAnimation.h"
#include "sfbxDocument.h"

namespace sfbx {

ObjectClass GetObjectClass(string_view n)
{
    if (n.empty()) {
        return ObjectClass::Unknown;
    }
#define Case(T) else if (n == sfbxS_##T) { return ObjectClass::T; }
    sfbxEachObjectClass(Case)
#undef Case
    else {
        sfbxPrint("GetFbxObjectClass(): unknown type \"%s\"\n", std::string(n).c_str());
        return ObjectClass::Unknown;
    }
}
ObjectClass GetObjectClass(Node* n)
{
    return GetObjectClass(n->getName());
}

string_view GetObjectClassName(ObjectClass t)
{
    switch (t) {
#define Case(T) case ObjectClass::T: return sfbxS_##T;
        sfbxEachObjectClass(Case)
#undef Case
    default:
        return "";
    }
}


ObjectSubClass GetObjectSubClass(string_view n)
{
    if (n.empty()) {
        return ObjectSubClass::Unknown;
    }
#define Case(T) else if (n == sfbxS_##T) { return ObjectSubClass::T; }
    sfbxEachObjectSubClass(Case)
#undef Case
    else {
        sfbxPrint("GetFbxObjectSubClass(): unknown subtype \"%s\"\n", std::string(n).c_str());
        return ObjectSubClass::Unknown;
    }
}

ObjectSubClass GetObjectSubClass(Node* n)
{
    if (GetPropertyCount(n) == 3)
        return GetObjectSubClass(GetPropertyString(n, 2));
#ifdef sfbxEnableLegacyFormatSupport
    else if (GetPropertyCount(n) == 2)
        return GetObjectSubClass(GetPropertyString(n, 1));
#endif
    else
        return ObjectSubClass::Unknown;
}

string_view GetObjectSubClassName(ObjectSubClass t)
{
    switch (t) {
#define Case(T) case ObjectSubClass::T: return sfbxS_##T;
        sfbxEachObjectSubClass(Case)
#undef Case
    default: return "";
    }
}


static constexpr inline string_view GetInternalObjectClassName(ObjectClass t, ObjectSubClass st)
{
#define Case1(T, ST, Ret) if (t == ObjectClass::T && st == ObjectSubClass::ST) return Ret
#define Case2(T, Ret) if (t == ObjectClass::T) return Ret

    Case1(Deformer, Cluster, sfbxS_SubDeformer);
    Case1(Deformer, BlendShapeChannel, sfbxS_SubDeformer);
    Case2(AnimationStack, sfbxS_AnimStack);
    Case2(AnimationLayer, sfbxS_AnimLayer);
    Case2(AnimationCurveNode, sfbxS_AnimCurveNode);
    Case2(AnimationCurve, sfbxS_AnimCurve);

#undef Case1
#undef Case2

    return GetObjectClassName(t);
}


std::string MakeFullName(string_view display_name, string_view class_name)
{
    std::string ret;
    size_t pos = display_name.find('\0'); // ignore class name part
    if (pos == std::string::npos)
        ret = display_name;
    else
        ret.assign(display_name.data(), pos);

    ret += (char)0x00;
    ret += (char)0x01;
    ret += class_name;
    return ret;
}

bool IsFullName(string_view name)
{
    size_t n = name.size();
    if (n > 2) {
        for (size_t i = 0; i < n - 1; ++i)
            if (name[i] == 0x00 && name[i + 1] == 0x01)
                return true;
    }
    return false;
}

bool SplitFullName(string_view full_name, string_view& display_name, string_view& class_name)
{
    const char* str = full_name.data();
    size_t n = full_name.size();
    if (n > 2) {
        for (size_t i = 0; i < n - 1; ++i) {
            if (str[i] == 0x00 && str[i + 1] == 0x01) {
                display_name = string_view(str, i);
                i += 2;
                class_name = string_view(str + i, n - i);
                return true;
            }
        }
    }
    display_name = string_view(full_name);
    return false;
}



Object::Object()
{
    m_id = (int64)this;
}

Object::~Object()
{
}

ObjectClass Object::getClass() const { return ObjectClass::Unknown; }
ObjectSubClass Object::getSubClass() const { return ObjectSubClass::Unknown; }
string_view Object::getInternalClassName() const { return GetInternalObjectClassName(getClass(), getSubClass()); }

void Object::setNode(Node* n)
{
    m_node = n;
    if (n) {
        // do these in constructObject() is too late because of referencing other objects...
        size_t cprops = GetPropertyCount(n);
        if (cprops == 3) {
            m_id = GetPropertyValue<int64>(n, 0);
            m_name = GetPropertyString(n, 1);
        }
#ifdef sfbxEnableLegacyFormatSupport
        else if (cprops == 2) {
            // no ID in legacy format
            m_name = GetPropertyString(n, 0);
        }
#endif
    }
}

void Object::importFBXObjects()
{
}

void Object::exportFBXObjects()
{
    if (m_id == 0)
        return;

    auto objects = m_document->findNode(sfbxS_Objects);
    m_node = objects->createChild(
        GetObjectClassName(getClass()), m_id, getFullName(), GetObjectSubClassName(getSubClass()));
}

void Object::exportFBXConnections()
{
    for (auto parent : getParents())
        m_document->createLinkOO(this, parent);
}

template<class T> T* Object::createChild(string_view name)
{
    auto ret = m_document->createObject<T>(name);
    addChild(ret);
    return ret;
}
#define Body(T) template T* Object::createChild(string_view name);
sfbxEachObjectType(Body)
#undef Body


void Object::addChild(Object* v)
{
    if (v) {
        m_children.push_back(v);
        v->addParent(this);
    }
}

void Object::eraseChild(Object* v)
{
    if (erase(m_children, v))
        v->eraseParent(this);
}

void Object::addParent(Object* v)
{
    if (v)
        m_parents.push_back(v);
}

void Object::eraseParent(Object* v)
{
    erase(m_parents, v);
}


int64 Object::getID() const { return m_id; }
string_view Object::getFullName() const { return m_name; }
string_view Object::getName() const { return m_name.c_str(); }

Node* Object::getNode() const { return m_node; }

span<Object*> Object::getParents() const  { return make_span(m_parents); }
span<Object*> Object::getChildren() const { return make_span(m_children); }
Object* Object::getParent(size_t i) const { return i < m_parents.size() ? m_parents[i] : nullptr; }
Object* Object::getChild(size_t i) const  { return i < m_children.size() ? m_children[i] : nullptr; }

Object* Object::findChild(string_view name) const
{
    for (auto c : m_children)
        if (c->getFullName() == name)
            return c;
    return nullptr;
}

void Object::setID(int64 id) { m_id = id; }
void Object::setName(string_view v) { m_name = MakeFullName(v, getInternalClassName()); }


} // namespace sfbx
