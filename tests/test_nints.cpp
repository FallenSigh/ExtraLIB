#include <random>
#include <cassert>
#include <unordered_map>
#include <functional>

#include "log.h"
#include "integer.h"

int main(void) {
    exlib::set_log_level(exlib::log_level::debug);
    std::mt19937 rand;
    std::uniform_int_distribution num(-30, 31);
    rand.seed(19519);
    int m = 0;
    constexpr int n = 100000;

    exlib::nints<64, uint8_t, std::allocator<uint8_t>> a = 0;
    exlib::nints<64, uint32_t> b = 0;
    long long c = 0;
    long long d = 0;

    std::size_t i;

    auto log_and_check = [&](const char* operation, auto& a, auto& b, auto& c, auto& d, auto action) {
        exlib::log_info("testing {} at {}, {}, {}, {}: ", operation, static_cast<std::int64_t>(a), static_cast<std::int64_t>(b), c, d);
        
        if (action() == false) {
            exlib::log_fatal("fatal {} at {}, {}, {}, {}", operation, static_cast<std::int64_t>(a), static_cast<std::int64_t>(b), c, d);
            return false;
        }
        return true;
    };                


    std::unordered_map<int, std::function<bool()>> operations;

    operations[m++] = [&]() {
        return log_and_check("+=", a, b, c, d, [&]() { a += b; c += d; return a == c;});
    };

    operations[m++] = [&]() {
        return log_and_check("-=", a, b, c, d, [&]() { a -= b; c -= d; return a == c; });
    };

    operations[m++] = [&]() {
        return log_and_check("*=", a, b, c, d, [&]() { a *= b; c *= d; return a == c; });
    };

    operations[m++] = [&]() {
        return log_and_check("/=", a, b, c, d, [&]() { if (b == 0) b = 3ull, d = 3ull; a /= b; c /= d; return a == c; });
    };

    operations[m++] = [&]() {
        return log_and_check("%=", a, b, c, d, [&]() { if (b == 0) b = 3ull, d = 3ull;  a %= b; c %= d; return a == c; });
    };

    operations[m++] = [&]() {
        return log_and_check("&=", a, b, c, d, [&]() { a &= b; c &= d; return a == c; });
    };

    operations[m++] = [&]() {
        return log_and_check("|=", a, b, c, d, [&]() { a |= b; c |= d; return a == c; });
    };

    operations[m++] = [&]() {
        return log_and_check("^=", a, b, c, d, [&]() { a ^= b; c ^= d; return a == c; });
    };

    operations[m++] = [&]() {
        return log_and_check("+", a, b, c, d, [&]() { return a + b == c + d; });
    };

    operations[m++] = [&]() {
        return log_and_check("-", a, b, c, d, [&]() { return a - b == c - d; });
    };

    operations[m++] = [&]() {
        return log_and_check("*", a, b, c, d, [&]() { return a * b == c * d; });
    };

    operations[m++] = [&]() {
        return log_and_check("/", a, b, c, d, [&]() { if (b == 0) b = 3ull, d = 3ull; return a / b == c / d; });
    };

    operations[m++] = [&]() {
        return log_and_check("%", a, b, c, d, [&]() { if (b == 0) b = 3ull, d = 3ull; return a % b == c % d; });
    };

    operations[m++] = [&]() {
        return log_and_check("&", a, b, c, d, [&]() { return (a & b) == (c & d); });
    };

    operations[m++] = [&]() {
        return log_and_check("^", a, b, c, d, [&]() { return (a ^ b) == (c ^ d); });
    };

    operations[m++] = [&]() {
        return log_and_check("|", a, b, c, d, [&]() { return (a | b) == (c | d); });
    };

    operations[m++] = [&]() {
        return log_and_check("~", a, b, c, d, [&]() { return (~a) == (~c); });
    };

//     operations[m++] = [&]() {
//         return log_and_check(">>=", a, b, c, d, [&]() { return (a >>= b) == (c >>= d); });
//     };
// 
//     operations[m++] = [&]() {
//         return log_and_check("<<=", a, b, c, d, [&]() { return (a <<= b) == (c <<= d); });
//     };

     operations[m++] = [&]() {
        return log_and_check("<=", a, b, c, d, [&]() { return (a <= b) == (c <= d); });
    };

    operations[m++] = [&]() {
        return log_and_check(">=", a, b, c, d, [&]() { return (a >= b) == (c >= d); });
    };

    operations[m++] = [&]() {
        return log_and_check("<", a, b, c, d, [&]() { return (a < b) == (c < d); });
    };

    operations[m++] = [&]() {
        return log_and_check(">", a, b, c, d, [&]() { return (a > b) == (c > d); });
    };

    operations[m++] = [&]() {
        return log_and_check("==", a, b, c, d, [&]() { return (a == c) == (b == d); });
    };

    operations[m++] = [&]() {
        return log_and_check("!=", a, b, c, d, [&]() { return (a != c) == (b != d); });
    };

    std::uniform_int_distribution ops(0, m - 1);
    std::clock_t start = std::clock();
    for (i = 0; i < n; i++) {
        auto op = ops(rand);
        a = c = num(rand);
        b = d = num(rand);

        if (operations.find(op) != operations.end() && !operations[op]()) {
            break;
        }
    }
    std::clock_t end = std::clock();
    exlib::log_info("finished in {} ms, {}ms per task", (end - start) / 1000, (static_cast<double>((end - start)) / 1000) /  n);

    if (i != n) return -1;

    return 0;
}
