#include "pch.h"
#include "sfbxInternal.h"
#include "sfbxNode.h"
#include "sfbxDocument.h"

namespace sfbx {

Node::Node()
{
}

uint64_t Node::read(std::istream& is, uint64_t start_offset)
{
    uint64_t ret = 0;

    uint64_t end_offset, num_props, prop_size;
    if (getDocumentVersion() >= sfbxI_FBX2016_FileVersion) {
        // size records are 64bit since FBX 2016
        end_offset = read1<uint64_t>(is);
        num_props = read1<uint64_t>(is);
        prop_size = read1<uint64_t>(is);
        ret += 24;
    }
    else {
        end_offset = read1<uint32_t>(is);
        num_props = read1<uint32_t>(is);
        prop_size = read1<uint32_t>(is);
        ret += 12;
    }

    uint8_t name_len = read1<uint8_t>(is);
    readv(is, m_name, name_len);
    ret += 1;
    ret += name_len;

    reserveProperties(num_props);
    for (uint32_t i = 0; i < num_props; i++)
        createProperty()->read(is);
    ret += prop_size;

    while (start_offset + ret < end_offset) {
        auto child = createChild();
        ret += child->read(is, start_offset + ret);
        if (child->isNull())
            eraseChild(child);
    }
    return ret;
}

uint64_t Node::write(std::ostream& os, uint64_t start_offset)
{
    uint32_t header_size = getHeaderSize() + m_name.size();
    if (isNull()) {
        for (uint32_t i = 0; i < header_size; i++)
            writev(os, (uint8_t)0);
        return header_size;
    }

    Node null_node;
    null_node.m_document = m_document;

    uint64_t property_size = 0;
    uint64_t children_size = 0;
    bool null_terminate = !m_children.empty() || m_properties.empty();
    {
        CounterStream cs;
        for (auto& prop : m_properties)
            prop.write(cs);
        property_size = cs.size();
    }
    {
        CounterStream cs;
        for (auto child : m_children)
            child->write(cs, 0);
        if (null_terminate)
            null_node.write(cs, 0);
        children_size = cs.size();
    }

    uint64_t end_offset = start_offset + header_size + property_size + children_size;
    if (getDocumentVersion() >= sfbxI_FBX2016_FileVersion) {
        // size records are 64bit since FBX 2016
        writev(os, uint64_t(end_offset));
        writev(os, uint64_t(m_properties.size()));
        writev(os, uint64_t(property_size));
    }
    else {
        writev(os, uint32_t(end_offset));
        writev(os, uint32_t(m_properties.size()));
        writev(os, uint32_t(property_size));
    }
    writev(os, uint8_t(m_name.size()));
    writev(os, m_name);

    for (auto& prop : m_properties)
        prop.write(os);

    uint64_t pos = header_size + property_size;
    for (auto child : m_children)
        pos += child->write(os, start_offset + pos);
    if (null_terminate)
        pos += null_node.write(os, start_offset + pos);
    return pos;
}

bool Node::isNull() const
{
    return m_name.empty() && m_children.empty() && m_properties.empty();
}

bool Node::isRoot() const
{
    return m_parent == nullptr;
}

void Node::setName(string_view v)
{
    m_name = v;
}

void Node::reserveProperties(size_t v)
{
    m_properties.reserve(v);
}

Property* Node::createProperty()
{
    m_properties.emplace_back();
    return &m_properties.back();
}

Node* Node::createChild(string_view name)
{
    auto p = m_document->createChildNode(name);
    m_children.push_back(p);
    p->m_parent = this;
    return p;
}

void Node::eraseChild(Node* n)
{
    m_document->eraseNode(n);

    auto it = std::find(m_children.begin(), m_children.end(), n);
    if (it != m_children.end())
        m_children.erase(it);
}

string_view Node::getName() const
{
    return m_name;
}

span<Property> Node::getProperties() const
{
    return make_span(m_properties);
}

Property* Node::getProperty(size_t i)
{
    if (i < m_properties.size())
        return &m_properties[i];
    return nullptr;
}

#ifdef sfbxEnableLegacyFormatSupport

template<class S, class D>
static inline void ToArray(span<Property> props, RawVector<D>& dst)
{
    dst.resize(props.size() / get_vector_size<D>);
    auto* d = (get_scalar_t<D>*)dst.data();
    for (auto& prop : props)
        *d++ = prop.getValue<get_scalar_t<S>>();
}

#define Def(S, D)\
    template<> void Node::getPropertiesValues<S, D>(RawVector<D>& dst) const { ToArray<S, D>(getProperties(), dst); }

Def(int32, int32);
Def(int64, int64);
Def(float32, float32);
Def(float64, float32);
Def(double2, float2);
Def(double3, float3);
Def(double4, float4);
#undef Def


template<class S, class D>
void ToVector(span<Property> props, D& dst)
{
    if (props.size() == get_vector_size<D>) {
        auto* d = dst.data();
        for (auto& prop : props)
            *d++ = prop.getValue<get_scalar_t<S>>();
    }
}
template<> void Node::getPropertiesValues<double4x4, float4x4>(float4x4& dst) const { ToVector<double4x4, float4x4>(getProperties(), dst); }

#endif


Node* Node::getParent() const
{
    return m_parent;
}

span<Node*> Node::getChildren() const
{
    return make_span(m_children);
}

Node* Node::getChild(size_t i) const
{
    return i < m_children.size() ? m_children[i] : nullptr;
}

Node* Node::findChild(string_view name) const
{
    auto it = std::find_if(m_children.begin(), m_children.end(),
        [name](Node* p) { return p->getName() == name; });
    return it != m_children.end() ? *it : nullptr;
}

std::string Node::toString(int depth) const
{
    std::string s;
    AddTabs(s, depth);
    s += getName();
    s += ": ";
    join(s, m_properties, ", ",
        [depth](auto& p) { return p.toString(depth); });
    s += " ";

    if (!m_children.empty() || (m_children.empty() && m_properties.empty())) {
        s += "{\n";
        for (auto* c : m_children)
            s += c->toString(depth + 1);
        AddTabs(s, depth);
        s += "}";
    }
    s += "\n";

    return s;
}

uint32_t Node::getDocumentVersion() const
{
    return (uint32_t)m_document->getVersion();
}

uint32_t Node::getHeaderSize() const
{
    if (getDocumentVersion() >= sfbxI_FBX2016_FileVersion) {
        // sizeof(uint64_t) * 3 + 1
        return 25;
    }
    else {
        // sizeof(uint32_t) * 3 + 1
        return 13;
    }
}

} // namespace sfbx
