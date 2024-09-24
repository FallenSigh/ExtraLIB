# ExtraLIB
useless library

### content

- n-bit signed/unsigned integer
- float with n-bit mantissa
- fraction
- complex
- numpy-like ndarray

### examples
```cpp
#include <complex.h>
#include <iostream>

#include "integer.h"
#include "nfloats.h"
#include "complex.h"
#include "fraction.h"
#include "ndarray.h"

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
}
    return 0;
}
```

### reference
[mallochook(github.com/archibate/mallocvis)](https://github.com/archibate/mallocvis)

[minilog(github.com/archibate/minilog)](https://github.com/archibate/minilog)