#pragma once
#include "sfbxProperty.h"

namespace sfbx {

class Node
{
friend class Document;
public:
    Node();
    Node(Node&& v) noexcept;
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    bool readAscii(string_view& is);
    uint64_t readBinary(std::istream& is, uint64_t start_offset);
    uint64_t write(std::ostream& os, uint64_t start_offset);
    bool isNull() const;
    bool isRoot() const;

    void setName(string_view v);
    void setForceNullTerminate(bool v);

    void reserveProperties(size_t v);
    Property* createProperty();
    Node* createChild(string_view name = {});
    void eraseChild(Node* n);

    // utils
    template<class T> void addProperty(const T& v) { createProperty()->assign(v); }
    template<class... T> void addProperties(T&&... v) { reserveProperties(getProperties().size() + sizeof...(T));  addProperties_(v...); }
    template<class... T> Node* createChild(string_view name, T&&... v) { auto r = createChild(name);  r->addProperties(v...); return r; }


    string_view getName() const;
    span<Property> getProperties() const;
    Property* getProperty(size_t i);
    // for legacy format. there are no array types and arrays are represented as a huge list of properties.
    template<class Src, class Dst> void getPropertiesValues(RawVector<Dst>& dst) const;
    template<class Src, class Dst, sfbxRestrict(is_vector<Dst>)> void getPropertiesValues(Dst& dst) const;

    Node* getParent() const;
    span<Node*> getChildren() const;
    Node* getChild(size_t i) const;
    Node* findChild(string_view name) const;

    std::string toString(int depth = 0) const;

private:
    void addProperties_() {}
    template<class T, class... U> void addProperties_(T&& v, U&&... a) { addProperty(v); addProperties_(a...); }

    uint32_t getDocumentVersion() const;
    uint32_t getHeaderSize() const;
    bool isNullTerminated() const;

    Document* m_document{};
    std::string m_name;
    std::vector<Property> m_properties;

    Node* m_parent{};
    std::vector<Node*> m_children;
    bool m_force_null_terminate = false;
};

} // namespace sfbx
