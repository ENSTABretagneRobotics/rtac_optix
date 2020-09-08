#include <iostream>
using namespace std;

#include <optix_helpers/Context.h>
#include <optix_helpers/RayType.h>
using namespace optix_helpers;

#include "cusamples.h"

int main()
{
    Context context;

    RayType rayType0 = context->create_raytype(Source(cusample::coloredRay, "colored_ray.h"));
    cout << rayType0 << endl;

    // Won't compile. Only Context can create RayTypes.
    //RayType rayType1(10, Source(cusample::coloredRay, "colored_ray.h"));

    return 0;
}

