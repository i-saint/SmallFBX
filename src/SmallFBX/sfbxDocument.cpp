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

static const char* g_creator = "SmallFBX 1.0.0";

static const uint8_t g_fbx_header_magic[23]{
    'K', 'a', 'y', 'd', 'a', 'r', 'a', ' ', 'F', 'B', 'X', ' ', 'B', 'i', 'n', 'a', 'r', 'y', ' ', ' ', 0x00, 0x1a, 0x00,
};

// *** these must not be changed. it leads to CRC check failure. ***
static const char g_fbx_time_id[] = "1970-01-01 10:00:00:000";
static const uint8_t g_fbx_file_id[16]{ 0x28, 0xb3, 0x2a, 0xeb, 0xb6, 0x24, 0xcc, 0xc2, 0xbf, 0xc8, 0xb0, 0x2a, 0xa9, 0x2b, 0xfc, 0xf1 };
static const uint8_t g_fbx_footer_magic1[16]{ 0xfa, 0xbc, 0xab, 0x09, 0xd0, 0xc8, 0xd4, 0x66, 0xb1, 0x76, 0xfb, 0x83, 0x1c, 0xf7, 0x26, 0x7e };
static const uint8_t g_fbx_footer_magic2[16]{ 0xf8, 0x5a, 0x8c, 0x6a, 0xde, 0xf5, 0xd9, 0x7e, 0xec, 0xe9, 0x0c, 0xe3, 0x75, 0x8f, 0x29, 0x0b };


Document::Document()
{
    initialize();
}

void Document::initialize()
{
    m_root_model = createObject<Model>("Scene");
    m_root_model->setID(0);
}

bool Document::read(std::istream& is)
{
    unload();
    initialize();

    char magic[std::size(g_fbx_header_magic)];
    readv(is, magic, std::size(g_fbx_header_magic));
    if (memcmp(magic, g_fbx_header_magic, std::size(g_fbx_header_magic)) != 0) {
        sfbxPrint("sfbx::Document::read(): not a fbx file\n");
        return false;
    }
    m_version = (FileVersion)read1<uint32_t>(is);


    try {
        uint64_t pos = std::size(g_fbx_header_magic) + 4;
        for (;;) {
            auto node = createNode();
            pos += node->read(is, pos);
            if (node->isNull()) {
                eraseNode(node);
                break;
            }
        }

        if (Node* objects = findNode(sfbxS_Objects)) {
            for (Node* n : objects->getChildren()) {
                if (Object* obj = createObject(GetObjectClass(n), GetObjectSubClass(n))) {
                    obj->setNode(n);
                }
            }
        }

        if (Node* connections = findNode(sfbxS_Connections)) {
            for (Node* n : connections->getChildren()) {
                auto name = n->getName();
                auto ct = GetPropertyString(n, 0);
                if (name == sfbxS_C && ct == sfbxS_OO) {
                    Object* child = findObject(GetPropertyValue<int64>(n, 1));
                    Object* parent = findObject(GetPropertyValue<int64>(n, 2));
                    if (child && parent)
                        parent->addChild(child);
                }
                else if (name == sfbxS_C && ct == sfbxS_OP) {
                    auto name = GetPropertyString(n, 3); // todo
                    Object* child = findObject(GetPropertyValue<int64>(n, 1));
                    Object* parent = findObject(GetPropertyValue<int64>(n, 2));
                    if (child && parent) {
                        parent->addChild(child);
                    }
                }
#ifdef sfbxEnableLegacyFormatSupport
                else if (name == sfbxS_Connect && ct == sfbxS_OO) {
                    Object* child = findObject(GetPropertyString(n, 1));
                    Object* parent = findObject(GetPropertyString(n, 2));
                    if (child && parent)
                        parent->addChild(child);
                }
#endif
                else {
                    sfbxPrint("sfbx::Document::read(): unrecognized connection type %s %s\n",
                        std::string(name).c_str(), std::string(ct).c_str());
                }
            }
        }

        // index based loop because m_objects maybe push_backed in the loop
        for (size_t i = 0; i < m_objects.size(); ++i) {
            auto obj = m_objects[i];
            obj->importFBXObjects();
            if (obj->getParents().empty())
                m_root_objects.push_back(obj.get());
        }

        if (Node* takes = findNode(sfbxS_Takes)) {
            auto current = GetChildPropertyString(takes, sfbxS_Current);
            if (auto* t = findAnimationStack(current))
                m_current_take = t;
        }
    }
    catch (const std::runtime_error& e) {
        sfbxPrint("sfbx::Document::read(): exception %s\n", e.what());
        return false;
    }
    return true;
}

bool Document::read(const std::string& path)
{
    unload();

    std::ifstream file;
    file.open(path, std::ios::in | std::ios::binary);
    if (file)
        return read(file);
    return false;
}


bool Document::writeBinary(std::ostream& os)
{
    writev(os, g_fbx_header_magic);
    writev(os, m_version);

    uint64_t pos = std::size(g_fbx_header_magic) + 4;
    for (Node* node : m_root_nodes)
        pos += node->write(os, pos);
    {
        Node null_node;
        null_node.m_document = this;
        pos += null_node.write(os, pos);
    }

    // footer

    writev(os, g_fbx_footer_magic1);
    pos += std::size(g_fbx_footer_magic1);

    // add padding to 16 byte align
    uint64_t pad = 16 - (pos % 16);
    for (uint64_t i = 0; i < pad; ++i)
        writev(os, (int8)0);

    writev(os, (int32)0);
    writev(os, m_version);

    // 120 byte space
    for (uint64_t i = 0; i < 120; ++i)
        writev(os, (int8)0);

    writev(os, g_fbx_footer_magic2);

    return true;
}

bool Document::writeBinary(const std::string& path)
{
    std::ofstream file;
    file.open(path, std::ios::out | std::ios::binary);
    if (file)
        return writeBinary(file);
    return false;
}

bool Document::writeAscii(std::ostream& output)
{
    auto data = toString();
    output.write(data.data(), data.size());
    return true;
}

bool Document::writeAscii(const std::string& path)
{
    std::ofstream file;
    file.open(path, std::ios::out | std::ios::binary);
    if (file)
        return writeAscii(file);
    return false;
}



void Document::unload()
{
    m_version = FileVersion::Default;
    m_nodes.clear();
    m_root_nodes.clear();
    m_objects.clear();
    m_root_objects.clear();
    m_root_model = {};
}

FileVersion Document::getVersion()
{
    return m_version;
}


void Document::setVersion(FileVersion v)
{
    m_version = v;
}


Node* Document::createNode(string_view name)
{
    auto n = createChildNode(name);
    m_root_nodes.push_back(n);
    return n;
}
Node* Document::createChildNode(string_view name)
{
    auto n = new Node();
    n->m_document = this;
    n->setName(name);
    m_nodes.push_back(NodePtr(n));
    return n;
}

void Document::eraseNode(Node* n)
{
    {
        auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
            [n](const NodePtr& p) { return p.get() == n; });
        if (it != m_nodes.end())
            m_nodes.erase(it);
    }
    {
        auto it = std::find(m_root_nodes.begin(), m_root_nodes.end(), n);
        if (it != m_root_nodes.end())
            m_root_nodes.erase(it);
    }
}

Node* Document::findNode(string_view name) const
{
    auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
        [&name](const NodePtr& p) { return p->getName() == name; });
    return it != m_nodes.end() ? it->get() : nullptr;
}

span<sfbx::NodePtr> Document::getAllNodes() const { return make_span(m_nodes); }
span<Node*> Document::getRootNodes() const { return make_span(m_root_nodes); }

void Document::createLinkOO(Object* child, Object* parent)
{
    if (!child || !parent)
        return;
    if (auto c = findNode(sfbxS_Connections))
        c->createChild(sfbxS_C, sfbxS_OO, child->getID(), parent->getID());
}

void Document::createLinkOP(Object* child, Object* parent, string_view target)
{
    if (!child || !parent)
        return;
    if (auto c = findNode(sfbxS_Connections))
        c->createChild(sfbxS_C, sfbxS_OP, child->getID(), parent->getID(), target);
}

Object* Document::createObject(ObjectClass c, ObjectSubClass s)
{
    Object* r{};
    switch (c) {
    case ObjectClass::NodeAttribute:
        switch (s) {
        case ObjectSubClass::Null: r = new NullAttribute(); break;
        case ObjectSubClass::Root: r = new RootAttribute(); break;
        case ObjectSubClass::LimbNode: r = new LimbNodeAttribute(); break;
        case ObjectSubClass::Light: r = new LightAttribute(); break;
        case ObjectSubClass::Camera: r = new CameraAttribute(); break;
        default: r = new NodeAttribute(); break;
        }
        break;
    case ObjectClass::Model:
        switch (s) {
        case ObjectSubClass::Null: r = new Null(); break;
        case ObjectSubClass::Root: r = new Root(); break;
        case ObjectSubClass::LimbNode: r = new LimbNode(); break;
        case ObjectSubClass::Mesh: r = new Mesh(); break;
        case ObjectSubClass::Light: r = new Light(); break;
        case ObjectSubClass::Camera: r = new Camera(); break;
        default: r = new Model(); break;
        }
        break;
    case ObjectClass::Geometry:
        switch (s) {
        case ObjectSubClass::Mesh: r = new GeomMesh(); break;
        case ObjectSubClass::Shape: r = new Shape(); break;
        default: r = new Geometry(); break;
        }
        break;
    case ObjectClass::Deformer:
        switch (s) {
        case ObjectSubClass::Skin: r = new Skin(); break;
        case ObjectSubClass::Cluster: r = new Cluster(); break;
        case ObjectSubClass::BlendShape: r = new BlendShape(); break;
        case ObjectSubClass::BlendShapeChannel: r = new BlendShapeChannel(); break;
        default: r = new Deformer(); break;
        }
        break;
    case ObjectClass::Pose:
        switch (s) {
        case ObjectSubClass::BindPose: r = new BindPose(); break;
        default: r = new Pose(); break;
        }
        break;
    case ObjectClass::Video:             r = new Video(); break;
    case ObjectClass::Material:          r = new Material(); break;
    case ObjectClass::AnimationStack:    r = new AnimationStack(); break;
    case ObjectClass::AnimationLayer:    r = new AnimationLayer(); break;
    case ObjectClass::AnimationCurveNode:r = new AnimationCurveNode(); break;
    case ObjectClass::AnimationCurve:    r = new AnimationCurve(); break;
    default: break;
    }

    if (r) {
        addObject(ObjectPtr(r));
    }
    else {
        sfbxPrint("sfbx::Document::createObject(): unrecongnized type \"%s\"\n", GetObjectClassName(c).data());
    }
    return r;
}

template<class T>
T* Document::createObject(string_view name)
{
    T* r = new T();
    r->setName(name);
    addObject(ObjectPtr(r));
    return r;
}

#define Body(T) template T* Document::createObject(string_view name);
sfbxEachObjectType(Body)
#undef Body

void Document::addObject(ObjectPtr obj)
{
    if (obj) {
        m_objects.push_back(obj);
        if (auto take = as<AnimationStack>(obj.get()))
            m_anim_stacks.push_back(take);
        obj->m_document = this;
    }
}


void Document::eraseObject(Object* obj)
{
    {
        auto it = std::find_if(m_objects.begin(), m_objects.end(),
            [obj](const ObjectPtr& p) { return p.get() == obj; });
        if (it != m_objects.end())
            m_objects.erase(it);
    }
    {
        auto it = std::find(m_root_objects.begin(), m_root_objects.end(), obj);
        if (it != m_root_objects.end())
            m_root_objects.erase(it);
    }
    {
        auto it = std::find(m_anim_stacks.begin(), m_anim_stacks.end(), obj);
        if (it != m_anim_stacks.end())
            m_anim_stacks.erase(it);
    }
}

Object* Document::findObject(int64 id) const
{
    auto it = std::find_if(m_objects.begin(), m_objects.end(),
        [id](auto& p) { return p->getID() == id; });
    return it != m_objects.end() ? it->get() : nullptr;
}

Object* Document::findObject(string_view name) const
{
    auto it = std::find_if(m_objects.begin(), m_objects.end(),
        [&name](auto& p) { return p->getName() == name; });
    return it != m_objects.end() ? it->get() : nullptr;
}

span<ObjectPtr> Document::getAllObjects() const { return make_span(m_objects); }
span<Object*> Document::getRootObjects() const { return make_span(m_root_objects); }
Model* Document::getRootModel() const { return m_root_model; }

span<AnimationStack*> Document::getAnimationStacks() const
{
    return make_span(m_anim_stacks);
}

AnimationStack* Document::findAnimationStack(string_view name) const
{
    for (auto take : m_anim_stacks)
        if (take->getName() == name || take->getFullName() == name)
            return take;
    return nullptr;
}

AnimationStack* Document::getCurrentTake() const { return m_current_take; }
void Document::setCurrentTake(AnimationStack* v) { m_current_take = v; }

void Document::constructNodes()
{
    m_nodes.clear();
    m_root_nodes.clear();

    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);
    std::string take_name{ m_current_take ? m_current_take->getName() : "" };

    auto header_extension = createNode(sfbxS_FBXHeaderExtension);
    {
        header_extension->createChild(sfbxS_FBXHeaderVersion, (int32_t)1003);
        header_extension->createChild(sfbxS_FBXVersion, (int32_t)getVersion());
        header_extension->createChild(sfbxS_EncryptionType, (int32_t)0);
        {
            auto timestamp = header_extension->createChild(sfbxS_CreationTimeStamp);
            timestamp->createChild(sfbxS_Version, 1000);
            timestamp->createChild(sfbxS_Year, now->tm_year);
            timestamp->createChild(sfbxS_Month, now->tm_mon);
            timestamp->createChild(sfbxS_Day, now->tm_mday);
            timestamp->createChild(sfbxS_Hour, now->tm_hour);
            timestamp->createChild(sfbxS_Minute, now->tm_min);
            timestamp->createChild(sfbxS_Second, now->tm_sec);
            timestamp->createChild(sfbxS_Millisecond, 0);
        }
        header_extension->createChild(sfbxS_Creator, g_creator);
        {
            auto other_flags = header_extension->createChild(sfbxS_OtherFlags);
            other_flags->createChild(sfbxS_TCDefinition, sfbxI_TCDefinition);
        }
        {
            auto scene_info = header_extension->createChild(sfbxS_SceneInfo, MakeFullName(sfbxS_GlobalInfo, sfbxS_SceneInfo), sfbxS_UserData);
            scene_info->createChild(sfbxS_Type, sfbxS_UserData);
            scene_info->createChild(sfbxS_Version, 100);
            {
                auto meta = scene_info->createChild(sfbxS_MetaData);
                meta->createChild(sfbxS_Version, 100);
                meta->createChild(sfbxS_Title, "");
                meta->createChild(sfbxS_Subject, "");
                meta->createChild(sfbxS_Author, "");
                meta->createChild(sfbxS_Keywords, "");
                meta->createChild(sfbxS_Revision, "");
                meta->createChild(sfbxS_Comment, "");
            }
            {
                auto prop = scene_info->createChild(sfbxS_Properties70);
                prop->createChild(sfbxS_P, sfbxS_DocumentUrl, sfbxS_KString, sfbxS_Url, "", "a.fbx");
                prop->createChild(sfbxS_P, sfbxS_SrcDocumentUrl, sfbxS_KString, sfbxS_Url, "", "a.fbx");
                prop->createChild(sfbxS_P, sfbxS_Original, sfbxS_Compound, "", "");
                prop->createChild(sfbxS_P, sfbxS_OriginalApplicationVendor, sfbxS_KString, "", "", "");
                prop->createChild(sfbxS_P, sfbxS_OriginalApplicationName, sfbxS_KString, "", "", "");
                prop->createChild(sfbxS_P, sfbxS_OriginalApplicationVersion, sfbxS_KString, "", "", "");
                prop->createChild(sfbxS_P, sfbxS_OriginalDateTime_GMT, sfbxS_DateTime, "", "", "");
                prop->createChild(sfbxS_P, sfbxS_OriginalFileName, sfbxS_KString, "", "", "");
                prop->createChild(sfbxS_P, sfbxS_LastSaved, sfbxS_Compound, "", "");
                prop->createChild(sfbxS_P, sfbxS_LastSavedApplicationVendor, sfbxS_KString, "", "", "");
                prop->createChild(sfbxS_P, sfbxS_LastSavedApplicationName, sfbxS_KString, "", "", "");
                prop->createChild(sfbxS_P, sfbxS_LastSavedApplicationVersion, sfbxS_KString, "", "", "");
                prop->createChild(sfbxS_P, sfbxS_LastSavedDateTime_GMT, sfbxS_DateTime, "", "", "");
            }
        }
    }

    createNode(sfbxS_FileId)->addProperties(make_span(g_fbx_file_id));
    createNode(sfbxS_CreationTime)->addProperties(g_fbx_time_id);
    createNode(sfbxS_Creator)->addProperties(g_creator);

    auto global_settings = createNode(sfbxS_GlobalSettings);
    {
        global_settings->createChild(sfbxS_Version, sfbxI_GlobalSettingsVersion);
        auto prop = global_settings->createChild(sfbxS_Properties70);
        prop->createChild(sfbxS_P, "UpAxis", "int", "Integer", "", 1);
        prop->createChild(sfbxS_P, "UpAxisSign", "int", "Integer", "", 1);
        prop->createChild(sfbxS_P, "FrontAxis", "int", "Integer", "", 2);
        prop->createChild(sfbxS_P, "FrontAxisSign", "int", "Integer", "", 1);
        prop->createChild(sfbxS_P, "CoordAxis", "int", "Integer", "", 0);
        prop->createChild(sfbxS_P, "CoordAxisSign", "int", "Integer", "", 1);
        prop->createChild(sfbxS_P, "OriginalUpAxis", "int", "Integer", "", -1);
        prop->createChild(sfbxS_P, "OriginalUpAxisSign", "int", "Integer", "", 1);
        prop->createChild(sfbxS_P, "UnitScaleFactor", "double", "Number", "", 1.000000);
        prop->createChild(sfbxS_P, "OriginalUnitScaleFactor", "double", "Number", "", 1.000000);
        prop->createChild(sfbxS_P, "AmbientColor", "ColorRGB", "Color", "", 0.000000, 0.000000, 0.000000);
        prop->createChild(sfbxS_P, "DefaultCamera", "KString", "", "", "Producer Perspective");
        prop->createChild(sfbxS_P, "TimeMode", "enum", "", "", 0);
        prop->createChild(sfbxS_P, "TimeProtocol", "enum", "", "", 2);
        prop->createChild(sfbxS_P, "SnapOnFrameMode", "enum", "", "", 0);
        prop->createChild(sfbxS_P, "TimeSpanStart", "KTime", "Time", "", (int64)0);
        prop->createChild(sfbxS_P, "TimeSpanStop", "KTime", "Time", "", (int64)sfbxI_TicksPerSecond);
        prop->createChild(sfbxS_P, "CustomFrameRate", "double", "Number", "", -1.000000);
        prop->createChild(sfbxS_P, "TimeMarker", "Compound", "", "");
        prop->createChild(sfbxS_P, "CurrentTimeMarker", "int", "Integer", "", -1);
    }

    auto documents = createNode(sfbxS_Documents);
    {
        documents->createChild(sfbxS_Count, (int32)1);
        auto doc = documents->createChild(sfbxS_Document);
        {
            doc->addProperties((int64)this, "My Scene", "Scene");

            auto prop = doc->createChild(sfbxS_Properties70);
            prop->createChild(sfbxS_P, "SourceObject", "object", "", "");
            prop->createChild(sfbxS_P, "ActiveAnimStackName", "KString", "", "", take_name);

            doc->createChild(sfbxS_RootNode, 0);
        }
    }

    auto references = createNode(sfbxS_References);

    auto definitions = createNode(sfbxS_Definitions);

    createNode(sfbxS_Objects);
    createNode(sfbxS_Connections);

    // index based loop because m_objects maybe push_backed in the loop
    for (size_t i = 0; i < m_objects.size(); ++i)
        m_objects[i]->exportFBXObjects();
    for (size_t i = 0; i < m_objects.size(); ++i)
        m_objects[i]->exportFBXConnections();

    {
        auto add_object_type = [definitions](size_t n, const char* type) -> Node* {
            if (n == 0)
                return nullptr;
            auto ot = definitions->createChild(sfbxS_ObjectType);
            ot->addProperty(type);
            ot->createChild(sfbxS_Count, (int32)n);
            return ot;
        };

        add_object_type(1, sfbxS_GlobalSettings);

        add_object_type(countObjects<NodeAttribute>(), sfbxS_NodeAttribute);
        add_object_type(countObjects<Model>(), sfbxS_Model);
        add_object_type(countObjects<Geometry>(), sfbxS_Geometry);
        add_object_type(countObjects<Deformer>(), sfbxS_Deformer);
        add_object_type(countObjects<Pose>(), sfbxS_Pose);

        add_object_type(countObjects<AnimationStack>(), sfbxS_AnimationStack);
        add_object_type(countObjects<AnimationLayer>(), sfbxS_AnimationLayer);
        add_object_type(countObjects<AnimationCurveNode>(), sfbxS_AnimationCurveNode);
        add_object_type(countObjects<AnimationCurve>(), sfbxS_AnimationCurve);

        add_object_type(countObjects<Material>(), sfbxS_Material);
    }

    auto takes = createNode(sfbxS_Takes);
    takes->createChild(sfbxS_Current, take_name);
    for (auto* t : m_anim_stacks) {
        auto take = takes->createChild(sfbxS_Take, t->getName());
        take->createChild(sfbxS_FileName, std::string(t->getName()) + ".tak");

        float lstart = t->getLocalStart();
        float lstop = t->getLocalStop();
        float rstart = t->getReferenceStart();
        float rstop = t->getReferenceStop();
        if (lstart != 0.0f || lstop != 0.0f)
            take->createChild(sfbxS_LocalTime, ToTicks(lstart), ToTicks(lstop));
        if (rstart != 0.0f || rstop != 0.0f)
            take->createChild(sfbxS_ReferenceTime, ToTicks(rstart), ToTicks(rstop));
    }
}

std::string Document::toString()
{
    char version[128];
    sprintf(version, "; FBX %d.%d.0 project file\n", (int)m_version / 1000 % 10, (int)m_version / 100 % 10);

    std::string s;
    s += version;
    s += "; ----------------------------------------------------\n\n";

    for (auto node : getRootNodes()) {
        // these nodes seem required only in binary format.
        if (node->getName() == sfbxS_FileId ||
            node->getName() == sfbxS_CreationTime ||
            node->getName() == sfbxS_Creator)
            continue;
        s += node->toString();
    }
    return s;
}

} // namespace sfbx
