#ifndef _DEF_OPTIX_HELPERS_SAMPLES_TEST_SCENES_H_
#define _DEF_OPTIX_HELPERS_SAMPLES_TEST_SCENES_H_

#include <iostream>

#include <rtac_base/types/Pose.h>

#include <optix_helpers/Context.h>
#include <optix_helpers/RayType.h>
#include <optix_helpers/RayGenerator.h>
#include <optix_helpers/PinHoleView.h>

#include <optix_helpers/samples/raytypes.h>
#include <optix_helpers/samples/materials.h>
#include <optix_helpers/samples/geometries.h>
#include <optix_helpers/samples/items.h>
#include <optix_helpers/samples/utils.h>
#include <optix_helpers/samples/raygenerators.h>

namespace optix_helpers { namespace samples { namespace scenes {

template <typename RayGeneratorType>
class SceneBase
{
    protected:

    Context          context_;
    RayGeneratorType raygenerator_;

    public:

    SceneBase() {};
    ViewGeometry view() { return raygenerator_->view(); }
    void launch()
    {
        auto shape = raygenerator_->render_shape();
        (*context_)->launch(0, shape.width, shape.height);
    }
};

class Scene0 : public SceneBase<raygenerators::RgbCamera>
{
    public:

    using Pose = rtac::types::Pose<float>;
    using Quaternion = rtac::types::Quaternion<float>;

    static const Source raygenSource;

    public:

    Scene0(size_t width, size_t height);

};


}; //namespace scenes
}; //namespace samples
}; //namespace optix_helpers

#endif //_DEF_OPTIX_HELPERS_SAMPLES_TEST_SCENES_H_
