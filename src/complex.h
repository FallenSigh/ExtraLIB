#pragma once
#include <format>
#include <string>

namespace exlib {
    template<class Dtype>
    struct complex {
        using dtype = Dtype;
        using self_type = complex<dtype>;
        using reference = complex<dtype>&;
        using const_reference = const complex<dtype>&;

        // a + ib
        dtype a;
        dtype b;

        complex() noexcept {
            a = 0;
            b = 0;
        }

        complex(const dtype& _a, const dtype& _b) {
            this->a = _a;
            this->b = _b;
        }

        reference operator+=(const_reference other) noexcept {
            a += other.a;
            b += other.b;
            return *this;
        }

        reference operator-=(const_reference other) noexcept {
            a -= other.a;
            b -= other.b;
            return *this;
        }

        reference operator*=(const_reference other) noexcept {
            dtype t = a * other.a - b * other.b;
            b = a * other.b + b * other.a;
            a = t;
            return *this;
        }

        reference operator/=(const_reference other) noexcept {
            dtype f = other.a * other.a + other.b * other.b;
            dtype t = a * other.a + b * other.b;
            b = b * other.a - a * other.b;
            a = t;
            a /= f;
            b /= f;
            return *this;
        }

        self_type operator+(const_reference other) const noexcept {
            auto copy = *this;
            copy += other;
            return copy;
        }

        self_type operator-(const_reference other) const noexcept {
            auto copy = *this;
            copy -= other;
            return copy;
        }

        self_type operator/(const_reference other) const noexcept {
            auto copy = *this;
            copy /= other;
            return copy;
        }

        self_type operator*(const_reference other) const noexcept {
            auto copy = *this;
            copy *= other;
            return copy;
        }

        std::string str() const noexcept {
            return a.str() + " + i" + b.str();
        }

        friend std::ostream& operator<<(std::ostream& os, const auto& val) noexcept {
            os << val.str();
            return os;
        }
    };
}

// formatter
template<class DType>
struct std::formatter<exlib::complex<DType>> : formatter<typename exlib::complex<DType>::dtype>{
    auto format(const auto& f, auto& ctx) const {
        return std::format_to(ctx.out(), "{} + i{}", f.a.str(), f.b.str());
    }
};