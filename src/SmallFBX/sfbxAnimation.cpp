#include "pch.h"
#include "sfbxInternal.h"
#include "sfbxAnimation.h"
#include "sfbxModel.h"
#include "sfbxDeformer.h"
#include "sfbxMaterial.h"
#include "sfbxDocument.h"

namespace sfbx {


ObjectClass AnimationStack::getClass() const { return ObjectClass::AnimationStack; }

void AnimationStack::importFBXObjects()
{
    super::importFBXObjects();

    EnumerateProperties(getNode(), [this](Node* n) {
        if (GetPropertyString(n, 0) == sfbxS_LocalStart)
            m_local_start = FromTicks(GetPropertyValue<int64>(n, 4));
        else if (GetPropertyString(n, 0) == sfbxS_LocalStop)
            m_local_stop = FromTicks(GetPropertyValue<int64>(n, 4));
        else if (GetPropertyString(n, 0) == sfbxS_ReferenceStart)
            m_reference_start = FromTicks(GetPropertyValue<int64>(n, 4));
        else if (GetPropertyString(n, 0) == sfbxS_ReferenceStop)
            m_reference_stop = FromTicks(GetPropertyValue<int64>(n, 4));
        });
}

void AnimationStack::exportFBXObjects()
{
    super::exportFBXObjects();

    float start{}, stop{};
    bool first = true;
    for (auto* layer : getAnimationLayers()) {
        for (auto* node : layer->getAnimationCurveNodes()) {
            if (first) {
                start = node->getStartTime();
                stop = node->getStopTime();
            }
            else {
                start = std::min(start, node->getStartTime());
                stop = std::min(start, node->getStopTime());
            }
        }
    }

    m_reference_start = m_local_start = start;
    m_reference_stop = m_local_stop = stop;
    auto props = getNode()->createChild(sfbxS_Properties70);
    if (m_local_start != 0.0f)
        props->createChild(sfbxS_P, sfbxS_LocalStart, sfbxS_KTime, sfbxS_Time, "", ToTicks(m_local_start));
    if (m_local_stop != 0.0f)
        props->createChild(sfbxS_P, sfbxS_LocalStop, sfbxS_KTime, sfbxS_Time, "", ToTicks(m_local_stop));
    if (m_reference_start != 0.0f)
        props->createChild(sfbxS_P, sfbxS_ReferenceStart, sfbxS_KTime, sfbxS_Time, "", ToTicks(m_reference_start));
    if (m_reference_stop != 0.0f)
        props->createChild(sfbxS_P, sfbxS_ReferenceStop, sfbxS_KTime, sfbxS_Time, "", ToTicks(m_reference_stop));
}

void AnimationStack::addChild(Object* v)
{
    super::addChild(v);
    if (auto l = as<AnimationLayer>(v))
        m_anim_layers.push_back(l);
}

void AnimationStack::eraseChild(Object* v)
{
    super::eraseChild(v);
    if (auto l = as<AnimationLayer>(v))
        erase(m_anim_layers, l);
}

float AnimationStack::getLocalStart() const { return m_local_start; }
float AnimationStack::getLocalStop() const { return m_local_stop; }
float AnimationStack::getReferenceStart() const { return m_reference_start; }
float AnimationStack::getReferenceStop() const { return m_reference_stop; }
span<AnimationLayer*> AnimationStack::getAnimationLayers() const { return m_anim_layers; }

AnimationLayer* AnimationStack::createLayer(string_view name)
{
    return createChild<AnimationLayer>(name);
}

void AnimationStack::applyAnimation(float time)
{
    for (auto layer : m_anim_layers)
        layer->applyAnimation(time);
}

bool AnimationStack::remap(Document* doc)
{
    // make_reverse because elements maybe erased in the loop
    for (auto layer : make_reverse(getAnimationLayers())) {
        if (!layer->remap(doc))
            eraseChild(layer);
    }
    if (m_anim_layers.empty())
        return false;

    AnimationStack* dst = this;
    if (auto s = doc->findAnimationStack(getFullName())) {
        s->merge(this);
        dst = s;
    }

    // todo: 
    doc->addObject(dst->shared_from_this(), true);
    for (auto layer : dst->getAnimationLayers()) {
        doc->addObject(layer->shared_from_this(), true);
        for (auto node : layer->getAnimationCurveNodes()) {
            doc->addObject(node->shared_from_this(), true);
            for (auto curve : node->getAnimationCurves())
                doc->addObject(curve->shared_from_this(), true);
        }
    }
    return true;
}

void AnimationStack::merge(AnimationStack* src)
{
    for (auto layer : src->getAnimationLayers()) {
        if (auto l = as<AnimationLayer>(findChild(layer->getFullName())))
            l->merge(layer);
        else
            addChild(layer);
    }
}



ObjectClass AnimationLayer::getClass() const { return ObjectClass::AnimationLayer; }

void AnimationLayer::importFBXObjects()
{
    super::importFBXObjects();
}

void AnimationLayer::exportFBXObjects()
{
    super::exportFBXObjects();
}

void AnimationLayer::addChild(Object* v)
{
    super::addChild(v);
    if (auto acn = as<AnimationCurveNode>(v))
        m_anim_nodes.push_back(acn);
}

void AnimationLayer::eraseChild(Object* v)
{
    super::eraseChild(v);
    if (auto acn = as<AnimationCurveNode>(v))
        erase(m_anim_nodes, acn);
}

span<AnimationCurveNode*> AnimationLayer::getAnimationCurveNodes() const
{
    return make_span(m_anim_nodes);
}

AnimationCurveNode* AnimationLayer::createCurveNode(AnimationKind kind, Object* target)
{
    auto ret = createChild<AnimationCurveNode>();
    ret->initialize(kind, target);
    return ret;
}

void AnimationLayer::applyAnimation(float time)
{
    for (auto n : m_anim_nodes)
        n->applyAnimation(time);
}

bool AnimationLayer::remap(Document* doc)
{
    for (size_t i = 0; i < m_anim_nodes.size(); /**/) {
        auto node = m_anim_nodes[i];
        if (!node->remap(doc))
            eraseChild(node);
        else
            ++i;
    }
    return !m_anim_nodes.empty();
}

void AnimationLayer::merge(AnimationLayer* src)
{
    // make_reverse because elements maybe erased in the loop
    for (auto node : make_reverse(src->getAnimationCurveNodes())) {
        if (auto old = find_if(m_anim_nodes,
            [node](auto n) { return node->getAnimationTarget() == n->getAnimationTarget() && node->getAnimationKind() == n->getAnimationKind(); }))
        {
            old->unlink();
        }
        src->eraseChild(node);
        addChild(node);
    }
}



struct AnimationKindInfo
{
    AnimationKind kind;
    string_view object_name;
    string_view link_name;
    std::vector<string_view> curve_names;
};
static const AnimationKindInfo g_akinfo[] = {
    {AnimationKind::Position,     sfbxS_T, sfbxS_LclTranslation, {"d|X", "d|Y", "d|Z"}},
    {AnimationKind::Rotation,     sfbxS_R, sfbxS_LclRotation, {"d|X", "d|Y", "d|Z"}},
    {AnimationKind::Scale,        sfbxS_S, sfbxS_LclScale, {"d|X", "d|Y", "d|Z"}},
    {AnimationKind::Color,        sfbxS_Color, sfbxS_Color, {"d|X", "d|Y", "d|Z"}},
    {AnimationKind::Intensity,    sfbxS_Intensity, sfbxS_Intensity, {"d|" sfbxS_Intensity}},
    {AnimationKind::FocalLength,  sfbxS_FocalLength, sfbxS_FocalLength, {"d|" sfbxS_FocalLength}},
    {AnimationKind::DeformWeight, sfbxS_DeformPercent, sfbxS_DeformPercent, {"d|" sfbxS_DeformPercent}},
    {AnimationKind::filmboxTypeID, sfbxS_filmboxTypeID, sfbxS_filmboxTypeID, {"d|" sfbxS_filmboxTypeID}},
    {AnimationKind::lockInfluenceWeights, sfbxS_lockInfluenceWeights, sfbxS_lockInfluenceWeights, {"d|" sfbxS_lockInfluenceWeights}},
};
static const AnimationKindInfo* FindAnimationKindInfo(AnimationKind v)
{
    for (auto& info : g_akinfo)
        if (info.kind == v)
            return &info;
    return nullptr;
}
static const AnimationKindInfo* FindAnimationKindInfo(string_view name)
{
    for (auto& info : g_akinfo)
        if (info.object_name == name)
            return &info;
    return nullptr;
}


struct AnimationCurveInfo
{
    string_view link_name;
    string_view fbx_typename;
    PropertyType type;
    int element_index;

    void parseDefaultValueNode(Node* p, void* dst) const
    {
        switch (type) {
        case PropertyType::Int16:
            ((int32*)dst)[element_index] = (int32)GetPropertyValue<int16>(p, 4);
            break;
        case PropertyType::Int32:
            ((int32*)dst)[element_index] = (int32)GetPropertyValue<int32>(p, 4);
            break;
        case PropertyType::Float64:
            ((float32*)dst)[element_index] = (float32)GetPropertyValue<float64>(p, 4);
            break;
        default:
            break;
        }
    }

    void addDefaultValueNode(Node* props, float v) const
    {
        switch (type) {
        case PropertyType::Int16:
            props->createChild(sfbxS_P, link_name, fbx_typename, "", sfbxS_A, (int16)v);
            break;
        case PropertyType::Int32:
            props->createChild(sfbxS_P, link_name, fbx_typename, "", sfbxS_A, (int32)v);
            break;
        case PropertyType::Float64:
            props->createChild(sfbxS_P, link_name, fbx_typename, "", sfbxS_A, (float64)v);
            break;
        default:
            break;
        }
    }
};

static const AnimationCurveInfo g_acinfo[] = {
    {"d|X", sfbxS_Number, PropertyType::Float64, 0},
    {"d|Y", sfbxS_Number, PropertyType::Float64, 1},
    {"d|Z", sfbxS_Number, PropertyType::Float64, 2},
    {"d|" sfbxS_Intensity,            sfbxS_Number, PropertyType::Float64, 0},
    {"d|" sfbxS_FocalLength,          sfbxS_Number, PropertyType::Float64, 0},
    {"d|" sfbxS_DeformPercent,        sfbxS_Number, PropertyType::Float64, 0},
    {"d|" sfbxS_filmboxTypeID,        sfbxS_Short,  PropertyType::Int16,   0},
    {"d|" sfbxS_lockInfluenceWeights, sfbxS_Bool,   PropertyType::Int32,   0},
};
static const AnimationCurveInfo* FindAnimationCurveInfo(string_view name)
{
    for (auto& info : g_acinfo)
        if (info.link_name == name)
            return &info;
    return nullptr;
}


ObjectClass AnimationCurveNode::getClass() const { return ObjectClass::AnimationCurveNode; }

void AnimationCurveNode::importFBXObjects()
{
    super::importFBXObjects();

    auto name = getName();
    if (auto aki = FindAnimationKindInfo(name)) {
        m_kind = aki->kind;
        EnumerateProperties(getNode(), [this](Node* p) {
            auto link_name = GetPropertyString(p);
            if (auto aci = FindAnimationCurveInfo(link_name))
                aci->parseDefaultValueNode(p, &m_default_value);
            });
    }
    else {
        sfbxPrint("sfbx::AnimationCurveNode: unrecognized animation target \"%s\"\n", std::string(name).c_str());
    }
}

void AnimationCurveNode::exportFBXObjects()
{
    super::exportFBXObjects();

    auto props = getNode()->createChild(sfbxS_Properties70);
    if (auto* aki = FindAnimationKindInfo(getName())) {
        for (auto curve : m_curves) {
            if (auto aci = FindAnimationCurveInfo(curve->m_link_name)) {
                float v = curve->evaluate(curve->getStartTime());
                aci->addDefaultValueNode(props, v);
            }
        }
    }
}

void AnimationCurveNode::exportFBXConnections()
{
    // ignore super::constructLinks()

    m_document->createLinkOO(this, getParent());
    if (auto* info = FindAnimationKindInfo(m_kind)) {
        if (auto* target = getAnimationTarget())
            m_document->createLinkOP(this, target, info->link_name);
        for (auto curve : m_curves)
            m_document->createLinkOP(curve, this, curve->m_link_name);
    }
}

void AnimationCurveNode::addChild(Object* v)
{
    super::addChild(v);
    if (auto curve = as<AnimationCurve>(v))
        m_curves.push_back(curve);
}

void AnimationCurveNode::addChild(Object* v, string_view p)
{
    super::addChild(v, p);
    if (auto curve = as<AnimationCurve>(v)) {
        if (auto acd = FindAnimationCurveInfo(p)) {
            curve->m_link_name = p;
            curve->m_element_index = acd->element_index;
        }
    }
}

void AnimationCurveNode::eraseChild(Object* v)
{
    super::eraseChild(v);
    if (auto curve = as<AnimationCurve>(v))
        erase(m_curves, curve);
}

AnimationKind AnimationCurveNode::getAnimationKind() const
{
    return m_kind;
}

Object* AnimationCurveNode::getAnimationTarget() const
{
    for (auto p : getParents()) 
        if (!as<AnimationLayer>(p))
            return p;
    return nullptr;
}

span<AnimationCurve*> AnimationCurveNode::getAnimationCurves() const
{
    return make_span(m_curves);
}

float AnimationCurveNode::getStartTime() const
{
    if (m_curves.empty()) {
        return 0.0f;
    }
    else {
        float ret = std::numeric_limits<float>::max();
        for (auto curve : m_curves)
            ret = std::min(ret, curve->getStartTime());
        return ret;
    }
}

float AnimationCurveNode::getStopTime() const
{
    if (m_curves.empty()) {
        return 0.0f;
    }
    else {
        float ret = std::numeric_limits<float>::min();
        for (auto curve : m_curves)
            ret = std::max(ret, curve->getStopTime());
        return ret;
    }
}

float AnimationCurveNode::evaluateF1(float time) const
{
    if (m_curves.empty())
        return m_default_value.f3.x;
    return m_curves[0]->evaluate(time);
}

float3 AnimationCurveNode::evaluateF3(float time) const
{
    float3 r = m_default_value.f3;
    for (auto curve : m_curves)
        r[curve->m_element_index] = curve->evaluate(time);
    return r;
}

int AnimationCurveNode::evaluateI(float time) const
{
    if (m_curves.empty())
        return m_default_value.i;
    return (int)m_curves[0]->evaluate(time);
}

void AnimationCurveNode::applyAnimation(float time) const
{
    if (m_curves.empty() || m_kind == AnimationKind::Unknown)
        return;

    auto* target = getAnimationTarget();
    switch (m_kind) {
    case AnimationKind::Position:
        if (auto* model = as<Model>(target))
            model->setPosition(evaluateF3(time));
        break;
    case AnimationKind::Rotation:
        if (auto* model = as<Model>(target))
            model->setRotation(evaluateF3(time));
        break;
    case AnimationKind::Scale:
        if (auto* model = as<Model>(target))
            model->setScale(evaluateF3(time));
        break;
    case AnimationKind::Color:
        if (auto* light = as<Light>(target))
            light->setColor(evaluateF3(time));
        break;
    case AnimationKind::Intensity:
        if (auto* light = as<Light>(target))
            light->setIntensity(evaluateF1(time));
        break;
    case AnimationKind::FocalLength:
        if (auto cam = as<Camera>(target))
            cam->setFocalLength(evaluateF1(time));
        break;
    case AnimationKind::DeformWeight:
        if (auto* bsc = as<BlendShapeChannel>(target))
            bsc->setWeight(evaluateF1(time));
        break;
    default:
        // should not be here
        sfbxPrint("sfbx::AnimationCurveNode: something wrong\n");
        break;
    }
}

void AnimationCurveNode::initialize(AnimationKind kind, Object* target)
{
    m_kind = kind;
    if (target)
        target->addChild(this);

    if (auto* acd = FindAnimationKindInfo(m_kind)) {
        setName(acd->object_name);
        for (auto& link_name : acd->curve_names) {
            auto curve = m_document->createObject<AnimationCurve>();
            addChild(curve, link_name);
        }
    }
}

void AnimationCurveNode::addValue(float time, float value)
{
    if (m_curves.size() != 1) {
        sfbxPrint("afbx::AnimationCurveNode::addValue() curve count mismatch\n");
        return;
    }
    m_curves[0]->addValue(time, value);
}

void AnimationCurveNode::addValue(float time, float3 value)
{
    if (m_curves.size() != 3) {
        sfbxPrint("afbx::AnimationCurveNode::addValue() curve count mismatch\n");
        return;
    }
    m_curves[0]->addValue(time, value.x);
    m_curves[1]->addValue(time, value.y);
    m_curves[2]->addValue(time, value.z);
}

bool AnimationCurveNode::remap(Document* doc)
{
    if (m_kind == AnimationKind::Unknown || m_curves.empty())
        return false;
    for (auto& p : getParents()) {
        if (!as<AnimationLayer>(p)) {
            if (auto np = doc->findObject(p->getFullName())) {
                p->eraseChild(this); // erase p from m_parents. so, this loop is invalidated
                np->addChild(this);
                return true;
            }
        }
    }
    return false;
}

void AnimationCurveNode::unlink()
{
    while (!m_parents.empty())
        m_parents.back()->eraseChild(this);

    while (!m_curves.empty()) {
        auto c = m_curves.back();
        eraseChild(c);
        m_document->eraseObject(c);
    }
    m_document->eraseObject(this);
}


ObjectClass AnimationCurve::getClass() const { return ObjectClass::AnimationCurve; }

void AnimationCurve::importFBXObjects()
{
    super::importFBXObjects();

    for (auto n : getNode()->getChildren()) {
        auto name = n->getName();
        if (name == sfbxS_Default) {
            m_default = (float32)GetPropertyValue<float64>(n);
        }
        else if (name == sfbxS_KeyTime) {
            RawVector<int64> times_i64;
            GetPropertyValue<int64>(times_i64, n);
            transform(m_times, times_i64, [](int64 v) { return FromTicks(v); });
        }
        else if (name == sfbxS_KeyValueFloat) {
            GetPropertyValue<float32>(m_values, n);
        }
    }
}

void AnimationCurve::exportFBXObjects()
{
    super::exportFBXObjects();

    RawVector<int64> times_i64;
    transform(times_i64, m_times, [](float v) { return ToTicks(v); });

    auto n = getNode();
    n->createChild(sfbxS_Default, (float64)m_default);
    n->createChild(sfbxS_KeyVer, sfbxI_KeyVer);
    n->createChild(sfbxS_KeyTime, times_i64);
    n->createChild(sfbxS_KeyValueFloat, m_values); // float array

    int attr_flags[] = { 24836 };
    float attr_data[] = { 0, 0, 0, 0 };
    int attr_refcount[] = { (int)m_times.size() };
    n->createChild(sfbxS_KeyAttrFlags, make_span(attr_flags));
    n->createChild(sfbxS_KeyAttrDataFloat, make_span(attr_data));
    n->createChild(sfbxS_KeyAttrRefCount, make_span(attr_refcount));
}

void AnimationCurve::exportFBXConnections()
{
    // do nothing
}

span<float> AnimationCurve::getTimes() const { return make_span(m_times); }
span<float> AnimationCurve::getValues() const { return make_span(m_values); }

float AnimationCurve::getStartTime() const { return m_times.empty() ? 0.0f : m_times.front(); }
float AnimationCurve::getStopTime() const { return m_times.empty() ? 0.0f : m_times.back(); }

float AnimationCurve::evaluate(float time) const
{
    if (m_times.empty())
        return m_default;
    else if (time <= m_times.front())
        return m_values.front();
    else if (time >= m_times.back())
        return m_values.back();
    else {
        // lerp
        auto it = std::lower_bound(m_times.begin(), m_times.end(), time);
        size_t i = std::distance(m_times.begin(), it);

        float t2 = m_times[i];
        float v2 = m_values[i];
        if (time == t2)
            return v2;

        float t1 = m_times[i - 1];
        float v1 = m_values[i - 1];
        float w = (time - t1) / (t2 - t1);
        return v1 + (v2 - v1) * w;
    }
}

void AnimationCurve::setTimes(span<float> v) { m_times = v; }
void AnimationCurve::setValues(span<float> v) { m_values = v; }

void AnimationCurve::addValue(float time, float value)
{
    m_times.push_back(time);
    m_values.push_back(value);
}

} // namespace sfbx
