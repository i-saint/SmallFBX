#include "pch.h"
#include "sfbxInternal.h"
#include "sfbxObject.h"
#include "sfbxModel.h"
#include "sfbxGeometry.h"
#include "sfbxDeformer.h"

namespace sfbx {

ObjectClass Geometry::getClass() const { return ObjectClass::Geometry; }

void Geometry::addChild(Object* v)
{
    super::addChild(v);
    if (auto deformer = as<Deformer>(v))
        m_deformers.push_back(deformer);
}

void Geometry::eraseChild(Object* v)
{
    super::eraseChild(v);
    if (auto deformer = as<Deformer>(v))
        erase(m_deformers, deformer);
}

Model* Geometry::getModel() const
{
    for (auto p : m_parents)
        if (auto model = as<Model>(p))
            return model;
    return nullptr;
}

bool Geometry::hasDeformer() const
{
    return !m_deformers.empty();
}

bool Geometry::hasSkinDeformer() const
{
    for (auto d : m_deformers)
        if (auto skin = as<Skin>(d))
            return true;
    return false;
}

span<Deformer*> Geometry::getDeformers() const
{
    return make_span(m_deformers);
}

template<> Skin* Geometry::createDeformer()
{
    auto ret = createChild<Skin>();
    //ret->setName(getName()); // FBX SDK seems don't do this
    return ret;
}

template<> BlendShape* Geometry::createDeformer()
{
    auto ret = createChild<BlendShape>();
    ret->setName(getName());
    return ret;
}


ObjectSubClass GeomMesh::getSubClass() const { return ObjectSubClass::Mesh; }

void GeomMesh::importFBXObjects()
{
    super::importFBXObjects();

    for (auto n : getNode()->getChildren()) {
        if (n->getName() == sfbxS_Vertices) {
            // points
            GetPropertyValue<double3>(m_points, n);
        }
        else if (n->getName() == sfbxS_PolygonVertexIndex) {
            // counts & indices
            GetPropertyValue<int>(m_indices, n);
            m_counts.resize(m_indices.size()); // reserve
            int* dst_counts = m_counts.data();
            size_t cfaces = 0;
            size_t cpoints = 0;
            for (int& i : m_indices) {
                ++cpoints;
                if (i < 0) { // negative value indicates the last index in the face
                    i = ~i;
                    dst_counts[cfaces++] = cpoints;
                    cpoints = 0;
                }
            }
            m_counts.resize(cfaces); // fit to actual size
        }
        else if (n->getName() == sfbxS_LayerElementNormal) {
            // normals
            //auto mapping = n->findChildProperty(sfbxS_MappingInformationType);
            //auto ref = n->findChildProperty(sfbxS_ReferenceInformationType);
            LayerElementF3 tmp;
            tmp.name = GetChildPropertyString(n, sfbxS_Name);
            GetChildPropertyValue<double3>(tmp.data, n, sfbxS_Normals);
            GetChildPropertyValue<int>(tmp.indices, n, sfbxS_NormalsIndex);
            m_normal_layers.push_back(std::move(tmp));
        }
        else if (n->getName() == sfbxS_LayerElementUV) {
            // uv
            LayerElementF2 tmp;
            tmp.name = GetChildPropertyString(n, sfbxS_Name);
            GetChildPropertyValue<double2>(tmp.data, n, sfbxS_UV);
            GetChildPropertyValue<int>(tmp.indices, n, sfbxS_UVIndex);
            m_uv_layers.push_back(std::move(tmp));
        }
        else if (n->getName() == sfbxS_LayerElementColor) {
            // colors
            LayerElementF4 tmp;
            tmp.name = GetChildPropertyString(n, sfbxS_Name);
            GetChildPropertyValue<double4>(tmp.data, n, sfbxS_Colors);
            GetChildPropertyValue<int>(tmp.indices, n, sfbxS_ColorIndex);
            m_color_layers.push_back(std::move(tmp));
        }
    }
}

void GeomMesh::exportFBXObjects()
{
    super::exportFBXObjects();

    Node* n = getNode();

    n->createChild(sfbxS_GeometryVersion, sfbxI_GeometryVersion);

    // points
    n->createChild(sfbxS_Vertices, make_adaptor<double3>(m_points));

    // indices
    {
        // check if counts and indices are valid
        size_t total_counts = 0;
        for (int c : m_counts)
            total_counts += c;

        if (total_counts != m_indices.size()) {
            sfbxPrint("sfbx::Mesh: *** indices mismatch with counts ***\n");
        }
        else {
            auto* src_counts = m_counts.data();
            auto dst_node = n->createChild(sfbxS_PolygonVertexIndex);
            auto dst_prop = dst_node->createProperty();
            auto dst = dst_prop->allocateArray<int>(m_indices.size()).data();

            size_t cpoints = 0;
            for (int i : m_indices) {
                if (++cpoints == *src_counts) {
                    i = ~i; // negative value indicates the last index in the face
                    cpoints = 0;
                    ++src_counts;
                }
                *dst++ = i;
            }
        }
    }

    auto add_mapping_and_reference_info = [this](Node* node, const auto& layer) {
        if (layer.data.size() == m_indices.size() || layer.indices.size() == m_indices.size())
            node->createChild(sfbxS_MappingInformationType, "ByPolygonVertex");
        else if (layer.data.size() == m_points.size() && layer.indices.empty())
            node->createChild(sfbxS_MappingInformationType, "ByControllPoint");

        if (!layer.indices.empty())
            node->createChild(sfbxS_ReferenceInformationType, "IndexToDirect");
        else
            node->createChild(sfbxS_ReferenceInformationType, "Direct");
    };

    int clayers = 0;

    // normal layers
    for (auto& layer : m_normal_layers) {
        if (layer.data.empty())
            continue;

        ++clayers;
        auto l = n->createChild(sfbxS_LayerElementNormal);
        l->createChild(sfbxS_Version, sfbxI_LayerElementNormalVersion);
        l->createChild(sfbxS_Name, layer.name);

        add_mapping_and_reference_info(l, layer);
        l->createChild(sfbxS_Normals, make_adaptor<double3>(layer.data));
        if (!layer.indices.empty())
            l->createChild(sfbxS_NormalsIndex, layer.indices);
    }

    // uv layers
    for (auto& layer : m_uv_layers) {
        if (layer.data.empty())
            continue;

        ++clayers;
        auto l = n->createChild(sfbxS_LayerElementUV);
        l->createChild(sfbxS_Version, sfbxI_LayerElementUVVersion);
        l->createChild(sfbxS_Name, layer.name);

        add_mapping_and_reference_info(l, layer);
        l->createChild(sfbxS_UV, make_adaptor<double2>(layer.data));
        if (!layer.indices.empty())
            l->createChild(sfbxS_UVIndex, layer.indices);
    }

    // color layers
    for (auto& layer : m_color_layers) {
        if (layer.data.empty())
            continue;

        ++clayers;
        auto l = n->createChild(sfbxS_LayerElementColor);
        l->createChild(sfbxS_Version, sfbxI_LayerElementColorVersion);
        l->createChild(sfbxS_Name, layer.name);

        add_mapping_and_reference_info(l, layer);
        l->createChild(sfbxS_Colors, make_adaptor<double4>(layer.data));
        if (!layer.indices.empty())
            l->createChild(sfbxS_ColorIndex, layer.indices);
    }

    if (clayers) {
        // layer info
        auto l = n->createChild(sfbxS_Layer, 0);
        l->createChild(sfbxS_Version, sfbxI_LayerVersion);
        if (!m_normal_layers.empty()) {
            auto le = l->createChild(sfbxS_LayerElement);
            le->createChild(sfbxS_Type, sfbxS_LayerElementNormal);
            le->createChild(sfbxS_TypeIndex, 0);
        }
        if (!m_uv_layers.empty()) {
            auto le = l->createChild(sfbxS_LayerElement);
            le->createChild(sfbxS_Type, sfbxS_LayerElementUV);
            le->createChild(sfbxS_TypeIndex, 0);
        }
        if (!m_color_layers.empty()) {
            auto le = l->createChild(sfbxS_LayerElement);
            le->createChild(sfbxS_Type, sfbxS_LayerElementColor);
            le->createChild(sfbxS_TypeIndex, 0);
        }
    }
}

span<int> GeomMesh::getCounts() const { return make_span(m_counts); }
span<int> GeomMesh::getIndices() const { return make_span(m_indices); }
span<float3> GeomMesh::getPoints() const { return make_span(m_points); }
span<LayerElementF3> GeomMesh::getNormalLayers() const { return make_span(m_normal_layers); }
span<LayerElementF2> GeomMesh::getUVLayers() const { return make_span(m_uv_layers); }
span<LayerElementF4> GeomMesh::getColorLayers() const { return make_span(m_color_layers); }

void GeomMesh::setCounts(span<int> v) { m_counts = v; }
void GeomMesh::setIndices(span<int> v) { m_indices = v; }
void GeomMesh::setPoints(span<float3> v) { m_points = v; }
void GeomMesh::addNormalLayer(LayerElementF3&& v) { m_normal_layers.push_back(std::move(v)); }
void GeomMesh::addUVLayer(LayerElementF2&& v) { m_uv_layers.push_back(std::move(v)); }
void GeomMesh::addColorLayer(LayerElementF4&& v) { m_color_layers.push_back(std::move(v)); }

span<float3> GeomMesh::getPointsDeformed(bool apply_transform)
{
    if (m_deformers.empty() && !apply_transform)
        return make_span(m_points);

    m_points_deformed = m_points;
    auto dst = make_span(m_points_deformed);
    bool is_skinned = false;
    for (auto deformer : m_deformers) {
        deformer->deformPoints(dst);
        if (as<Skin>(deformer))
            is_skinned = true;
    }
    if (!is_skinned && apply_transform) {
        if (auto model = getModel()) {
            auto mat = model->getGlobalMatrix();
            for (auto& v : dst)
                v = mul_p(mat, v);
        }
    }
    return dst;
}

span<float3> GeomMesh::getNormalsDeformed(size_t layer_index, bool apply_transform)
{
    if (layer_index >= m_normal_layers.size())
        return {};

    auto& l = m_normal_layers[layer_index];
    if (m_deformers.empty() && !apply_transform)
        return make_span(l.data);

    l.data_deformed = l.data;
    auto dst = make_span(l.data_deformed);
    bool is_skinned = false;
    for (auto deformer : m_deformers) {
        deformer->deformNormals(dst);
        if (as<Skin>(deformer))
            is_skinned = true;
    }
    if (!is_skinned && apply_transform) {
        if (auto model = getModel()) {
            auto mat = model->getGlobalMatrix();
            for (auto& v : dst)
                v = normalize(mul_v(mat, v));
        }
    }
    return dst;
}


ObjectSubClass Shape::getSubClass() const { return ObjectSubClass::Shape; }

void Shape::importFBXObjects()
{
    super::importFBXObjects();

    for (auto n : getNode()->getChildren()) {
        auto name = n->getName();
        if (name == sfbxS_Indexes)
            GetPropertyValue<int>(m_indices, n);
        else if (name == sfbxS_Vertices)
            GetPropertyValue<double3>(m_delta_points, n);
        else if (name == sfbxS_Normals)
            GetPropertyValue<double3>(m_delta_normals, n);
    }
}

void Shape::exportFBXObjects()
{
    super::exportFBXObjects();

    Node* n = getNode();
    n->createChild(sfbxS_Version, sfbxI_ShapeVersion);
    if (!m_indices.empty())
        n->createChild(sfbxS_Indexes, m_indices);
    if (!m_delta_points.empty())
        n->createChild(sfbxS_Vertices, make_adaptor<double3>(m_delta_points));
    if (!m_delta_normals.empty())
        n->createChild(sfbxS_Normals, make_adaptor<double3>(m_delta_normals));
}

span<int> Shape::getIndices() const { return make_span(m_indices); }
span<float3> Shape::getDeltaPoints() const { return make_span(m_delta_points); }
span<float3> Shape::getDeltaNormals() const { return make_span(m_delta_normals); }

void Shape::setIndices(span<int> v) { m_indices = v; }
void Shape::setDeltaPoints(span<float3> v) { m_delta_points = v; }
void Shape::setDeltaNormals(span<float3> v) { m_delta_normals = v; }

} // namespace sfbx
