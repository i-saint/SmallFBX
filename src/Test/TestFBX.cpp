#include "pch.h"
#include "Test.h"
#include "SmallFBX.h"

using sfbx::as;
using sfbx::span;
using sfbx::make_span;
using sfbx::float3;

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

        // joints & skin & bindpose
        sfbx::Model* joints[5]{};
        sfbx::Skin* skin{};
        sfbx::BindPose* bind_pose{};
        {
            joints[0] = root->createChild<sfbx::LimbNode>("joint1");
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
                cluster->setIndices(indices);
                cluster->setWeights(weights);
                cluster->setBindMatrix(joints[i]->getGlobalMatrix());
            }

            bind_pose = doc->createObject<sfbx::BindPose>();
            bind_pose->addPoseData(node, node->getGlobalMatrix());
            for (int i = 0; i < 5; ++i)
                bind_pose->addPoseData(joints[i], joints[i]->getGlobalMatrix());
        }

        // animation
        {
            sfbx::AnimationStack* take = doc->createObject<sfbx::AnimationStack>("take1");
            sfbx::AnimationLayer* layer = take->createLayer("deform");

            sfbx::AnimationCurveNode* n1 = layer->createCurveNode(sfbx::AnimationKind::Rotation, joints[1]);
            n1->addValue(0.0f, float3{  0.0f, 0.0f, 0.0f });
            n1->addValue(1.0f, float3{ 30.0f, 0.0f, 0.0f });
            n1->addValue(2.0f, float3{  0.0f, 0.0f, 0.0f });
            n1->addValue(3.0f, float3{-30.0f, 0.0f, 0.0f });

            sfbx::AnimationCurveNode* bsw = layer->createCurveNode(sfbx::AnimationKind::DeformWeight, bschannel);
            bsw->addValue(0.0f, 0.0f);
            bsw->addValue(1.5f, 1.0f);
            bsw->addValue(3.0f, 0.0f);

            doc->setCurrentTake(take);
        }


        doc->exportFBXNodes();
        doc->writeBinary("test_base_bin.fbx");
        doc->writeAscii("test_base_ascii.fbx");
    }

    {
        sfbx::DocumentPtr doc = sfbx::MakeDocument();
        sfbx::Model* root = doc->getRootModel();

        sfbx::Model* joints[5]{};
        joints[0] = root->createChild<sfbx::LimbNode>("joint1");
        joints[1] = joints[0]->createChild<sfbx::LimbNode>("joint2");
        joints[2] = joints[1]->createChild<sfbx::LimbNode>("joint3");
        joints[3] = joints[2]->createChild<sfbx::LimbNode>("joint4");
        joints[4] = joints[3]->createChild<sfbx::LimbNode>("joint5");


        // animation
        {
            sfbx::AnimationStack* take = doc->createObject<sfbx::AnimationStack>("take1");
            sfbx::AnimationLayer* layer = take->createLayer("deform");
            sfbx::AnimationCurveNode* n1 = layer->createCurveNode(sfbx::AnimationKind::Rotation, joints[1]);
            n1->addValue(0.0f, float3{  0.0f, 0.0f, 0.0f });
            n1->addValue(1.0f, float3{ 30.0f, 0.0f, 0.0f });
            n1->addValue(2.0f, float3{  0.0f, 0.0f, 0.0f });
            n1->addValue(3.0f, float3{-30.0f, 0.0f, 0.0f });
            n1->addValue(4.0f, float3{  0.0f, 0.0f, 0.0f });
        }

        doc->exportFBXNodes();
        doc->writeBinary("test_anim_bin.fbx");
        doc->writeAscii("test_anim_ascii.fbx");
    }


    {
        // test animation remap
        sfbx::DocumentPtr doc = sfbx::MakeDocument("test_base_ascii.fbx");
        if (doc->mergeAnimations("test_anim_ascii.fbx")) {
            if (auto take = doc->getCurrentTake()) {
                take->applyAnimation(11.0f);
            }
        }
    }
}

static void PrintObject(sfbx::Object* obj, int depth = 0)
{
    // indent
    for (int i = 0; i < depth; ++i)
        testPrint("  ");

    testPrint("\"%s\" [0x%llx] (%s : %s)\n",
        obj->getName().data(),
        obj->getID(),
        sfbx::GetObjectClassName(obj->getClass()).data(),
        sfbx::GetObjectSubClassName(obj->getSubClass()).data());

    //// for debug
    //if (auto skin = as<sfbx::Skin>(obj)) {
    //    auto weights_v = skin->getJointWeights();
    //    auto weights_4 = skin->createFixedJointWeights(4);
    //    auto matrices = skin->getJointMatrices();
    //    testPrint("");
    //}
    //else if (auto anim = as<sfbx::AnimationLayer>(obj)) {
    //    for (auto n : anim->getAnimationCurveNodes()) {
    //        if (n->getAnimationKind() == sfbx::AnimationKind::Position) {
    //            float start = n->getStartTime();
    //            float stop = n->getStopTime();
    //            for (float t = start; t <= stop; t += 0.033334f) {
    //                auto v = n->evaluate3(t);
    //                testPrint("");
    //            }
    //        }
    //    }
    //}

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

    sfbx::DocumentPtr doc = sfbx::MakeDocument(path);
    if (doc->valid()) {
        if (!path2.empty())
            doc->mergeAnimations(path2);
        if (!path3.empty())
            doc->mergeAnimations(path3);

        testPrint("Objects:\n");
        for (auto obj : doc->getRootObjects())
            PrintObject(obj);

        doc->writeBinary("out_b.fbx");
        doc->writeAscii("out_a.fbx");
    }

}

testCase(fbxAnimationCurve)
{
    sfbx::DocumentPtr doc = sfbx::MakeDocument();
    auto curve = doc->createObject<sfbx::AnimationCurve>("TestCurve");

    float times[]{ 0.0f, 1.0f, 2.0f };
    float values[]{ 0.0f, 100.0f, 400.0f };
    curve->setTimes(times);
    curve->setRawValues(values);

    for (float t = -0.5f; t < 2.5f; t += 0.1f) {
        printf("time: %f, value: %f\n", t, curve->evaluate(t));
    }
}
