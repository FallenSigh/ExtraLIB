#include <algorithm>
#include <cmath>

#include "details/integer.h"

// N bit unsigned integer
namespace exlib {
    template<std::size_t N, class Word = unsigned char, class Allocator = void>
    using unints = integer<N, Word, Allocator, false>;
}