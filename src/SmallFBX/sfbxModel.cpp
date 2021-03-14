#include "pch.h"
#include "sfbxInternal.h"
#include "sfbxModel.h"
#include "sfbxGeometry.h"
#include "sfbxMaterial.h"

namespace sfbx {

ObjectClass NodeAttribute::getClass() const
{
    return ObjectClass::NodeAttribute;
}

ObjectSubClass NullAttribute::getSubClass() const { return ObjectSubClass::Null; }

void NullAttribute::exportFBXObjects()
{
    super::exportFBXObjects();
    getNode()->createChild(sfbxS_TypeFlags, sfbxS_Null);
}

ObjectSubClass RootAttribute::getSubClass() const { return ObjectSubClass::Root; }

void RootAttribute::exportFBXObjects()
{
    super::exportFBXObjects();
    getNode()->createChild(sfbxS_TypeFlags, sfbxS_Null, sfbxS_Skeleton, sfbxS_Root);
}

ObjectSubClass LimbNodeAttribute::getSubClass() const { return ObjectSubClass::LimbNode; }

void LimbNodeAttribute::exportFBXObjects()
{
    super::exportFBXObjects();
    getNode()->createChild(sfbxS_TypeFlags, sfbxS_Skeleton);
}

ObjectSubClass LightAttribute::getSubClass() const { return ObjectSubClass::Light; }

ObjectSubClass CameraAttribute::getSubClass() const { return ObjectSubClass::Camera; }



ObjectClass Model::getClass() const { return ObjectClass::Model; }

void Model::importFBXObjects()
{
    super::importFBXObjects();
    auto n = getNode();
    if (!n)
        return;

    EnumerateProperties(n, [this](Node* p) {
        auto get_int = [p]() -> int {
            if (GetPropertyCount(p) == 5)
                return GetPropertyValue<int32>(p, 4);
#ifdef sfbxEnableLegacyFormatSupport
            else if (GetPropertyCount(p) == 4) {
                return GetPropertyValue<int32>(p, 3);
            }
#endif
            return 0;
        };

        auto get_float3 = [p]() -> float3 {
            if (GetPropertyCount(p) == 7) {
                return float3{
                    (float)GetPropertyValue<float64>(p, 4),
                    (float)GetPropertyValue<float64>(p, 5),
                    (float)GetPropertyValue<float64>(p, 6),
                };
            }
#ifdef sfbxEnableLegacyFormatSupport
            else if (GetPropertyCount(p) == 6) {
                return float3{
                    (float)GetPropertyValue<float64>(p, 3),
                    (float)GetPropertyValue<float64>(p, 4),
                    (float)GetPropertyValue<float64>(p, 5),
                };
            }
#endif
            return {};
        };

        auto pname = GetPropertyString(p);
        if (pname == sfbxS_Visibility) {
            m_visibility = GetPropertyValue<bool>(p, 4);
        }
        else if (pname == sfbxS_LclTranslation)
            m_position = get_float3();
        else if (pname == sfbxS_RotationOrder)
            m_rotation_order = (RotationOrder)get_int();
        else if (pname == sfbxS_PreRotation)
            m_pre_rotation = get_float3();
        else if (pname == sfbxS_PostRotation)
            m_post_rotation = get_float3();
        else if (pname == sfbxS_LclRotation)
            m_rotation = get_float3();
        else if (pname == sfbxS_LclScale)
            m_scale = get_float3();
        });
}

#define sfbxVector3d(V) (float64)V.x, (float64)V.y, (float64)V.z

void Model::exportFBXObjects()
{
    super::exportFBXObjects();
    auto n = getNode();
    if (!n)
        return;

    // version
    n->createChild(sfbxS_Version, sfbxI_ModelVersion);

    auto properties = n->createChild(sfbxS_Properties70);

    // attribute
    properties->createChild(sfbxS_P, "DefaultAttributeIndex", "int", "Integer", "", 0);

    // position
    if (m_position != float3::zero())
        properties->createChild(sfbxS_P,
            sfbxS_LclTranslation, sfbxS_LclTranslation, sfbxS_Empty, sfbxS_A, sfbxVector3d(m_position));

    // rotation
    if (m_pre_rotation != float3::zero() || m_post_rotation != float3::zero() || m_rotation != float3::zero()) {
        // rotation active
        properties->createChild(sfbxS_P,
            sfbxS_RotationActive, sfbxS_bool, sfbxS_Empty, sfbxS_Empty, (int32)1);
        // rotation order
        if (m_rotation_order != RotationOrder::XYZ)
            properties->createChild(sfbxS_P,
                sfbxS_RotationOrder, sfbxS_RotationOrder, sfbxS_Empty, sfbxS_A, (int32)m_rotation_order);
        // pre-rotation
        if (m_pre_rotation != float3::zero())
            properties->createChild(sfbxS_P,
                sfbxS_PreRotation, sfbxS_Vector3D, sfbxS_Vector, sfbxS_Empty, sfbxVector3d(m_pre_rotation));
        // post-rotation
        if (m_post_rotation != float3::zero())
            properties->createChild(sfbxS_P,
                sfbxS_PostRotation, sfbxS_Vector3D, sfbxS_Vector, sfbxS_Empty, sfbxVector3d(m_post_rotation));
        // rotation
        if (m_rotation != float3::zero())
            properties->createChild(sfbxS_P,
                sfbxS_LclRotation, sfbxS_LclRotation, sfbxS_Empty, sfbxS_A, sfbxVector3d(m_rotation));
    }

    // scale
    if (m_scale!= float3::one())
        properties->createChild(sfbxS_P,
            sfbxS_LclScale, sfbxS_LclScale, sfbxS_Empty, sfbxS_A, sfbxVector3d(m_scale));
}

void Model::addChild(Object* v)
{
    super::addChild(v);
    if (auto model = as<Model>(v))
        m_child_models.push_back(model);
}

void Model::eraseChild(Object* v)
{
    super::eraseChild(v);
    if (auto model = as<Model>(v))
        erase(m_child_models, model);
}

void Model::addParent(Object* v)
{
    super::addParent(v);
    if (auto model = as<Model>(v))
        m_parent_model = model;
}

void Model::eraseParent(Object* v)
{
    super::eraseParent(v);
    if (v == m_parent_model)
        m_parent_model = nullptr;
}

Model* Model::getParentModel() const { return m_parent_model; }

bool Model::getVisibility() const { return m_visibility; }
RotationOrder Model::getRotationOrder() const { return m_rotation_order; }
float3 Model::getPosition() const { return m_position; }

float3 Model::getPreRotation() const { return m_pre_rotation; }
float3 Model::getRotation() const { return m_rotation; }
float3 Model::getPostRotation() const { return m_post_rotation; }
float3 Model::getScale() const { return m_scale; }

void Model::updateMatrices() const
{
    if (m_matrix_dirty) {
        // scale
        float4x4 r = scale44(m_scale);

        // rotation
        if (m_post_rotation != float3::zero())
            r *= transpose(to_mat4x4(rotate_euler(m_rotation_order, m_post_rotation * DegToRad)));
        if (m_rotation != float3::zero())
            r *= transpose(to_mat4x4(rotate_euler(m_rotation_order, m_rotation * DegToRad)));
        if (m_pre_rotation != float3::zero())
            r *= transpose(to_mat4x4(rotate_euler(m_rotation_order, m_pre_rotation * DegToRad)));

        // translation
        (float3&)r[3] = m_position;

        m_matrix_local = r;
        m_matrix_global = m_matrix_local;
        if (m_parent_model)
            m_matrix_global *= m_parent_model->getGlobalMatrix();

        m_matrix_dirty = false;
    }
}

float4x4 Model::getLocalMatrix() const
{
    updateMatrices();
    return m_matrix_local;
}

float4x4 Model::getGlobalMatrix() const
{
    updateMatrices();
    return m_matrix_global;
}

void Model::setVisibility(bool v) { m_visibility = v; }
void Model::setRotationOrder(RotationOrder v) { m_rotation_order = v; }

void Model::propagateDirty()
{
    // note:
    // this is needlessly slow on huge joint structure + animations.
    // we need to separate update transform phase and propagate dirty phase for optimal animation playback.
    m_matrix_dirty = true;
    for (auto c : m_child_models)
        c->propagateDirty();
}

#define MarkDirty(V, A) if (A != V) { V = A; propagateDirty(); }
void Model::setPosition(float3 v)     { MarkDirty(m_position, v); }
void Model::setPreRotation(float3 v)  { MarkDirty(m_pre_rotation, v); }
void Model::setRotation(float3 v)     { MarkDirty(m_rotation, v); }
void Model::setPostRotation(float3 v) { MarkDirty(m_post_rotation, v); }
void Model::setScale(float3 v)        { MarkDirty(m_scale, v); }
#undef MarkDirty



ObjectSubClass Null::getSubClass() const { return ObjectSubClass::Null; }

void Null::exportFBXObjects()
{
    if (!m_attr)
        m_attr = createChild<NullAttribute>();
    super::exportFBXObjects();
}

void Null::addChild(Object* v)
{
    super::addChild(v);
    if (auto attr = as<NullAttribute>(v))
        m_attr = attr;
}

void Null::eraseChild(Object* v)
{
    super::eraseChild(v);
    if (v == m_attr)
        m_attr = nullptr;
}



ObjectSubClass Root::getSubClass() const { return ObjectSubClass::Root; }

void Root::exportFBXObjects()
{
    if (!m_attr)
        m_attr = createChild<RootAttribute>();
    super::exportFBXObjects();
}

void Root::addChild(Object* v)
{
    super::addChild(v);
    if (auto attr = as<RootAttribute>(v))
        m_attr = attr;
}

void Root::eraseChild(Object* v)
{
    super::eraseChild(v);
    if (v == m_attr)
        m_attr = nullptr;
}



ObjectSubClass LimbNode::getSubClass() const { return ObjectSubClass::LimbNode; }

void LimbNode::exportFBXObjects()
{
    if (!m_attr)
        m_attr = createChild<LimbNodeAttribute>();
    super::exportFBXObjects();
}


void LimbNode::addChild(Object* v)
{
    super::addChild(v);
    if (auto attr = as<LimbNodeAttribute>(v))
        m_attr = attr;
}


void LimbNode::eraseChild(Object* v)
{
    super::eraseChild(v);
    if (v == m_attr)
        m_attr = nullptr;
}



ObjectSubClass Mesh::getSubClass() const { return ObjectSubClass::Mesh; }

void Mesh::importFBXObjects()
{
    super::importFBXObjects();

#ifdef sfbxEnableLegacyFormatSupport
    // in old fbx, Model::Mesh has geometry data (Geometry::Mesh does not exist)
    auto n = getNode();
    if (n->findChild(sfbxS_Vertices)) {
        getGeometry()->setNode(n);
    }
#endif
}

void Mesh::addChild(Object* v)
{
    super::addChild(v);
    if (auto geom = as<GeomMesh>(v))
        m_geom = geom;
    else if (auto material = as<Material>(v))
        m_materials.push_back(material);
}

void Mesh::eraseChild(Object* v)
{
    super::eraseChild(v);
    if (v == m_geom)
        m_geom = nullptr;
    else if (auto material = as<Material>(v))
        erase(m_materials, material);
}

GeomMesh* Mesh::getGeometry()
{
    if (!m_geom)
        m_geom = createChild<GeomMesh>(getName());
    return m_geom;
}

span<Material*> Mesh::getMaterials() const
{
    return make_span(m_materials);
}



ObjectSubClass Light::getSubClass() const { return ObjectSubClass::Light; }

void Light::importFBXObjects()
{
    super::importFBXObjects();
}

void LightAttribute::importFBXObjects()
{
    super::importFBXObjects();

    auto light = as<Light>(getParent());
    if (!light)
        return;

    EnumerateProperties(getNode(), [light](Node* p) {
        auto name = GetPropertyString(p, 0);
        if (name == sfbxS_LightType)
            light->m_light_type = (LightType)GetPropertyValue<int32>(p, 4);
        else if (name == sfbxS_Color)
            light->m_color = float3{
                    (float32)GetPropertyValue<float64>(p, 4),
                    (float32)GetPropertyValue<float64>(p, 5),
                    (float32)GetPropertyValue<float64>(p, 6)};
        else if (name == sfbxS_Intensity)
            light->m_intensity = (float32)GetPropertyValue<float64>(p, 4);
        else if (name == sfbxS_InnerAngle)
            light->m_inner_angle = (float32)GetPropertyValue<float64>(p, 4);
        else if (name == sfbxS_OuterAngle)
            light->m_outer_angle = (float32)GetPropertyValue<float64>(p, 4);
        });
}

void Light::exportFBXObjects()
{
    if (!m_attr)
        m_attr = createChild<LightAttribute>();
    super::exportFBXObjects();
}

void LightAttribute::exportFBXObjects()
{
    super::exportFBXObjects();

    auto light = as<Light>(getParent());
    if (!light)
        return;

    auto color = light->m_color;
    auto props = getNode()->createChild(sfbxS_Properties70);
    props->createChild(sfbxS_P, sfbxS_LightType, sfbxS_enum, "", "", (int32)light->m_light_type);
    props->createChild(sfbxS_P, sfbxS_Color, sfbxS_Color, "", "A", (float64)color.x, (float64)color.y, (float64)color.z);
    props->createChild(sfbxS_P, sfbxS_Intensity, sfbxS_Number, "", "A", (float64)light->m_intensity);
    if (light->m_light_type == LightType::Spot) {
        props->createChild(sfbxS_P, sfbxS_InnerAngle, sfbxS_Number, "", "A", (float64)light->m_inner_angle);
        props->createChild(sfbxS_P, sfbxS_OuterAngle, sfbxS_Number, "", "A", (float64)light->m_outer_angle);
    }
}

void Light::addChild(Object* v)
{
    super::addChild(v);
    if (auto attr = as<LightAttribute>(v))
        m_attr = attr;
}

void Light::eraseChild(Object* v)
{
    super::eraseChild(v);
    if (v == m_attr)
        m_attr = nullptr;
}

LightType Light::getLightType() const { return m_light_type; }
float3 Light::getColor() const { return m_color; }
float Light::getIntensity() const { return m_intensity; }
float Light::getInnerAngle() const { return m_inner_angle; }
float Light::getOuterAngle() const { return m_outer_angle; }

void Light::setLightType(LightType v) { m_light_type = v; }
void Light::setColor(float3 v) { m_color = v; }
void Light::setIntensity(float v) { m_intensity = v; }
void Light::setInnerAngle(float v) { m_inner_angle = v; }
void Light::setOuterAngle(float v) { m_outer_angle = v; }



ObjectSubClass Camera::getSubClass() const { return ObjectSubClass::Camera; }

void Camera::importFBXObjects()
{
    super::importFBXObjects();
}

void CameraAttribute::importFBXObjects()
{
    super::importFBXObjects();

    auto cam = as<Camera>(getParent());
    if (!cam)
        return;

    EnumerateProperties(getNode(), [cam](Node* p) {
        auto name = GetPropertyString(p, 0);
        if (name == sfbxS_CameraProjectionType)
            cam->m_camera_type = (CameraType)GetPropertyValue<int32>(p, 4);
        else if (name == sfbxS_FocalLength)
            cam->m_focal_length = (float32)GetPropertyValue<float64>(p, 4);
        else if (name == sfbxS_FilmWidth)
            cam->m_aperture.x = (float32)GetPropertyValue<float64>(p, 4) * InchToMillimeter;
        else if (name == sfbxS_FilmHeight)
            cam->m_aperture.y = (float32)GetPropertyValue<float64>(p, 4) * InchToMillimeter;
        else if (name == sfbxS_FilmOffsetX)
            cam->m_lens_shift.x = (float32)GetPropertyValue<float64>(p, 4) * InchToMillimeter;
        else if (name == sfbxS_FilmOffsetY)
            cam->m_lens_shift.y = (float32)GetPropertyValue<float64>(p, 4) * InchToMillimeter;
        else if (name == sfbxS_NearPlane)
            cam->m_near_plane = (float32)GetPropertyValue<float64>(p, 4);
        else if (name == sfbxS_FarPlane)
            cam->m_far_plane = (float32)GetPropertyValue<float64>(p, 4);
        });
}

void Camera::exportFBXObjects()
{
    if (!m_attr)
        m_attr = createChild<CameraAttribute>();
    super::exportFBXObjects();
}

void CameraAttribute::exportFBXObjects()
{
    super::exportFBXObjects();

    auto cam = as<Camera>(getParent());
    if (!cam)
        return;

    auto props = getNode()->createChild(sfbxS_Properties70);
    props->createChild(sfbxS_P, sfbxS_CameraProjectionType, sfbxS_enum, "", "", (int32)cam->m_camera_type);
    props->createChild(sfbxS_P, sfbxS_FocalLength, sfbxS_Number, "", "A", (float64)cam->m_focal_length);
    props->createChild(sfbxS_P, sfbxS_FilmWidth, sfbxS_Number, "", "A", (float64)cam->m_aperture.x * MillimeterToInch);
    props->createChild(sfbxS_P, sfbxS_FilmHeight, sfbxS_Number, "", "A", (float64)cam->m_aperture.y * MillimeterToInch);
    if (cam->m_lens_shift.x != 0.0f)
        props->createChild(sfbxS_P, sfbxS_FilmOffsetX, sfbxS_Number, "", "A", (float64)cam->m_lens_shift.x * MillimeterToInch);
    if (cam->m_lens_shift.y != 0.0f)
        props->createChild(sfbxS_P, sfbxS_FilmOffsetY, sfbxS_Number, "", "A", (float64)cam->m_lens_shift.y * MillimeterToInch);
    props->createChild(sfbxS_P, sfbxS_NearPlane, sfbxS_Number, "", "A", (float64)cam->m_near_plane);
    props->createChild(sfbxS_P, sfbxS_FarPlane, sfbxS_Number, "", "A", (float64)cam->m_far_plane);
}

void Camera::addChild(Object* v)
{
    super::addChild(v);
    if (auto attr = as<CameraAttribute>(v))
        m_attr = attr;
}

void Camera::eraseChild(Object* v)
{
    super::eraseChild(v);
    if (v == m_attr)
        m_attr = nullptr;
}

CameraType Camera::getCameraType() const { return m_camera_type; }
float3 Camera::getUpVector() const { return m_up_vector; }
float Camera::getFocalLength() const { return m_focal_length; }
float2 Camera::getApertureSize() const { return m_aperture; }
float2 Camera::getLensShift() const { return m_lens_shift; }
float2 Camera::getAspectSize() const { return m_aspect; }
float2 Camera::getFildOfView() const
{
    return float2{
        compute_fov(m_aperture.x, m_focal_length),
        compute_fov(m_aperture.y, m_focal_length),
    };
}
float Camera::getNearPlane() const { return m_near_plane; }
float Camera::getFarPlane() const { return m_far_plane; }


void Camera::setCameraType(CameraType v) { m_camera_type = v; }
void Camera::setUpVector(float3 v) { m_up_vector = v; }
void Camera::setFocalLength(float v) { m_focal_length = v; }
void Camera::setApetrueSize(float2 v) { m_aperture = v; }
void Camera::setLensShift(float2 v) { m_lens_shift = v; }
void Camera::setAspectSize(float2 v) { m_aspect = v; }
void Camera::setNearPlane(float v) { m_near_plane = v; }
void Camera::setFarPlane(float v) { m_far_plane = v; }

} // namespace sfbx
