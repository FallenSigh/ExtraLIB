#include <cmath>
#include <iostream>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <random>

#include "log.h"
#include "nfloats.h"

int main() {
    exlib::set_log_level(exlib::log_level::debug);
    auto to_bin = [](double x, const char *split = "\0") -> std::string {
        std::uint64_t num_as_int = *reinterpret_cast<uint64_t*>(&x);
        auto bin = std::bitset<64>(num_as_int);
        std::string res = bin.to_string();
        res.insert(1, split);
        res.insert(1 + 12, split);
        return res;
    };

    std::mt19937 rand;
    rand.seed(123);
    std::uniform_int_distribution range(101, 20001);
    for (std::size_t i = 0; i <= 10000; i++) {
        std::size_t r1 = range(rand);
        std::size_t r2 = range(rand);
        double x = r1 / 100.0;
        double y = r2 / 100.0;

        exlib::log_info("testing {}: {}, {}", i, x, y);
        exlib::nfloats<52> a(x);
        exlib::nfloats<52> b(y);

        a += b;
        x += y;
        if (a != x) {
            exlib::log_fatal("fatal, {}, {}", a.to<double>(), x);
            exlib::log_fatal("{}", a.bin(' '));
            exlib::log_fatal("{}", to_bin(x, " "));
            break;
        }
    }
    return 0;
}
