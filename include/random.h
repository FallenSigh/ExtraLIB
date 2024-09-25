#pragma once
#include "ndarray.h"
#include <random>

namespace exlib {
    namespace random {
        template <class Shape, class DType = double>
        auto randn() noexcept {
            std::mt19937 rand;
            auto res = ndarray<Shape, DType>();
            res.assign([&rand]() -> DType {
                return std::normal_distribution<DType>{}(rand);
            });
            return res;
        }        
    }
}