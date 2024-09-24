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
    unints<128> b("123456789");
    std::cout << a + b << "\n";
    std::cout << a - b << "\n";
    std::cout << a * b << "\n";
    std::cout << a / b << "\n\n";
}
    
{
    // n-digits floating point
    // Not completed yet
    nfloats<64> f = 33.1415926;
    nfloats<64> g = 1.1616;
    std::cout << std::setprecision(10) << f + g << "\n";
    std::cout << std::setprecision(10) << f - g << "\n";
    std::cout << std::setprecision(10) << f * g << "\n";
    std::cout << std::setprecision(10) << f / g << "\n\n";
}

{
    // complex
    // Not completed yet
    complex<double> c(1, 10);
    complex<double> d(1, -10);
    std::cout << c + d << "\n";
    std::cout << c - d << "\n";
    std::cout << c * d << "\n";
    std::cout << c / d << "\n\n";
}

{
    // fraction
    // Not completed yet
    fraction<int> a(1, 3);
    fraction<int> b(1, 4);
    std::cout << a + b << "\n";
    std::cout << a - b << "\n";
    std::cout << a * b << "\n";
    std::cout << a / b << "\n\n";
}

{
    // numpy-like ndarray
    auto a = ones<shape<100>>();
    auto b = a.reshape<shape<5, 5, 4>>();
    ndarray<shape<2, 2>> c = {{1.14, 5.14}, {2.88, 1.46}};
    auto d = ones<shape<1, 5, 1>>();

    std::cout << a.slice<slice<1, 4>>() << "\n";
    std::cout << (b + d) << "\n";
    std::cout << (b * d) << "\n";
    std::cout << b.slice<slice<1, 2>, slice<3, 4>, void>() << "\n";
    std::cout << b.slice<void, slice<1, 2>>() << "\n";

    std::cout << sum(c) << " " << mean(c) << "\n";
    std::cout << exp(a.slice<slice<1, 4>>()) << " " << log(a.slice<slice<1, 4>>()) << "\n";
}
    return 0;
}