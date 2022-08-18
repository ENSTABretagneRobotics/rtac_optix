#include <iostream>
using namespace std;

#include <compile_test/ptx_files.h>

#include "compile_test.h"

template <typename T>
void print(const std::vector<T>& v)
{
    for(auto value : v) {
        cout << " " << value;
    }
    cout << endl;
}

int main()
{
    cout << "Hello there !" << endl;

    std::vector<float> in(32);
    for(int i = 0; i < in.size(); i++) { 
        in[i] = i;
    }
    std::vector<float> out(32, 0.0);

    print(in);
    print(out);

    copy(in,out);

    print(in);
    print(out);

    auto ptxDict = compile_test::get_ptx_files();
    for(auto& f : ptxDict) {
        cout << "PTX file " << f.first << endl << f.second << "\n\n\n\n\n\n";
    }
    return 0;
}
