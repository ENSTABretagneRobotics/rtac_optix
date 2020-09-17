#include <optix_helpers/samples/scenes.h>

namespace optix_helpers { namespace samples { namespace scenes {

const Source Scene0::raygenSource = Source(R"(
#include <optix.h>

using namespace optix;

#include <rays/RGB.h>
#include <view/pinhole.h>

rtDeclareVariable(uint2, launchIndex, rtLaunchIndex,);
rtDeclareVariable(rtObject, topObject,,);

//rtBuffer<float, 2> renderBuffer;
rtBuffer<float3, 2> renderBuffer;

RT_PROGRAM void pinhole_scene0()
{
    raytypes::RGB payload;
    payload.color = make_float3(0.0f,0.0f,0.0f);

    Ray ray = pinhole_ray(launchIndex, 0);

    rtTrace(topObject, ray, payload);
    //renderBuffer[launchIndex] = payload.color.x;
    renderBuffer[launchIndex] = payload.color;
}
)", "pinhole_scene0");

Scene0::Scene0(size_t width, size_t height,
               unsigned int glboId)
{
    using namespace std;
    size_t W = width;
    size_t H = height;

    (*context_)->setMaxTraceDepth(10);
    (*context_)->setMaxCallableProgramDepth(10);
    cout << "Default stack size : " << (*context_)->getStackSize() << endl;
    (*context_)->setStackSize(8096);
    cout << "Stack size : " << (*context_)->getStackSize() << endl;

    raytypes::RGB rayType0(context_);
    //cout << rayType0 << endl;

    Material white    = materials::white(context_, rayType0);
    Material mirror   = materials::perfect_mirror(context_, rayType0);
    Material glass    = materials::perfect_refraction(context_, rayType0, 1.1);
    Material lambert  = materials::lambert(context_, rayType0, {10.0,10.0,10.0});
    TexturedMaterial checkerboard = materials::checkerboard(context_, rayType0, 
                                                            {255,255,255},
                                                            {0,0,0}, 10, 10);

    SceneItem square0 = items::square(context_,
        {materials::checkerboard(context_, rayType0, {0,255,0}, {0,0,255}, 10, 10)},
        10);

    //SceneItem cube0 = items::cube(context_,
    //    {materials::checkerboard(context_, rayType0, {255,255,255}, {0,0,0}, 4, 4)});
    SceneItem cube0 = items::cube(context_, {mirror});
    //SceneItem cube0 = items::cube(context_, {lambert});
    cube0->set_pose(Pose({4,0,1}));

    SceneItem cube1 = items::cube(context_,
        {materials::checkerboard(context_, rayType0, {255,255,255}, {0,0,0}, 4, 4)});
    //SceneItem cube0 = items::cube(context_, {mirror});
    cube1->set_pose(Pose({-2.5,4,2}));

    //SceneItem sphere0 = items::sphere(context_, {checkerboard});
    //SceneItem sphere0 = items::sphere(context_, {white});
    //SceneItem sphere0 = items::sphere(context_, {mirror});
    SceneItem sphere0 = items::sphere(context_, {glass}, 3.0);
    //SceneItem sphere0 = items::sphere(context_, {lambert});
    //SceneItem sphere0 = items::cube(context_, {mirror});
    //SceneItem sphere0 = items::tube(context_, {mirror});
    //SceneItem sphere0 = items::tube(context_, {glass});
    //SceneItem sphere0 = items::tube(context_, {lambert});
    //SceneItem sphere0 = items::tube(context_, {checkerboard});
    //SceneItem sphere0 = items::tube(context_, {white});
    //SceneItem sphere0 = items::square(context_, {lambert});
    //SceneItem sphere0 = items::square(context_, {mirror});
    //SceneItem sphere0 = items::parabola(context_, {lambert}, .5, 0.0, 2.0);
    //SceneItem sphere0 = items::parabola(context_, {mirror}, .5, 0.0, 2.0);
    sphere0->set_pose(Pose({0,0,1}));
    //sphere0->set_pose(Pose({0,0.5,1.5}));
    //sphere0->set_pose(Pose({0,0,0}, Quaternion({0.707,-0.707,0,0})));
    
    SceneItem sphere1 = items::sphere(context_, {mirror});
    sphere1->set_pose(Pose({0,0,1}));

    Model lense = context_->create_model();
    //lense->set_geometry(geometries::parabola(context_, 0.1, -0.1, 0.1));
    lense->set_geometry(geometries::parabola(context_, 0.1, -0.2, 0.2));
    //lense->add_material(mirror);
    //lense->add_material(glass);
    //auto lenseGlass = materials::perfect_refraction(context_, rayType0, 2.4);
    auto lenseGlass = materials::perfect_refraction(context_, rayType0, 1.7);
    lense->add_material(lenseGlass);

    SceneItem lense0 = context_->create_scene_item(lense);
    lense0->set_pose(Pose({0,0,2}));
    SceneItem lense1 = context_->create_scene_item(lense);
    lense1->set_pose(Pose({0,0,2}, Quaternion({0.0,1.0,0.0,0.0})));

    Quaternion q({1.0,1.0,0.0,0.0});
    q.normalize();
    SceneItem mirror0 = items::square(context_, {mirror});
    mirror0->set_pose(Pose({0,0,0}, q));
    SceneItem mirror1 = items::square(context_, {mirror});
    mirror1->set_pose(Pose({0,0,2}, q));

    optix::Group topObject = (*context_)->createGroup();
    topObject->setAcceleration((*context_)->createAcceleration("Trbvh"));
    topObject->addChild(square0->node());
    topObject->addChild(cube0->node());
    topObject->addChild(cube1->node());
    topObject->addChild(sphere0->node());
    topObject->addChild(sphere1->node());
    //topObject->addChild(mirror0->node());
    //topObject->addChild(mirror1->node());
    //topObject->addChild(lense0->node());
    //topObject->addChild(lense1->node());

    (*mirror->get_closest_hit_program(rayType0))["topObject"]->set(topObject);
    (*glass->get_closest_hit_program(rayType0))["topObject"]->set(topObject);
    (*lenseGlass->get_closest_hit_program(rayType0))["topObject"]->set(topObject);

    Program raygenProgram = context_->create_program(raygenSource,
                                                     {rayType0->definition(),
                                                      PinHoleView::rayGeometryDefinition});
    
    Buffer renderBuffer;
    if(glboId > 0) {
        renderBuffer = context_->create_gl_buffer(RT_BUFFER_OUTPUT,
                                                  RT_FORMAT_FLOAT3,
                                                  glboId,
                                                  "renderBuffer");
    }
    else {
        renderBuffer = context_->create_buffer(RT_BUFFER_OUTPUT,
                                               RT_FORMAT_FLOAT3,
                                               "renderBuffer");
    }
    PinHoleView pinhole(renderBuffer, raygenProgram);
    pinhole->set_size(W,H);
    pinhole->set_range(1.0e-2f, RT_DEFAULT_MAX);
    pinhole->look_at({0.0,0.0,0.0},{ 2.0, 5.0, 4.0});
    //pinhole->look_at({0.0,0.0,0.0},{ 3.0,-3.0, 4.0});
    //pinhole->look_at({0.0,0.0,0.0},{ -5.0,-2.0, 4.0});
    //pinhole->look_at({0.0,0.0,0.0},{-5.0,-4.0,-3.0});
    //pinhole->look_at({0.0,0.0,0.0},{ 5.0, 0.0, 3.0});
    //pinhole->look_at({0.0,1.0,0.0});
    //pinhole->look_at({0.0,0.0,0.0},{ 2.0, 5.0, -4.0});
    //pinhole->look_at({0.0,0.0,0.0},{ -1.0, -1.0, 3.5});

    view_ = pinhole;

    (*context_)->setRayGenerationProgram(0, *raygenProgram);
    (*context_)->setMissProgram(0, *raytypes::RGB::rgb_miss_program(context_, {0.8,0.8,0.8}));

    (*raygenProgram)["topObject"]->set(topObject);
}

PinHoleView Scene0::view()
{
    return view_;
}

void Scene0::launch()
{
    size_t W, H;
    (*view_->render_buffer())->getSize(W,H);
    (*context_)->launch(0,W,H);
}

}; //namespace scenes
}; //namespace samples
}; //namespace optix_helpers


