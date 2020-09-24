#ifndef _DEF_OPTIX_HELPERS_PINHOLE_VIEW_H_
#define _DEF_OPTIX_HELPERS_PINHOLE_VIEW_H_

#include <iostream>
#include <initializer_list>

#include <rtac_base/types/common.h>

#include <optix_helpers/Handle.h>
#include <optix_helpers/Source.h>
#include <optix_helpers/RayGenerator.h>

namespace optix_helpers { namespace samples { namespace raygenerators {

class PinHoleViewObj : public RayGeneratorObj
{
    public:

    static const Source rayGeometryDefinition;

    using Pose    = RayGeneratorObj::Pose;
    using Vector3 = RayGeneratorObj::Vector3;
    using Matrix3 = RayGeneratorObj::Matrix3;

    protected:

    float fovy_;

    virtual void update_geometry();

    public:

    PinHoleViewObj(const Context& context, 
                   const Buffer& renderBuffer,
                   const RayType& rayType,
                   float fovy,
                   const Source& raygenSource,
                   const Sources& additionalHeaders = {});

    virtual void set_pose(const Pose& pose);
    virtual void set_range(float zNear, float zFar);

    void set_fovy(float fovy);
};

class PinHoleView : public Handle<PinHoleViewObj>
{
    public:
    
    using Pose    = PinHoleViewObj::Pose;
    using Vector3 = PinHoleViewObj::Vector3;
    using Matrix3 = PinHoleViewObj::Matrix3;
    static const Source& rayGeometryDefinition;
    
    PinHoleView();
    PinHoleView(const Context& context, 
                const Buffer& renderBuffer,
                const RayType& rayType,
                float fovy,
                const Source& raygenSource,
                const Sources& additionalHeaders = {});

    operator RayGenerator();
};

}; //namespace raygenerators
}; //namespace samples
}; //namespace optix_helpers

#endif //_DEF_OPTIX_HELPERS_ORTHO_VIEW_H_
