#ifndef _DEF_OPTIX_HELPERS_DISPLAY_POINTCLOUD_RENDERER_H_
#define _DEF_OPTIX_HELPERS_DISPLAY_POINTCLOUD_RENDERER_H_

#include <iostream>
#include <array>
#include <algorithm>

#include <rtac_base/types/PointCloud.h>

#include <optix_helpers/Handle.h>
#include <optix_helpers/Source.h>

#include <optix_helpers/display/RenderBufferGL.h>
#include <optix_helpers/display/Renderer.h>
#include <optix_helpers/display/View3D.h>

namespace optix_helpers { namespace display {


class PointCloudRendererObj : public RendererObj
{
    public:

    using Mat4    = View3DObj::Mat4;
    using Shape   = View3DObj::Shape;
    using Pose    = View3DObj::Pose;
    using Color   = std::array<float,3>;

    static const Source vertexShader;
    static const Source fragmentShader;

    protected:
    
    size_t numPoints_;
    GLuint points_;
    Pose   pose_;
    Color  color_;
    
    void allocate_points(size_t numPoints);
    void delete_points();

    public:
    
    PointCloudRendererObj(const View3D& view,
                          const Color& color = {0.7,0.7,1.0});
    ~PointCloudRendererObj();
    
    template <typename PointCloudT>
    void set_points(const rtac::types::PointCloud<PointCloudT>& pc);
    void set_points(size_t numPoints, const float* data);
    void set_points(const RenderBufferGL& buffer);
    void set_pose(const Pose& pose);
    void set_color(const Color& color);

    virtual void draw();
};
using PointCloudRenderer = Handle<PointCloudRendererObj>;

// implementation
template <typename PointCloudT>
void PointCloudRendererObj::set_points(const rtac::types::PointCloud<PointCloudT>& pc)
{
    this->allocate_points(pc.size());
    glBindBuffer(GL_ARRAY_BUFFER, points_);
    auto deviceData = static_cast<float*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
    int i = 0;
    for(auto& point : pc) {
        deviceData[i]     = point.x;
        deviceData[i + 1] = point.y;
        deviceData[i + 2] = point.z;
        i += 3;
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    numPoints_ = pc.size();
}

}; //namespace display
}; //namespace optix_helpers

#endif //_DEF_OPTIX_HELPERS_DISPLAY_POINTCLOUD_RENDERER_H_
