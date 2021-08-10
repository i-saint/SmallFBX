#pragma once
#include "sfbxObject.h"

namespace sfbx {

template<class T>
inline constexpr bool is_deformer = std::is_base_of_v<Deformer, T>;


// Geometry and its subclasses:
//  (Mesh, Shape)

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

//for reference only, fbx stores as strings
enum class LayerMappingMode : int
{
    None,
    ByControlPoint,
    ByPolygonVertex,
    ByPolygon,
    ByEdge,
    AllSame,
};
enum class LayerReferenceMode : int
{
    Direct,
    Index,
    IndexToDirect,
};

// FBX can store multiple normal / UV / vertex color channels ("layer" in FBX term).
// LayerElement store these data.
template<class T>
struct LayerElement
{
    std::string name;
    RawVector<int> indices; // can be empty. in that case, size of data must equal with vertex count or index count.
    RawVector<T> data;
    RawVector<T> data_deformed; // relevant only for normal layers for now.
    string_view mapping_mode = "";
    string_view reference_mode = "";
};
using LayerElementF2 = LayerElement<float2>;
using LayerElementF3 = LayerElement<float3>;
using LayerElementF4 = LayerElement<float4>;
using LayerElementI1 = LayerElement<int>;

struct LayerElementDesc
{
    std::string type;
    int index = 0;
};

// GeomMesh represents polygon mesh data. parent of GeomMesh seems to be always Mesh.
class GeomMesh : public Geometry
{
using super = Geometry;
public:
    ObjectSubClass getSubClass() const override;

    span<int> getCounts() const;
    span<int> getIndices() const;
    span<float3> getPoints() const;
    span<LayerElementF3> getNormalLayers() const; // can be zero or multiple layers
    span<LayerElementF2> getUVLayers() const;     // can be zero or multiple layers
    span<LayerElementF4> getColorLayers() const;  // can be zero or multiple layers
    span<LayerElementI1> getMatrialLayers() const;// can be zero or multiple layers
    span<std::vector<LayerElementDesc>> getLayers() const;     //

    void setCounts(span<int> v);
    void setIndices(span<int> v);
    void setPoints(span<float3> v);
    void addNormalLayer(LayerElementF3&& v);
    void addUVLayer(LayerElementF2&& v);
    void addColorLayer(LayerElementF4&& v);
    void addMaterialLayer(LayerElementI1&& v);

    template<typename T>
    void checkModes(LayerElement<T>& layer); //check & update to default/known-correct modes to data/indices

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
    std::vector<LayerElementI1> m_material_layers;
    std::vector<std::vector<LayerElementDesc>> m_layers;
};


// a Shape is a target of blend shape. see BlendShape and BlendShapeChannel in sfbxDeformer.h.
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
