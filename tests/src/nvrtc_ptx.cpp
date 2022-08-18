#include <iostream>
using namespace std;

#include <rtac_optix/Source.h>
#include <rtac_optix/Nvrtc.h>
using namespace rtac::optix;

int main()
{
    auto source0 = Source::New(R"(
    __device__
    void copy(const float& in, float& out) {
        out = in;
    }
    )", "copy.cu");
    cout << source0 << endl;

    Nvrtc nvrtc;
    cout << nvrtc << endl;

    auto ptx = nvrtc.compile(source0);
    cout << "Compiled ptx :\n" << ptx << endl;

    return 0;
}
