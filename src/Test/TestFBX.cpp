#include "pch.h"
#include "Test.h"
#include "SmallFBX.h"

using sfbx::as;
using sfbx::span;
using sfbx::make_span;
using sfbx::RawVector;
using sfbx::float3;

static void PrintObject(sfbx::Object* obj, int depth = 0)
{
    // indent
    for (int i = 0; i < depth; ++i)
        testPrint("  ");

    testPrint("\"%s\" [0x%llx] (%s : %s)\n",
        obj->getFullName().data(),
        obj->getID(),
        sfbx::GetObjectClassName(obj->getClass()).data(),
        sfbx::GetObjectSubClassName(obj->getSubClass()).data());

    // for test
    if (auto skin = as<sfbx::Skin>(obj)) {
        auto weights_v = skin->getJointWeights();
        auto weights_4 = skin->createFixedJointWeights(4);
        auto matrices = skin->getJointMatrices();
        testPrint("");
    }
    else if (auto anim = as<sfbx::AnimationLayer>(obj)) {
        for (auto n : anim->getAnimationCurveNodes()) {
            if (n->getAnimationKind() == sfbx::AnimationKind::Position) {
                float start = n->getStartTime();
                float stop = n->getStopTime();
                for (float t = start; t <= stop; t += 0.033334f) {
                    auto v = n->evaluate3(t);
                    testPrint("");
                }
            }
        }
    }

    if (!as<sfbx::Cluster>(obj)) {
        // avoid printing children of Cluster because it may result in a nearly endless list

        for (auto child : obj->getChildren())
            PrintObject(child, depth + 1);

    }
}

testCase(fbxRead)
{
    std::string path, path2, path3;
    test::GetArg("path", path);
    test::GetArg("path2", path2);
    test::GetArg("path3", path3);
    if (path.empty())
        return;

    sfbx::DocumentPtr doc = sfbx::MakeDocument();
    if (doc->read(path)) {
        if (!path2.empty()) {
            sfbx::DocumentPtr doc2 = sfbx::MakeDocument();
            if (doc2->read(path2)) {
                auto takes = doc2->getAnimationStacks();
                if (!takes.empty()) {
                    if (takes[0]->remap(doc)) {
                        doc->setCurrentTake(takes[0]);
                    }
                }
            }
        }
        if (!path3.empty()) {
            sfbx::DocumentPtr doc3 = sfbx::MakeDocument();
            if (doc3->read(path3)) {
                auto takes = doc3->getAnimationStacks();
                if (!takes.empty()) {
                    if (takes[0]->remap(doc)) {
                        doc->setCurrentTake(takes[0]);
                    }
                }
            }
        }

        testPrint("Objects:\n");
        for (auto obj : doc->getRootObjects())
            PrintObject(obj);

        doc->writeBinary("out_b.fbx");
        doc->writeAscii("out_a.fbx");
    }

}

testCase(fbxWrite)
{
    {
        sfbx::DocumentPtr doc = sfbx::MakeDocument();
        sfbx::Model* root = doc->getRootModel();

        sfbx::Mesh* node = root->createChild<sfbx::Mesh>("mesh");
        node->setPosition({ 0.0f, 0.0f, 0.0f });
        node->setRotation({ 0.0f, 0.0f, 0.0f });
        node->setScale({ 1.0f, 1.0f, 1.0f });

        float s = 10.0f;
        sfbx::GeomMesh* mesh = node->getGeometry();

        {
            // counts & indices & points
            int counts[]{
                4, 4, 4, 4,
            };
            int indices[]{
                0, 1, 3, 2,
                2, 3, 5, 4,
                4, 5, 7, 6,
                6, 7, 9, 8,
            };
            float3 points[]{
                {-s * 0.5f, s * 0, 0}, {s * 0.5f, s * 0, 0},
                {-s * 0.5f, s * 1, 0}, {s * 0.5f, s * 1, 0},
                {-s * 0.5f, s * 2, 0}, {s * 0.5f, s * 2, 0},
                {-s * 0.5f, s * 3, 0}, {s * 0.5f, s * 3, 0},
                {-s * 0.5f, s * 4, 0}, {s * 0.5f, s * 4, 0},
            };
            mesh->setCounts(counts);
            mesh->setIndices(indices);
            mesh->setPoints(points);

            // normals
            {
                sfbx::LayerElementF3 normals;
                normals.data = {
                    { 0, 0, -1}, { 0, 0, -1},
                    { 0, 0, -1}, { 0, 0, -1},
                    { 0, 0, -1}, { 0, 0, -1},
                    { 0, 0, -1}, { 0, 0, -1},
                    { 0, 0, -1}, { 0, 0, -1},
                };
                normals.indices = indices;
                mesh->addNormalLayer(std::move(normals));
            }

            // uv
            {
                sfbx::LayerElementF2 uv;
                uv.data = {
                    { 0, 0.00f }, { 1, 0.00f },
                    { 0, 0.25f }, { 1, 0.25f },
                    { 0, 0.50f }, { 1, 0.50f },
                    { 0, 0.75f }, { 1, 0.75f },
                    { 0, 1.00f }, { 1, 1.00f },
                };
                uv.indices = indices;
                mesh->addUVLayer(std::move(uv));
            }

            // colors
            {
                sfbx::LayerElementF4 colors;
                colors.data = {
                    { 1, 0, 0, 1}, { 0, 1, 0, 1},
                    { 0, 0, 1, 1}, { 0, 0, 0, 1},
                    { 1, 0, 0, 1}, { 0, 1, 0, 1},
                    { 0, 0, 1, 1}, { 0, 0, 0, 1},
                    { 0, 0, 1, 1}, { 0, 0, 0, 1},
                };
                colors.indices = indices;
                mesh->addColorLayer(std::move(colors));

            }
        }

        // blend shape
        sfbx::BlendShape* blendshape{};
        sfbx::BlendShapeChannel* bschannel{};
        {
            int indices[]{
                6, 7, 8, 9,
            };
            float3 delta_points[]{
                {-s, 0, 0}, {s, 0, 0},
                {-s, 0, 0}, {s, 0, 0},
            };

            sfbx::Shape* shape = doc->createObject<sfbx::Shape>("shape");
            shape->setIndices(indices);
            shape->setDeltaPoints(delta_points);

            blendshape = mesh->createDeformer<sfbx::BlendShape>();
            bschannel = blendshape->createChannel(shape);
        }

        // joints & skin
        sfbx::Model* joints[5]{};
        sfbx::Skin* skin{};
        {
            joints[0] = root->createChild<sfbx::Root>("joint1");
            joints[1] = joints[0]->createChild<sfbx::LimbNode>("joint2");
            joints[2] = joints[1]->createChild<sfbx::LimbNode>("joint3");
            joints[3] = joints[2]->createChild<sfbx::LimbNode>("joint4");
            joints[4] = joints[3]->createChild<sfbx::LimbNode>("joint5");
            for (int i = 1; i < 5; ++i)
                joints[i]->setPosition({ 0, s, 0 });

            skin = mesh->createDeformer<sfbx::Skin>();
            for (int i = 0; i < 5; ++i) {
                sfbx::Cluster* cluster = skin->createCluster(joints[i]);
                int indices[2]{ i * 2 + 0, i * 2 + 1 };
                float weights[2]{ 1.0f, 1.0f };
                cluster->setIndices(make_span(indices));
                cluster->setWeights(make_span(weights));
                cluster->setBindMatrix(joints[i]->getGlobalMatrix());
            }
        }

        // animation
        {
            sfbx::AnimationStack* take = doc->createObject<sfbx::AnimationStack>("take1");
            sfbx::AnimationLayer* layer = take->createLayer("deform");
            sfbx::AnimationCurveNode* n1 = layer->createCurveNode(sfbx::AnimationKind::Rotation, joints[1]);
            n1->addValue(0.0f, float3{  0.0f, 0.0f, 0.0f });
            n1->addValue(3.0f, float3{ 30.0f, 0.0f, 0.0f });
            n1->addValue(6.0f, float3{  0.0f, 0.0f, 0.0f });
            n1->addValue(9.0f, float3{-30.0f, 0.0f, 0.0f });

            sfbx::AnimationCurveNode* bsw = layer->createCurveNode(sfbx::AnimationKind::DeformWeight, bschannel);
            bsw->addValue(0.0f,   0.0f);
            bsw->addValue(4.5f, 100.0f);
            bsw->addValue(9.0f,   0.0f);

            doc->setCurrentTake(take);
        }


        doc->constructNodes();
        doc->writeBinary("test_base_bin.fbx");
        doc->writeAscii("test_base_ascii.fbx");
    }
    {
        sfbx::DocumentPtr doc = sfbx::MakeDocument();
        sfbx::Model* root = doc->getRootModel();

        sfbx::Model* joints[5]{};
        joints[0] = root->createChild<sfbx::Root>("joint1");
        joints[1] = joints[0]->createChild<sfbx::LimbNode>("joint2");
        joints[2] = joints[1]->createChild<sfbx::LimbNode>("joint3");
        joints[3] = joints[2]->createChild<sfbx::LimbNode>("joint4");
        joints[4] = joints[3]->createChild<sfbx::LimbNode>("joint5");


        // animation
        {
            sfbx::AnimationStack* take = doc->createObject<sfbx::AnimationStack>("take1");
            sfbx::AnimationLayer* layer = take->createLayer("deform");
            sfbx::AnimationCurveNode* n1 = layer->createCurveNode(sfbx::AnimationKind::Rotation, joints[1]);
            n1->addValue(0.0f, float3{ 0.0f, 0.0f, 0.0f });
            n1->addValue(3.0f, float3{ 30.0f, 0.0f, 0.0f });
            n1->addValue(6.0f, float3{ 0.0f, 0.0f, 0.0f });
            n1->addValue(9.0f, float3{ -30.0f, 0.0f, 0.0f });
            n1->addValue(12.0f, float3{ 0.0f, 0.0f, 0.0f });
        }

        doc->constructNodes();
        doc->writeBinary("test_anim_bin.fbx");
        doc->writeAscii("test_anim_ascii.fbx");
    }


    {
        sfbx::DocumentPtr base = sfbx::MakeDocument();
        base->read("test_base_bin.fbx");

        {
            sfbx::DocumentPtr anim = sfbx::MakeDocument();
            anim->read("test_anim_bin.fbx");
            auto takes = anim->getAnimationStacks();
            if (!takes.empty()) {
                if (takes[0]->remap(base)) {
                    base->setCurrentTake(takes[0]);
                }
            }
        }
        if (auto take = base->getCurrentTake()) {
            take->applyAnimation(11.0f);
        }
    }
}

testCase(fbxAnimationCurve)
{
    sfbx::DocumentPtr doc = sfbx::MakeDocument();
    auto curve = doc->createObject<sfbx::AnimationCurve>("TestCurve");

    float times[]{ 0.0f, 1.0f, 2.0f };
    float values[]{ 0.0f, 100.0f, 400.0f };
    curve->setTimes(times);
    curve->setValues(values);

    for (float t = -0.5f; t < 2.5f; t += 0.1f) {
        printf("time: %f, value: %f\n", t, curve->evaluate(t));
    }
}
