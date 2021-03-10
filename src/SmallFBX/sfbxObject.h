#pragma once
#include "sfbxTypes.h"

namespace sfbx {

enum class ObjectClass : int
{
    Unknown,
    NodeAttribute,
    Model,
    Geometry,
    Deformer,
    Pose,
    Video,
    Material,
    AnimationStack,
    AnimationLayer,
    AnimationCurveNode,
    AnimationCurve,
};

enum class ObjectSubClass : int
{
    Unknown,
    Null,
    Root,
    LimbNode,
    Light,
    Camera,
    Mesh,
    Shape,
    BlendShape,
    BlendShapeChannel,
    Skin,
    Cluster,
    BindPose,
    Clip,
};

#define sfbxEachObjectClass(Body)\
    Body(NodeAttribute) Body(Model) Body(Geometry) Body(Deformer) Body(Pose) Body(Video) Body(Material)\
    Body(AnimationStack) Body(AnimationLayer) Body(AnimationCurveNode) Body(AnimationCurve)

#define sfbxEachObjectSubClass(Body)\
    Body(Null) Body(Root) Body(LimbNode) Body(Light) Body(Camera) Body(Mesh) Body(Shape)\
    Body(BlendShape) Body(BlendShapeChannel) Body(Skin) Body(Cluster) Body(BindPose) Body(Clip)

#define sfbxEachObjectType(Body)\
    Body(NodeAttribute) Body(NullAttribute) Body(RootAttribute) Body(LimbNodeAttribute) Body(LightAttribute) Body(CameraAttribute)\
    Body(Model) Body(Null) Body(Root) Body(LimbNode) Body(Light) Body(Camera) Body(Mesh)\
    Body(Geometry) Body(GeomMesh) Body(Shape)\
    Body(Deformer) Body(Skin) Body(Cluster) Body(BlendShape) Body(BlendShapeChannel)\
    Body(Pose) Body(BindPose)\
    Body(Video) Body(Material)\
    Body(AnimationStack) Body(AnimationLayer) Body(AnimationCurveNode) Body(AnimationCurve)

#define Decl(T) class T;
sfbxEachObjectType(Decl)
#undef Decl


ObjectClass GetObjectClass(string_view n);
ObjectClass GetObjectClass(Node* n);
string_view GetObjectClassName(ObjectClass t);
ObjectSubClass GetObjectSubClass(string_view n);
ObjectSubClass GetObjectSubClass(Node* n);
string_view GetObjectSubClassName(ObjectSubClass t);

// return full name (display name + \x00 \x01 + class name)
std::string MakeFullName(string_view display_name, string_view class_name);
// true if name is in full name (display name + \x00 \x01 + class name)
bool IsFullName(string_view name);
// split full name into display name & class name (e.g. "hoge\x00\x01Mesh" -> "hoge" & "Mesh")
bool SplitFullName(string_view full_name, string_view& display_name, string_view& class_name);


// base object class

class Object : public std::enable_shared_from_this<Object>
{
friend class Document;
public:
    virtual ~Object();
    virtual ObjectClass getClass() const;
    virtual ObjectSubClass getSubClass() const;

    template<class T> T* createChild(string_view name = {});
    virtual void addChild(Object* v);
    virtual void eraseChild(Object* v);

    int64 getID() const;
    string_view getFullName() const; // in object name format (e.g. "hoge\x00\x01Mesh")
    string_view getName() const; // return display part (e.g. "hoge" if object name is "hoge\x00\x01Mesh")
    Node* getNode() const;

    span<Object*> getParents() const;
    span<Object*> getChildren() const;
    Object* getParent(size_t i = 0) const;
    Object* getChild(size_t i = 0) const;
    Object* findChild(string_view name) const;

    void setID(int64 v);
    void setName(string_view v);
    void setNode(Node* v);

protected:
    Object();
    Object(const Object&) = delete;
    Object& operator=(const Object) = delete;

    virtual void importFBXObjects();
    virtual void exportFBXObjects();
    virtual void exportFBXConnections();
    virtual string_view getInternalClassName() const;
    virtual void addParent(Object* v);
    virtual void eraseParent(Object* v);

    Document* m_document{};
    Node* m_node{};
    int64 m_id{};
    std::string m_name;

    std::vector<Object*> m_parents;
    std::vector<Object*> m_children;
};


} // sfbx
