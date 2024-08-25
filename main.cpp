#include <iostream>

#include "nbits.h"

int main(void) {
    zstl::nbits_int<8> a = 5;
    zstl::nbits_int<5> b = 3;
    zstl::nbits_int<8> c = 10;
    zstl::nbits_int<8> d = 8;
    int x = 1;
    // auto e = zstl::nbits_int<8>(5);
    auto e = 5;
    a <<= 2;
    a >>= 2;
    b += a;
    a += b;
    a -= b;
    a += zstl::nbits_int<8>(1);
    b += zstl::nbits_int<5>(1);
    a -= 3;
    a *= 4;
    std::cout << a << " " << a.dec() << "\n";
    std::cout << b << " " << b.dec() << "\n";
    std::cout << c << " " << c.dec() << "\n";
    std::cout << d << " " << d.dec() << "\n";

    std::cout << "a + b: " << (a + b).dec() << " b + a: " << (b + a).dec() << "\n";
    std::cout << "a - b: " << (a - b).dec() << " b - a: " << (b - a).dec() << "\n";
    std::cout << "a * b: " << (a * b).dec() << " b * a: " << (b * a).dec() << "\n";
    std::cout << "a / b: " << (a / b).dec() << " b / a: " << (b / a).dec() << "\n";
    std::cout << "a % b: " << (a % b).dec() << " b % a: " << (b % a).dec() << "\n";
    std::cout << "a + e: " << (a + e).dec() << " e + a: " << (e + a).dec() << "\n";
    std::cout << "a - e: " << (a - e).dec() << " e - a: " << (e - a).dec() << "\n";
    std::cout << "a * e: " << (a * e).dec() << " e * a: " << (e * a).dec() << "\n";
    std::cout << "a / e: " << (a / e).dec() << " e / a: " << (e / a).dec() << "\n";
    std::cout << "a % e: " << (a % e).dec() << " e % a: " << (e % a).dec() << "\n";
    std::cout << "a & b: " << (a & b).dec() << " b & a: " << (b & a).dec() << "\n";
    std::cout << "a | b: " << (a | b).dec() << " b | a: " << (b | a).dec() << "\n";
    std::cout << "a ^ b: " << (a ^ b).dec() << " b ^ a: " << (b ^ a).dec() << "\n";
    std::cout << "a > b: " << (a > b) << " b > a: " << (b > a) << "\n";
    std::cout << "a < b: " << (a < b) << " b < a: " << (b < a) << "\n";
    std::cout << "a = b: " << (a == b) << " b = a: " << (b == a) << "\n";
    std::cout << "a != b: " << (a != b) << " b != a: " << (b != a) << "\n";
    std::cout << "a >= b: " << (a >= b) << " b >= a: " << (b >= a) << "\n";
    std::cout << "a <= b: " << (a <= b) << " b <= a: " << (b <= a) << "\n";
    std::cout << "a << " << x << " : " << (a << x) << "\n";
    std::cout << "a >> " << x << " : " << (a >> x) << "\n";
    return 0;
}