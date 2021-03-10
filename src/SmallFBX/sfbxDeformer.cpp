#include "pch.h"
#include "sfbxInternal.h"
#include "sfbxModel.h"
#include "sfbxGeometry.h"
#include "sfbxDeformer.h"
#include "sfbxDocument.h"

namespace sfbx {

ObjectClass Deformer::getClass() const { return ObjectClass::Deformer; }

GeomMesh* Deformer::getBaseMesh() const
{
    for (auto* p = getParent(); p; p = p->getParent())
        if (auto geom = as<GeomMesh>(p))
            return geom;
    return nullptr;
}

void Deformer::deformPoints(span<float3> dst) const {}
void Deformer::deformNormals(span<float3> dst) const {}


ObjectClass SubDeformer::getClass() const { return ObjectClass::Deformer; }


ObjectSubClass Skin::getSubClass() const { return ObjectSubClass::Skin; }

void Skin::importFBXObjects()
{
    super::importFBXObjects();
}

void Skin::exportFBXObjects()
{
    super::exportFBXObjects();

    auto n = getNode();
    n->createChild(sfbxS_Version, sfbxI_SkinVersion);
    n->createChild(sfbxS_Link_DeformAcuracy, (float64)50.0);

}

void Skin::addParent(Object* v)
{
    super::addParent(v);
    if (auto mesh = as<GeomMesh>(v))
        m_mesh = mesh;
}

void Skin::addChild(Object* v)
{
    super::addChild(v);
    if (auto cluster = as<Cluster>(v)) {
        m_clusters.push_back(cluster);

        // clear cached skin data
        m_weights = {};
        m_joint_matrices = {};
    }
}

void Skin::eraseChild(Object* v)
{
    super::eraseChild(v);
    if (auto cluster = as<Cluster>(v))
        erase(m_clusters, cluster);
}

GeomMesh* Skin::getMesh() const { return m_mesh; }
span<Cluster*> Skin::getClusters() const { return make_span(m_clusters); }

const JointWeights& Skin::getJointWeights() const
{
    auto& ret = m_weights;
    if (!ret.counts.empty())
        return ret;

    auto mesh = getBaseMesh();
    if (!mesh)
        return ret;

    size_t cclusters = m_clusters.size();
    size_t cpoints = mesh->getPoints().size();
    size_t total_weights = 0;

    // setup counts
    ret.counts.resize(cpoints, 0);
    for (auto cluster : m_clusters) {
        auto indices = cluster->getIndices();
        for (int vi : indices)
            ret.counts[vi]++;
        total_weights += indices.size();
    }

    // setup offsets
    ret.offsets.resize(cpoints);
    size_t offset = 0;
    int max_joints_per_vertex = 0;
    for (size_t pi = 0; pi < cpoints; ++pi) {
        ret.offsets[pi] = offset;
        int c = ret.counts[pi];
        offset += c;
        max_joints_per_vertex = std::max(max_joints_per_vertex, c);
    }
    ret.max_joints_per_vertex = max_joints_per_vertex;

    // setup weights
    ret.counts.zeroclear(); // clear to recount
    ret.weights.resize(total_weights);
    for (size_t ci = 0; ci < cclusters; ++ci) {
        auto cluster = m_clusters[ci];
        auto indices = cluster->getIndices();
        auto weights = cluster->getWeights();
        size_t cweights = weights.size();
        for (size_t wi = 0; wi < cweights; ++wi) {
            int vi = indices[wi];
            float weight = weights[wi];
            int pos = ret.offsets[vi] + ret.counts[vi]++;
            ret.weights[pos] = { (int)ci, weight };
        }
    }

    // sort weights
    {
        auto* w = ret.weights.data();
        for (int c : ret.counts) {
            std::sort(w, w + c, [](auto& a, auto& b) { return b.weight < a.weight; });
            w += c;
        }
    }
    return ret;
}

JointWeights Skin::createFixedJointWeights(int joints_per_vertex) const
{
    auto& tmp = getJointWeights();
    if (tmp.weights.empty())
        return tmp;

    JointWeights ret;
    size_t cpoints = tmp.counts.size();
    ret.max_joints_per_vertex = std::min(joints_per_vertex, tmp.max_joints_per_vertex);
    ret.counts.resize(cpoints);
    ret.offsets.resize(cpoints);
    ret.weights.resize(cpoints * joints_per_vertex);
    ret.weights.zeroclear();

    auto normalize_weights = [](span<JointWeight> weights) {
        float total = 0.0f;
        for (auto& w : weights)
            total += w.weight;

        if (total != 0.0f) {
            float rcp_total = 1.0f / total;
            for (auto& w : weights)
                w.weight *= rcp_total;
        }
    };

    for (size_t pi = 0; pi < cpoints; ++pi) {
        int count = tmp.counts[pi];
        ret.counts[pi] = std::min(count, joints_per_vertex);
        ret.offsets[pi] = joints_per_vertex * pi;

        auto* src = &tmp.weights[tmp.offsets[pi]];
        auto* dst = &ret.weights[ret.offsets[pi]];
        if (count < joints_per_vertex) {
            copy(dst, src, size_t(count));
        }
        else {
            copy(dst, src, size_t(joints_per_vertex));
            normalize_weights(make_span(dst, joints_per_vertex));
        }
    }
    return ret;
}

const JointMatrices& Skin::getJointMatrices() const
{
    auto& ret = m_joint_matrices;

    // todo: cache result
    size_t cclusters = m_clusters.size();
    ret.bindpose.resize(cclusters);
    ret.global_transform.resize(cclusters);
    ret.joint_transform.resize(cclusters);
    for (size_t ci = 0; ci < cclusters; ++ci) {
        auto bindpose = m_clusters[ci]->getTransform();
        ret.bindpose[ci] = bindpose;
        if (auto trans = as<Model>(m_clusters[ci]->getChild())) {
            auto global_matrix = trans->getGlobalMatrix();
            ret.global_transform[ci] = global_matrix;
            ret.joint_transform[ci] = bindpose * global_matrix;
        }
        else {
            // should not be here
            sfbxPrint("sfbx::Deformer::skinMakeJointMatrices(): Cluster has non-Model child\n");
            ret.global_transform[ci] = ret.joint_transform[ci] = float4x4::identity();
        }
    }
    return ret;
}


Cluster* Skin::createCluster(Model* joint)
{
    if (!joint)
        return nullptr;
    auto r = createChild<Cluster>();
    r->setName(joint->getName());
    r->addChild(joint);
    return r;
}

void Skin::deformPoints(span<float3> dst) const
{
    auto& weights = getJointWeights();
    auto& matrices = getJointMatrices();
    DeformPoints(dst, weights, matrices, dst);
}

void Skin::deformNormals(span<float3> dst) const
{
    auto& weights = getJointWeights();
    auto& matrices = getJointMatrices();
    DeformVectors(dst, weights, matrices, dst);
}



ObjectSubClass Cluster::getSubClass() const { return ObjectSubClass::Cluster; }

void Cluster::importFBXObjects()
{
    super::importFBXObjects();

    auto n = getNode();
    GetChildPropertyValue<int>(m_indices, n, sfbxS_Indexes);
    GetChildPropertyValue<float64>(m_weights, n, sfbxS_Weights);
    GetChildPropertyValue<double4x4>(m_transform, n, sfbxS_Transform);
    GetChildPropertyValue<double4x4>(m_transform_link, n, sfbxS_TransformLink);
}

void Cluster::exportFBXObjects()
{
    super::exportFBXObjects();

    auto n = getNode();
    n->createChild(sfbxS_Version, sfbxI_ClusterVersion);
    n->createChild(sfbxS_Mode, sfbxS_Total1);
    n->createChild(sfbxS_UserData, "", "");
    if (!m_indices.empty())
        n->createChild(sfbxS_Indexes, m_indices);
    if (!m_weights.empty())
        n->createChild(sfbxS_Weights, make_adaptor<float64>(m_weights));
    if (m_transform != float4x4::identity())
        n->createChild(sfbxS_Transform, (double4x4)m_transform);
    if (m_transform_link != float4x4::identity())
        n->createChild(sfbxS_TransformLink, (double4x4)m_transform_link);
}

span<int> Cluster::getIndices() const { return make_span(m_indices); }
span<float> Cluster::getWeights() const { return make_span(m_weights); }
float4x4 Cluster::getTransform() const { return m_transform; }
float4x4 Cluster::getTransformLink() const { return m_transform_link; }

void Cluster::setIndices(span<int> v) { m_indices = v; }
void Cluster::setWeights(span<float> v) { m_weights = v; }
void Cluster::setBindMatrix(float4x4 v)
{
    m_transform_link = v;
    m_transform = invert(v);
}


ObjectSubClass BlendShape::getSubClass() const { return ObjectSubClass::BlendShape; }

void BlendShape::importFBXObjects()
{
    super::importFBXObjects();
}

void BlendShape::exportFBXObjects()
{
    super::exportFBXObjects();

    auto n = getNode();
    n->createChild(sfbxS_Version, sfbxI_BlendShapeVersion);
}

void BlendShape::addChild(Object* v)
{
    super::addChild(v);
    if (auto ch = as<BlendShapeChannel>(v))
        m_channels.push_back(ch);
}

void BlendShape::eraseChild(Object* v)
{
    super::eraseChild(v);
    if (auto ch = as<BlendShapeChannel>(v))
        erase(m_channels, ch);
}

span<BlendShapeChannel*> BlendShape::getChannels() const
{
    return make_span(m_channels);
}

BlendShapeChannel* BlendShape::createChannel(string_view name)
{
    return createChild<BlendShapeChannel>(name);
}

BlendShapeChannel* BlendShape::createChannel(Shape* shape)
{
    if (!shape)
        return nullptr;
    auto ret = createChannel(shape->getName());
    ret->addShape(shape);
    return ret;
}

void BlendShape::deformPoints(span<float3> dst) const
{
    for (auto ch : m_channels)
        ch->deformPoints(dst);
}

void BlendShape::deformNormals(span<float3> dst) const
{
    for (auto ch : m_channels)
        ch->deformNormals(dst);
}


ObjectSubClass BlendShapeChannel::getSubClass() const { return ObjectSubClass::BlendShapeChannel; }

void BlendShapeChannel::importFBXObjects()
{
    super::importFBXObjects();

    for (auto c : getChildren()) {
        if (auto shape = as<Shape>(c))
            m_shape_data.push_back({ shape, 1.0f });
    }
    if (auto n = getNode()->findChild(sfbxS_FullWeights)) {
        RawVector<float> weights;
        GetPropertyValue<float64>(weights, n);
        if (weights.size() == m_shape_data.size()) {
            size_t n = weights.size();
            for (size_t i = 0; i < n; ++i)
                m_shape_data[i].weight = weights[i] * 0.01f; // percent to 0-1
        }
    }
}

void BlendShapeChannel::exportFBXObjects()
{
    super::exportFBXObjects();

    auto n = getNode();
    n->createChild(sfbxS_Version, sfbxI_BlendShapeChannelVersion);
    n->createChild(sfbxS_DeformPercent, (float64)0);
    if (!m_shape_data.empty()) {
        auto full_weights = n->createChild(sfbxS_FullWeights);
        auto* dst = full_weights->createProperty()->allocateArray<float64>(m_shape_data.size()).data();
        for (auto& data : m_shape_data)
            *dst++ = float64(data.weight) * 100.0; // 0-1 to percent
    }
}

float BlendShapeChannel::getWeight() const
{
    return m_weight;
}

span<BlendShapeChannel::ShapeData> BlendShapeChannel::getShapeData() const
{
    return make_span(m_shape_data);
}

void BlendShapeChannel::addShape(Shape* shape, float weight)
{
    if (shape) {
        addChild(shape);
        m_shape_data.push_back({ shape, weight });
    }
}

void BlendShapeChannel::setWeight(float v)
{
    m_weight = v;
}

void BlendShapeChannel::deformPoints(span<float3> dst) const
{
    if (m_weight <= 0.0f || m_shape_data.empty())
        return;

    if (m_shape_data.size() == 1) {
        auto& sd = m_shape_data[0];
        auto shape = sd.shape;
        auto indices = shape->getIndices();
        auto delta = shape->getDeltaPoints();
        size_t n = indices.size();
        float w = m_weight / sd.weight;
        for (size_t i = 0; i < n; ++i)
            dst[indices[i]] += delta[i] * w;
    }
    else {
        // todo
    }
}

void BlendShapeChannel::deformNormals(span<float3> dst) const
{
    if (m_weight == 0.0f)
        return;
    if (m_shape_data.size() == 1) {
        auto& sd = m_shape_data[0];
        auto shape = sd.shape;
        auto indices = shape->getIndices();
        auto delta = shape->getDeltaNormals();
        size_t n = indices.size();
        float w = m_weight / sd.weight;
        for (size_t i = 0; i < n; ++i)
            dst[indices[i]] += delta[i] * w;
    }
    else if (m_shape_data.size() > 1) {
        // todo
    }
}


ObjectClass Pose::getClass() const { return ObjectClass::Pose; }

ObjectSubClass BindPose::getSubClass() const { return ObjectSubClass::BindPose; }

void BindPose::importFBXObjects()
{
    super::importFBXObjects();

    for (auto n : getNode()->getChildren()) {
        if (n->getName() == sfbxS_PoseNode) {
            auto nid = GetChildPropertyValue<int64>(n, sfbxS_Node);
            auto model = as<Model>(m_document->findObject(nid));
            if (model) {
                float4x4 mat;
                GetChildPropertyValue<double4x4>(mat, n, sfbxS_Matrix);
                m_pose_data.push_back({ model, float4x4(mat) });
            }
            else {
                sfbxPrint("sfbx::Pose::constructObject(): non-Model joint object\n");
            }
        }
    }
}

void BindPose::exportFBXObjects()
{
    super::exportFBXObjects();

    auto n = getNode();
    n->createChild(sfbxS_Type, sfbxS_BindPose);
    n->createChild(sfbxS_Version, sfbxI_BindPoseVersion);
    n->createChild(sfbxS_NbPoseNodes, (int32)m_pose_data.size());
    for (auto& d : m_pose_data) {
        auto pn = n->createChild(sfbxS_PoseNode);
        pn->createChild(sfbxS_Node, (int64)d.object);
        pn->createChild(sfbxS_Matrix, (double4x4)d.matrix);
    }
}

span<BindPose::PoseData> BindPose::getPoseData() const { return make_span(m_pose_data); }
void BindPose::addPoseData(Model* joint, float4x4 bind_matrix) { m_pose_data.push_back({ joint, bind_matrix }); }

} // namespace sfbx
