#include <complex.h>
#include <iostream>

#include "include/integer.h"
#include "include/nfloats.h"
#include "include/complex.h"
#include "include/fraction.h"
#include "include/ndarray.h"

int main(void) {
    using namespace exlib;

{    
    // n-bit signed/unsigned integer
    nints<256> a("2147483647");
    unints<128> b("-1");
    std::cout << b << "\n";
    std::cout << b + a << "\n";
}
    
    
{
    // n-digits floating point
    // Not completed yet
    nfloats<1024> f = 3.1415926;
    std::cout << std::setprecision(10) << f << "\n";
}

{
    // complex
    // Not completed yet
    complex<int> c(1, 10);
    complex<int> d(1, -10);
    std::cout << c * d << "\n";
}

{
    // fraction
    fraction<int> a(1, 3);
    fraction<int> b(1, 4);
    std::cout << a + b << "\n";
}

{
    // numpy-like ndarray
    auto a = ones<shape<100>>();
    auto b = a.reshape<shape<5, 5, 4>>();
    ndarray<shape<2, 2>> c = {{1.14, 5.14}, {2.88, 1.46}};
    
    std::cout << a.slice<slice<1, 4>>() << "\n";
    std::cout << b.slice<slice<1, 2>, slice<3, 4>, void>() << "\n";
    std::cout << b.slice<void, slice<1, 2>>() << "\n";

    std::cout << sum(c) << " " << mean(c) << "\n";
    std::cout << exp(a.slice<slice<1, 4>>()) << " " << log(a.slice<slice<1, 4>>()) << "\n";
}
    return 0;
}