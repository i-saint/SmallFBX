#pragma once
#include "sfbxObject.h"
#
namespace sfbx {

enum class FileVersion : int
{
    Unknown = 0,

    Fbx2014 = 7400,
    Fbx2015 = Fbx2014,

    Fbx2016 = 7500,
    Fbx2017 = Fbx2016,
    Fbx2018 = Fbx2016,

    Fbx2019 = 7700,
    Fbx2020 = Fbx2019,

    Default = Fbx2020,
};

class Document
{
public:
    Document();
    explicit Document(std::istream& is);
    explicit Document(const std::string& path);
    bool valid() const;

    bool read(std::istream& is);
    bool read(const std::string& path);
    bool writeBinary(std::ostream& os) const;
    bool writeBinary(const std::string& path) const;
    bool writeAscii(std::ostream& os) const;
    bool writeAscii(const std::string& path) const;

    FileVersion getFileVersion() const;
    void setFileVersion(FileVersion v);

    // T: Mesh, Camera, Light, LimbNode, Skin, etc.
    // refer TestFBX.cpp for how to use. or refer sfbxEachObjectType() in sfbxObject.h for complete type list.
    template<class T>
    T* createObject(string_view name = {});

    Object* findObject(int64 id) const;
    // name accepts both full name and display name. (see MakeFullName() etc)
    Object* findObject(string_view name) const;
    span<ObjectPtr> getAllObjects() const;
    span<Object*> getRootObjects() const;
    Model* getRootModel() const;

    span<AnimationStack*> getAnimationStacks() const;
    AnimationStack* findAnimationStack(string_view name) const;
    AnimationStack* getCurrentTake() const;
    void setCurrentTake(AnimationStack* v);

    bool mergeAnimations(Document* doc);
    bool mergeAnimations(DocumentPtr doc) { return mergeAnimations(doc.get()); }
    bool mergeAnimations(std::istream& input);
    bool mergeAnimations(const std::string& path);

    void exportFBXNodes();

    // utils
    template<class T>
    size_t countObjects() const
    {
        return count(m_objects, [](auto& p) { return as<T>(p.get()) && p->getID() != 0; });
    }


public:
    // internal

    void unload();
    bool readAscii(std::istream& is);
    bool readBinary(std::istream& is);

    Node* createNode(string_view name = {});
    Node* createChildNode(string_view name = {});
    void eraseNode(Node* n);
    Node* findNode(string_view name) const;
    span<NodePtr> getAllNodes() const;
    span<Node*> getRootNodes() const;

    Object* createObject(ObjectClass t, ObjectSubClass s);
    void addObject(ObjectPtr obj, bool check = false);
    void eraseObject(Object* obj);

    void createLinkOO(Object* child, Object* parent);
    void createLinkOP(Object* child, Object* parent, string_view target);

private:
    void initialize();
    void importFBXObjects();

    FileVersion m_version = FileVersion::Default;

    std::vector<NodePtr> m_nodes;
    std::vector<Node*> m_root_nodes;

    std::vector<ObjectPtr> m_objects;
    std::vector<Object*> m_root_objects;
    std::vector<AnimationStack*> m_anim_stacks;
    Model* m_root_model{};
    AnimationStack* m_current_take{};
};

template<class... T>
inline DocumentPtr MakeDocument(T&&... v)
{
    return std::make_shared<Document>(std::forward<T>(v)...);
}

} // namespace sfbx
