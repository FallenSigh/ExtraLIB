#include <algorithm>
#include <cmath>

#include "details/integer.h"

// N bit signed integer
namespace exlib {
    template<std::size_t N, class Word = unsigned char, class Allocator = void>
    using nints = integer<N, Word, Allocator, true>;
}