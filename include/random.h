#pragma once
#include "ndarray.h"
#include <cstdint>
#include <optional>
#include <random>

namespace exlib {
    namespace random {
        template <class Shape, class DType = double>
        auto randn(std::optional<uint32_t> _seed = std::nullopt) noexcept {
            std::mt19937 rand;
            if (_seed) rand.seed(_seed.value());
            auto res = ndarray<Shape, DType>();
            res.assign([&rand]() -> DType {
                return std::normal_distribution<DType>{}(rand);
            });
            return res;
        }        
    }
}