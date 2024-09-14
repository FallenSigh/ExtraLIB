#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>

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
    
    exlib::unints<100> a = 23213213200ull;

    std::cout << a.hex() << "\n";
    std::cout << a.bin() << "\n";
    std::cout << a.str() << "\n";
    return 0;
}
