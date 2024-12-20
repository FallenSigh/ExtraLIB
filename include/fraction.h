#pragma once
#include <numeric>
#include "nfloats.h"

namespace exlib {
    template <class Dtype>
    struct fraction {
        using dtype = Dtype;
        using self_type = fraction<dtype>;
        using reference = fraction<dtype>&;
        using const_reference = const fraction<dtype>&;

        dtype a;
        dtype b;

        fraction() noexcept {
            a = 0;
            b = 1;
        }

        fraction(const dtype& _a, const dtype& _b) {
            this->a = _a;
            this->b = _b;
        }
    
        self_type operator+(const_reference other) const noexcept {
            auto lhs = *this;
            lhs += other;
            return lhs;
        }

        reference operator+=(const_reference other) noexcept {
            a = a * other.b + b * other.a;
            b *= other.b;
            shrink();
            return *this;
        }

        self_type operator-(const_reference other) const noexcept {
            auto lhs = *this;
            lhs -= other;
            return lhs;
        }

        reference operator-=(const_reference other) noexcept {
            a = a * other.b - b * other.a;
            b *= other.b;
            shrink();
            return *this;
        }

        self_type operator*(const_reference other) const noexcept {
            auto lhs = *this;
            lhs *= other;
            return lhs;
        }

        reference operator*=(const_reference other) noexcept {
            a *= other.a;
            b *= other.b;
            shrink();
            return *this;
        }

        self_type operator/(const_reference other) const noexcept {
            auto lhs = *this;
            lhs /= other;
            return lhs;
        }

        reference operator/=(const_reference other) noexcept {
            auto r = other.reciprocal();
            a *= r.a;
            b *= r.b;
            shrink();
            return *this;
        }

        self_type reciprocal() const noexcept {
            auto copy = *this;
            std::swap(copy.a, copy.b);
            return copy;
        }

        void shrink() noexcept {
            auto g = std::gcd(a, b);
            a /= g;
            b /= g;
        }

        std::string str() const noexcept {
            std::stringstream res;
            res << a;
            res << "/";
            res << b;
            return res.str();
        }

        auto value() const noexcept {
            return nfloats<sizeof(dtype) * byte_size>(a) / nfloats<sizeof(dtype) * byte_size>(b);
        }

        friend std::ostream& operator<<(std::ostream& os, const fraction& val) noexcept {
            os << val.str();
            return os;
        }
    };
}

// formatter
template<class DType>
struct std::formatter<exlib::fraction<DType>> : formatter<std::string>{
    auto format(const auto& f, auto& ctx) const {
        return std::format_to(ctx.out(), "{}/{}", f.a.str(), f.b.str());
    }
};