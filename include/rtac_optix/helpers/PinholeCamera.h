#ifndef _DEF_RTAC_OPTIX_HELPERS_PINHOLE_CAMERA_H_
#define _DEF_RTAC_OPTIX_HELPERS_PINHOLE_CAMERA_H_

#include <rtac_base/cuda/utils.h>

#include <rtac_optix/helpers/maths.h>

namespace rtac { namespace optix { namespace helpers {

struct PinholeCamera
{
    float3 position_; // ray origin
    float3 u_; // alongside top-edge, left to right
    float3 v_; // alongside left-edge, top to bottom
    float3 w_; // towards back, length controls the vertical field of view

    RTAC_HOSTDEVICE 
    static PinholeCamera Create(const float3& target   = {0.0,1.0f,0.0},
                                const float3& position = {0.0f, 0.0f, 0.0f},
                                float fovy = 90.0,
                                const float3& up = {0.0f,0.0f,1.0f})
    {
        PinholeCamera res;
        res.w_ = make_float3(0.0f,0.0f,1.0f);
        res.set_fovy(fovy);
        res.look_at(target, position, up);

        return res;
    }

    RTAC_HOSTDEVICE 
    void compute_ray(const uint3& idx, const uint3& dim, float3& origin, float3& direction)
    {
        origin = position_;
        // compensating aspect ratio
        float px = dim.x * (2.0f * idx.x / (dim.x - 1) - 1.0f) / dim.y; 
        float py =          2.0f * idx.y / (dim.y - 1) - 1.0f;
        direction = normalize(px*u_ + py*v_ + w_);
    }

    RTAC_HOSTDEVICE 
    void look_at(const float3& target, const float3& position,
                 const float3& up = {0.0f,0.0f,1.0f})
    {   // w_ towards target, orthogonal to u_ and v_.
        // u_ toward the right side, horizontal relative to up.
        // v_ towards bottom
        position_ = position;

        float fovy = sqrtf(dot(w_,w_)); // this is to keep the same field of view.
        w_ = normalize(target - position); 
        u_ = normalize(cross(w_, up)); // This normalization is necessary.
        v_ = normalize(cross(w_, u_)); // normalization not necessary in theory.

        w_ *= fovy; // 
    }

    RTAC_HOSTDEVICE 
    void set_fovy(float fovy)
    {
        // This function assumes normalized u_ and v_ vectors_ and a non-zero
        // w_, fovy in degrees.
        w_ /= tanf(0.5*fovy*M_PI / 180.0f) * sqrtf(dot(w_, w_));
    }
};



}; //namespace helpers
}; //namespace optix
}; //namespace rtac

#ifndef __NVCC__
#include <iostream>
inline std::ostream& operator<<(std::ostream& os, const rtac::optix::helpers::PinholeCamera& cam)
{
    os << "PinholeCamera :"
       << "\n- center : " << cam.position_.x <<" "<< cam.position_.y <<" "<< cam.position_.z
       << "\n- u      : " << cam.u_.x <<" "<< cam.u_.y <<" "<< cam.u_.z
       << "\n- v      : " << cam.v_.x <<" "<< cam.v_.y <<" "<< cam.v_.z
       << "\n- w      : " << cam.w_.x <<" "<< cam.w_.y <<" "<< cam.w_.z;
    return os;
}
#endif

#endif //_DEF_RTAC_OPTIX_HELPERS_PINHOLE_CAMERA_H_
