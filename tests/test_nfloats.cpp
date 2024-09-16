#include <cmath>
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
        exlib::nfloats<54> a(x);
        exlib::nfloats<54> b(y);

        a += b;
        x += y;
        if ((a - b).abs() < 1e-9) {
            exlib::log_fatal("fatal, {}, {}", a.to_double(), x);
            exlib::log_fatal("{}", a.bin());
            exlib::log_fatal("{}", to_bin(x, " "));
            break;
        } else {
            exlib::log_info("pass {}: {}, {}", i, a.str(), x);
        }
    }
    return 0;
}
