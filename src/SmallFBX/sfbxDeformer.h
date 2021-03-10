#pragma once
#include "sfbxObject.h"

namespace sfbx {

// Deformer and its subclasses:
//  (Skin, Cluster, BlendShape, BlendShapeChannel)

class Deformer : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;

    GeomMesh* getBaseMesh() const;

    // apply deform to dst. size of dst must be equal with base mesh.
    virtual void deformPoints(span<float3> dst) const;
    virtual void deformNormals(span<float3> dst) const;
};

class SubDeformer : public Object
{
using super = Object;
public:
protected:
    ObjectClass getClass() const override;
};


struct JointWeight
{
    int index; // index of joint/cluster
    float weight;
};

struct JointWeights // copyable
{
    int max_joints_per_vertex = 0;
    RawVector<int> counts; // per-vertex. counts of affected joints.
    RawVector<int> offsets; // per-vertex. offset to weights.
    RawVector<JointWeight> weights; // vertex * affected joints (total of counts). weights of affected joints.
};

struct JointMatrices
{
    RawVector<float4x4> bindpose;
    RawVector<float4x4> global_transform;
    RawVector<float4x4> joint_transform;
};

class Skin : public Deformer
{
using super = Deformer;
public:
    ObjectSubClass getSubClass() const override;
    void addChild(Object* v) override;
    void eraseChild(Object* v) override;

    GeomMesh* getMesh() const;
    span<Cluster*> getClusters() const;
    const JointWeights& getJointWeights() const;
    JointWeights createFixedJointWeights(int joints_per_vertex) const;
    const JointMatrices& getJointMatrices() const;

    // joint should be Null, Root or LimbNode
    Cluster* createCluster(Model* joint);

    // apply deform to dst. size of dst must be equal with base mesh.
    void deformPoints(span<float3> dst) const override;
    void deformNormals(span<float3> dst) const override;

protected:
    void importFBXObjects() override;
    void exportFBXObjects() override;
    void addParent(Object* v) override;

    GeomMesh* m_mesh{};
    std::vector<Cluster*> m_clusters;
    mutable JointWeights m_weights;
    mutable JointMatrices m_joint_matrices;
};


class Cluster : public SubDeformer
{
using super = SubDeformer;
public:
    ObjectSubClass getSubClass() const override;

    span<int> getIndices() const;
    span<float> getWeights() const;
    float4x4 getTransform() const;
    float4x4 getTransformLink() const;

    void setIndices(span<int> v);
    void setWeights(span<float> v);
    void setBindMatrix(float4x4 v); // v: global matrix of the joint (not inverted)

protected:
    void importFBXObjects() override;
    void exportFBXObjects() override;

    RawVector<int> m_indices;
    RawVector<float> m_weights;
    float4x4 m_transform = float4x4::identity();
    float4x4 m_transform_link = float4x4::identity();
};


class BlendShape : public Deformer
{
using super = Deformer;
public:
    ObjectSubClass getSubClass() const override;
    void addChild(Object* v) override;
    void eraseChild(Object* v) override;

    span<BlendShapeChannel*> getChannels() const;
    BlendShapeChannel* createChannel(string_view name);
    BlendShapeChannel* createChannel(Shape* shape);

    void deformPoints(span<float3> dst) const override;
    void deformNormals(span<float3> dst) const override;

protected:
    void importFBXObjects() override;
    void exportFBXObjects() override;

    std::vector<BlendShapeChannel*> m_channels;
};


class BlendShapeChannel : public SubDeformer
{
using super = SubDeformer;
public:
    struct ShapeData
    {
        Shape* shape;
        float weight;
    };

    ObjectSubClass getSubClass() const override;

    float getWeight() const;
    span<ShapeData> getShapeData() const;
    // weight: 0.0f - 100.0f
    void addShape(Shape* shape, float weight = 100.0f);

    void setWeight(float v);
    void deformPoints(span<float3> dst) const;
    void deformNormals(span<float3> dst) const;

protected:
    void importFBXObjects() override;
    void exportFBXObjects() override;

    std::vector<ShapeData> m_shape_data;
    float m_weight = 0.0f;
};



// Pose and its subclasses:
//  (BindPose only for now. probably RestPose will be added)

class Pose : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;
};

class BindPose : public Pose
{
using super = Pose;
public:
    struct PoseData
    {
        Model* object;
        float4x4 matrix;
    };

    ObjectSubClass getSubClass() const override;

    span<PoseData> getPoseData() const;
    void addPoseData(Model* joint, float4x4 bind_matrix);

protected:
    void importFBXObjects() override;
    void exportFBXObjects() override;

    std::vector<PoseData> m_pose_data;
};


} // namespace sfbx
