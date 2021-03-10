#pragma once
#include "sfbxObject.h"

namespace sfbx {

// animation-related classes
// (AnimationStack, AnimationLayer, AnimationCurveNode, AnimationCurve)

enum class AnimationKind
{
    Unknown,
    Position,       // float3
    Rotation,       // float3
    Scale,          // float3
    DeformWeight,   // float
    FocalLength,    // float
    filmboxTypeID,  // int16
    lockInfluenceWeights, // int32
};

class AnimationStack : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;
    void addChild(Object* v) override;
    void eraseChild(Object* v) override;

    float getLocalStart() const;
    float getLocalStop() const;
    float getReferenceStart() const;
    float getReferenceStop() const;
    span<AnimationLayer*> getAnimationLayers() const;

    AnimationLayer* createLayer(string_view name = {});

    void applyAnimation(float time);

    bool remap(Document* doc);
    bool remap(DocumentPtr doc) { return remap(doc.get()); }
    void merge(AnimationStack* src); // merge src into this

protected:
    void importFBXObjects() override;
    void exportFBXObjects() override;

    float m_local_start{};
    float m_local_stop{};
    float m_reference_start{};
    float m_reference_stop{};
    std::vector<AnimationLayer*> m_anim_layers;
};

class AnimationLayer : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;
    void addChild(Object* v) override;
    void eraseChild(Object* v) override;

    span<AnimationCurveNode*> getAnimationCurveNodes() const;

    AnimationCurveNode* createCurveNode(AnimationKind kind, Object* target);

    void applyAnimation(float time);

    bool remap(Document* doc);
    void merge(AnimationLayer* src);

protected:
    void importFBXObjects() override;
    void exportFBXObjects() override;

    std::vector<AnimationCurveNode*> m_anim_nodes;
};

class AnimationCurveNode : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;
    void addChild(Object* v) override;
    void eraseChild(Object* v) override;

    AnimationKind getAnimationKind() const;
    Object* getAnimationTarget() const;
    span<AnimationCurve*> getAnimationCurves() const;
    float getStartTime() const;
    float getStopTime() const;

    // evaluate curve(s)
    float evaluate(float time) const;
    float3 evaluate3(float time) const;

    // apply evaluated value to target
    void applyAnimation(float time) const;

    void initialize(AnimationKind kind, Object* target);
    void addValue(float time, float value);
    void addValue(float time, float3 value);

    bool remap(Document* doc);

protected:
    void importFBXObjects() override;
    void exportFBXObjects() override;
    void exportFBXConnections() override;

    AnimationKind m_kind = AnimationKind::Unknown;
    std::vector<AnimationCurve*> m_curves;
};

class AnimationCurve : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;

    span<float> getTimes() const;
    span<float> getValues() const;
    float getStartTime() const;
    float getStopTime() const;
    float evaluate(float time) const;

    void setTimes(span<float> v);
    void setValues(span<float> v);
    void addValue(float time, float value);

protected:
    void importFBXObjects() override;
    void exportFBXObjects() override;
    void exportFBXConnections() override;

    float m_default{};
    RawVector<float> m_times;
    RawVector<float> m_values;
};

} // namespace sfbx
