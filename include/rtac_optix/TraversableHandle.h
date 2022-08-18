#ifndef _DEF_RTAC_OPTIX_TRAVERSABLE_HANDLE_H_
#define _DEF_RTAC_OPTIX_TRAVERSABLE_HANDLE_H_

#include <iostream>

#include <optix.h>
// careful : because of OptiX function table optix_stubs.h must be included to
// ensure proper linking.
#include <optix_stubs.h>

#include <rtac_optix/Handle.h>

namespace rtac { namespace optix {

/**
 * **TraversableHandle IS NOT USED ANYMORE BY rtac_optix AND IS TO BE DELETED.**
 *
 * An abstract class which represents an element of the object tree.
 *
 * In OptiX the ray must interact with objects in the rendering scene.  These
 * objects can be either physical objects with a geometry and materials
 * (ObjectInstance), or compound objects which contains other objects
 * (GroupInstance). These objects form an object tree with a single root,
 * ObjectInstance being the leaves of the tree and GroupInstance being the
 * nodes. The root of the object tree is usually given as parameter to the
 * [optixTrace](https://raytracing-docs.nvidia.com/optix7/api/html/group__optix__device__api.html#gab2bdbb55a09ffbaa1643f1d7e2fcfcc9)
 * function as an
 * [OptixTraversableHandle](https://raytracing-docs.nvidia.com/optix7/api/html/group__optix__types.html#gaacf20eb67c33c2c1849adc058d43cff7).
 *
 * Both ObjectInstance and GroupInstance are high-level easier to use wrappers
 * around more low-level OptiX objects. The TraversableHandle, rtac_optix
 * counterpart of
 * [OptixTraversableHandle](https://raytracing-docs.nvidia.com/optix7/api/html/group__optix__types.html#gaacf20eb67c33c2c1849adc058d43cff7),
 * is the lowest common denominator between the ObjectInstance and the
 * GroupInstance classes and represent nothing more than an element of the
 * object tree. It is implicitly castable to an
 * [OptixTraversableHandle](https://raytracing-docs.nvidia.com/optix7/api/html/group__optix__types.html#gaacf20eb67c33c2c1849adc058d43cff7)
 * but specializations of TraversableHandles must implement the conversion.
 */
struct TraversableHandle
{
    // ABSTRACT class

    // This is an abstract class to represent the base type for nodes in the
    // rendering tree. The main purpose of this type is to be able to infer the
    // sbt records positions by exploring the world tree.

    public:

    using Ptr      = Handle<TraversableHandle>;
    using ConstPtr = Handle<const TraversableHandle>;
   
    /**
     * Implicit cast to
     * [OptixTraversableHandle](https://raytracing-docs.nvidia.com/optix7/api/html/group__optix__types.html#gaacf20eb67c33c2c1849adc058d43cff7).
     * To be reimplemented in subclasses.
     *
     * @return a valid
     * [OptixTraversableHandle](https://raytracing-docs.nvidia.com/optix7/api/html/group__optix__types.html#gaacf20eb67c33c2c1849adc058d43cff7).
     */
    virtual operator OptixTraversableHandle() = 0;

    // This represent the width in sbt offsets that the 
    virtual unsigned int sbt_width() const = 0;
};

}; //namespace optix
}; //namespace rtac

#endif //_DEF_RTAC_OPTIX_TRAVERSABLE_HANDLE_H_
