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
    bool read(std::istream &input);
    bool read(const std::string& path);
    bool writeBinary(std::ostream& output);
    bool writeBinary(const std::string& path);
    bool writeAscii(std::ostream& output);
    bool writeAscii(const std::string& path);
    void unload();

    FileVersion getVersion();
    void setVersion(FileVersion v);

    Node* createNode(string_view name = {});
    Node* createChildNode(string_view name = {});
    void eraseNode(Node* n);
    Node* findNode(string_view name) const;
    span<NodePtr> getAllNodes() const;
    span<Node*> getRootNodes() const;

    void createLinkOO(Object* child, Object* parent);
    void createLinkOP(Object* child, Object* parent, string_view target);


    Object* createObject(ObjectClass t, ObjectSubClass s);
    template<class T> T* createObject(string_view name = {});
    void addObject(ObjectPtr obj);
    void eraseObject(Object* objv);

    Object* findObject(int64 id) const;
    Object* findObject(string_view name) const; // name must be in node name format (e.g. "hoge\x00\x01Mesh")
    span<ObjectPtr> getAllObjects() const;
    span<Object*> getRootObjects() const;
    Model* getRootModel() const;

    span<AnimationStack*> getAnimationStacks() const;
    AnimationStack* findAnimationStack(string_view name) const;

    AnimationStack* getCurrentTake() const;
    void setCurrentTake(AnimationStack* v);

    void constructNodes();
    std::string toString();


    // utils
    template<class T>
    size_t countObjects() const
    {
        return count(m_objects, [](auto& p) { return as<T>(p.get()) && p->getID() != 0; });
    }

private:
    void initialize();

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
