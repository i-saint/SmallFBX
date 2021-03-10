#pragma once
#include "sfbxObject.h"

namespace sfbx {

// Geometry and its subclasses:
//  (Mesh, Shape)

template<class T>
inline constexpr bool is_deformer = std::is_base_of_v<Deformer, T>;

class Geometry : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;
    void addChild(Object* v) override;
    void eraseChild(Object* v) override;

    Model* getModel() const;
    bool hasDeformer() const;
    bool hasSkinDeformer() const;
    span<Deformer*> getDeformers() const;

    // T: Skin, BlendShape
    template<class T, sfbxRestrict(is_deformer<T>)>
    T* createDeformer();

protected:
    std::vector<Deformer*> m_deformers;
};


template<class T>
struct LayerElement
{
    std::string name;
    RawVector<int> indices; // can be empty. in that case, size of data must equal with vertex count or index count.
    RawVector<T> data;
    RawVector<T> data_deformed;
};
using LayerElementF2 = LayerElement<float2>;
using LayerElementF3 = LayerElement<float3>;
using LayerElementF4 = LayerElement<float4>;

class GeomMesh : public Geometry
{
using super = Geometry;
public:
    ObjectSubClass getSubClass() const override;

    span<int> getCounts() const;
    span<int> getIndices() const;
    span<float3> getPoints() const;
    span<LayerElementF3> getNormalLayers() const;
    span<LayerElementF2> getUVLayers() const;
    span<LayerElementF4> getColorLayers() const;

    void setCounts(span<int> v);
    void setIndices(span<int> v);
    void setPoints(span<float3> v);
    void addNormalLayer(LayerElementF3&& v);
    void addUVLayer(LayerElementF2&& v);
    void addColorLayer(LayerElementF4&& v);

    span<float3> getPointsDeformed(bool apply_transform = false);
    span<float3> getNormalsDeformed(size_t layer_index = 0, bool apply_transform = false);

protected:
    void importFBXObjects() override;
    void exportFBXObjects() override;

    RawVector<int> m_counts;
    RawVector<int> m_indices;
    RawVector<float3> m_points;
    RawVector<float3> m_points_deformed;
    std::vector<LayerElementF3> m_normal_layers;
    std::vector<LayerElementF2> m_uv_layers;
    std::vector<LayerElementF4> m_color_layers;
};

class Shape : public Geometry
{
using super = Geometry;
public:
    ObjectSubClass getSubClass() const override;

    span<int> getIndices() const;
    span<float3> getDeltaPoints() const;
    span<float3> getDeltaNormals() const;

    void setIndices(span<int> v);
    void setDeltaPoints(span<float3> v);
    void setDeltaNormals(span<float3> v);

public:
    void importFBXObjects() override;
    void exportFBXObjects() override;

    RawVector<int> m_indices;
    RawVector<float3> m_delta_points;
    RawVector<float3> m_delta_normals;
};


} // namespace sfbx
