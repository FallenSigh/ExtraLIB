#include <random>
#include <cassert>
#include <unordered_map>
#include <functional>

#include "log.h"
#include "nints.h"

int main(void) {
    std::mt19937 rand;
    std::uniform_int_distribution ops(0, 13);
    std::uniform_int_distribution num(-10, 30);
    rand.seed(19519);

    constexpr std::size_t n = 1000;

    zstl::nints<32> a = 0;
    zstl::nints<16> b = 0;
    int c = 0;
    short d = 0;

    std::size_t i;

    auto log_and_check = [&](const char* operation, auto& lhs, auto& rhs, auto& result_lhs, auto& result_rhs, auto action) {
        zstl::log_info("testing {} at {}, {} {} {} {}", operation, i, lhs.dec(), rhs.dec(), result_lhs, result_rhs);

        action();

        if (lhs != result_lhs) {
            zstl::log_error("fatal {} at {}, {} {} {} {}", operation, i, lhs.dec(), rhs.dec(), result_lhs, result_rhs);
            return false;
        }
        return true;
    };

    std::unordered_map<int, std::function<bool()>> operations = {
        {0, [&]() { return log_and_check("<<=", a, b, c, d, [&]() { a <<= b; c <<= d; }); }},
        {1, [&]() { return log_and_check(">>=", a, b, c, d, [&]() { a >>= b; c >>= d; }); }},
        {2, [&]() { return log_and_check("+=", a, b, c, d, [&]() { a += b; c += d; }); }},
        {3, [&]() { return log_and_check("-=", a, b, c, d, [&]() { a -= b; c -= d; }); }},
        {4, [&]() { return log_and_check("*=", a, b, c, d, [&]() { a *= b; c *= d; }); }},
        {5, [&]() { return log_and_check("/=", a, b, c, d, [&]() { if (b == 0) b = 1; if (d == 0) d = 1; a /= b; c /= d; }); }},
        {6, [&]() { return log_and_check("%=", a, b, c, d, [&]() { if (b == 0) b = 1; if (d == 0) d = 1; a %= b; c %= d; }); }},
        {7, [&]() { return log_and_check("+", a, b, c, d, [&]() { if (a + b != c + d) return false; return true; }); }},
        {8, [&]() { return log_and_check("-", a, b, c, d, [&]() { if (a - b != c - d) return false; return true; }); }},
        {9, [&]() { return log_and_check("*", a, b, c, d, [&]() { if (a * b != c * d) return false; return true; }); }},
        {10, [&]() { return log_and_check("/", a, b, c, d, [&]() { if (b == 0) b = 1, d = 1; if (a / b != c / d) return false; return true; }); }},
        {11, [&]() { return log_and_check("&", a, b, c, d, [&]() { if ((a & b) != (c & d)) return false; return true; }); }},
        {12, [&]() { return log_and_check("|", a, b, c, d, [&]() { if ((a | b) != (c | d)) return false; return true; }); }},
        {13, [&]() { return log_and_check("^", a, b, c, d, [&]() { if ((a ^ b) != (c ^ d)) return false; return true; }); }},
    };

    for (i = 0; i < n; i++) {
        auto op = ops(rand);
        a = c = num(rand);
        b = d = num(rand);

        if (op == 0 || op == 1) {
            b = b.abs();
            d = std::abs(d);
        }

        if (operations.find(op) != operations.end() && !operations[op]()) {
            break;
        }
    }

    if (i != n) return -1;

    return 0;
}
