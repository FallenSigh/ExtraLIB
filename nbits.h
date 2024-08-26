#pragma once
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <ostream>
#include <string>
#include <type_traits>

namespace zstl {

    template<std::size_t N>
    struct nbits_int {
        using bit = unsigned char;
        using iterator = bit*;
        using const_iterator = const bit*;
        
        bit _data[N];

        nbits_int() noexcept {
            std::fill(this->begin(), this->end(), 0);
        }

        nbits_int(const nbits_int<N>& other) noexcept {
            std::copy(other.begin(), other.end(), this->begin());
        }

        nbits_int(const nbits_int<N>&& other) noexcept {
            if (this != &other) {
                std::move(other.begin(), other.end(), this->data());
            }
        }

        template<std::size_t M>
        nbits_int(const nbits_int<M>& other) noexcept {
            // 保留符号位截断
            std::fill(std::begin(_data), std::end(_data), other.sign());

            for (std::size_t i = 0; i < std::min(N, M) - 1; i++) {
                _data[i] = other[i];
            }
        }

        template<typename T>
            requires std::is_integral_v<T>
        nbits_int(const T& x) noexcept {
            // 保留符号位截断
            std::fill(std::begin(_data), std::end(_data), (x >= 0) ? 0 : 1);
            for (std::size_t i = 0; i < N - 1; i++) {
                _data[i] = (x >> i & 1);
            }
        }

        template<typename T>
            requires std::is_integral_v<T>
        nbits_int<N>& operator=(const T& x) noexcept {
            // 保留符号位截断
            std::fill(std::begin(_data), std::end(_data), (x >= 0) ? 0 : 1);
            for (std::size_t i = 0; i < N - 1; i++) {
                _data[i] = (x >> i & 1);
            }
            return *this;
        }

        nbits_int<N>& operator=(nbits_int<N>&& other) noexcept {
            if (this != &other) {
                std::move(other.begin(), other.end(), this->data());
            }
            return *this;
        }

        template <std::size_t M>
        nbits_int<std::max(N, M)> operator+(const nbits_int<M>& other) noexcept {
            return _bitwise_add<M>(
            [this](std::size_t i) { return (i < N) ? _data[i] : this->sign(); },
            [&other](std::size_t i) { return (i < M) ? other[i] : other.sign(); });
        }

        template<typename T>
            requires std::is_integral_v<T>
        nbits_int<std::max(N, sizeof(T) * 8)> operator+(const T& other) noexcept {
            constexpr std::size_t M = sizeof(T) * 8;
            const bit other_sign = (other >> (M - 1) & 1);

            return _bitwise_add<M>(
            [this](std::size_t i) { return (i < N) ? _data[i] : this->sign(); },
            [&other, &other_sign](std::size_t i) { return (i < M) ? (other >> i & 1) : other_sign; });
        }

        template<typename T>
            requires std::is_integral_v<T>
        friend nbits_int<std::max(sizeof(T) * 8, N)> operator+(const T& lhs, const nbits_int<N>& rhs) noexcept {
            constexpr std::size_t M = sizeof(T) * 8;
            const bit lhs_sign = lhs >> (M - 1) & 1;

            return _bitwise_add<M>(
            [&lhs, &lhs_sign](std::size_t i) { return (i < M) ? (lhs >> i & 1) : lhs_sign; },
            [&rhs](std::size_t i) { return (i < N) ? rhs[i] : rhs.sign(); });
        }

        template<std::size_t M>
        nbits_int<std::max(N, M)> operator-(const nbits_int<M>& other) noexcept {
            return _bitwise_sub<M>(
            [this](std::size_t i) { return (i < N) ? _data[i] : this->sign(); },
            [&other](std::size_t i) { return (i < M) ? other[i] : other.sign(); });
        }

        template<typename T>
            requires std::is_integral_v<T>
        friend nbits_int<std::max(sizeof(T) * 8, N)> operator-(const T& lhs, const nbits_int<N>& rhs) noexcept {
            constexpr std::size_t M = sizeof(T) * 8;
            const bit lhs_sign = lhs >> (M - 1) & 1;

            return _bitwise_sub<M>(
            [&lhs, &lhs_sign](std::size_t i) { return (i < M) ? (lhs >> i & 1) : lhs_sign; },
            [&rhs](std::size_t i) { return (i < N) ? rhs[i] : rhs.sign(); });
        }

        template<typename T>
            requires std::is_integral_v<T>
        nbits_int<std::max(N, sizeof(T) * 8)> operator-(const T& other) noexcept {
            constexpr std::size_t M = sizeof(T) * 8;
            const bit other_sign = (other >> (M - 1) & 1);

            return _bitwise_sub<M>(
            [this](std::size_t i) { return (i < N) ? _data[i] : this->sign(); },
            [&other, &other_sign](std::size_t i) { return (i < M) ? (other >> i & 1) : other_sign; });
       
        }

        template<std::size_t M>
        nbits_int<std::max(N, M)> operator*(const nbits_int<M>& other) const noexcept {
            auto lhs_abs = nbits_int<std::max(N, M)>(std::move(this->abs()));
            auto rhs_abs = std::move(other.abs());

            nbits_int<std::max(N, M)> res = 0;
            for (std::size_t i = 0; i < M; i++) {
                if (rhs_abs[i]) {
                    res += (lhs_abs << i);
                }
            }
            if (this->sign() != other.sign()) {
                res = ~res + nbits_int<std::max(N, M)>(1);
            }
            return res;
        }

        template<typename T>
            requires std::is_integral_v<T>
        nbits_int<std::max(N, sizeof(T) * 8)> operator*(const T& val) const noexcept {
            return *this * nbits_int<sizeof(T) * 8>(val);
        }

        template<typename T>
            requires std::is_integral_v<T>
        friend nbits_int<std::max(sizeof(T) * 8, N)> operator*(const T& lhs, const nbits_int<N>& rhs) noexcept {
            return nbits_int<sizeof(T) * 8>(lhs) * rhs;
        }

        template<std::size_t M>
        nbits_int<std::max(N, M)> operator/(const nbits_int<M>& other) const {
            if (other == 0) {
                throw std::runtime_error("divided by zero!");
            }

            auto lhs_abs = std::move(this->abs());
            auto rhs_abs = std::move(other.abs());

            nbits_int<std::max(N, M)> quotient = 0;
            nbits_int<std::max(N, M)> remainder = 0;

            for (std::size_t i = 0; i < N; i++) {
                remainder <<= 1;
                remainder[0] = lhs_abs[N - 1- i];

                if (remainder >= rhs_abs) {
                    remainder = remainder - rhs_abs;
                    quotient[N - 1 - i] = 1;
                }
            }

            if (this->sign() != other.sign()) {
                quotient = ~quotient + nbits_int<std::max(N, M)>(1);
            }

            return quotient;
        }

        template<typename T>
            requires std::is_integral_v<T>
        nbits_int<std::max(N, sizeof(T) * 8)> operator/(const T& val) const {
            return *this / nbits_int<sizeof(T) * 8>(val);
        }

        template<typename T>
            requires std::is_integral_v<T>
        friend nbits_int<std::max(sizeof(T) * 8, N)> operator/(const T& lhs, const nbits_int<N>& rhs) noexcept {
            return nbits_int<sizeof(T) * 8>(lhs) / rhs;
        }

        template<std::size_t M>
        nbits_int<std::max(N, M)> operator%(const nbits_int<M>& other) const {
            if (other == 0) {
                throw std::runtime_error("divided by zero!");
            }

            auto lhs_abs = std::move(this->abs());
            auto rhs_abs = std::move(other.abs());

            nbits_int<std::max(N, M)> remainder = 0;

            for (std::size_t i = 0; i < N; i++) {
                remainder <<= 1;
                remainder[0] = lhs_abs[N - 1- i];

                if (remainder >= rhs_abs) {
                    remainder = remainder - rhs_abs;
                }
            }

            if (this->sign() != other.sign()) {
                remainder = ~remainder + nbits_int<std::max(N, M)>(1);
            }

            return remainder;
        }

        template<typename T>
            requires std::is_integral_v<T>
        nbits_int<std::max(N, sizeof(T) * 8)> operator%(const T& val) const {
            return *this % nbits_int<sizeof(T) * 8>(val);
        }

        template<typename T>
            requires std::is_integral_v<T>
        friend nbits_int<std::max(sizeof(T) * 8, N)> operator%(const T& lhs, const nbits_int<N>& rhs) noexcept {
            return nbits_int<sizeof(T) * 8>(lhs) % rhs;
        }

        template<std::size_t M>
        nbits_int<N>& operator+=(const nbits_int<M>& other) noexcept {
            return _bitwise_add_assign<M>(
            [this](std::size_t i) { return (i < N) ? _data[i] : this->sign(); },
            [&other](std::size_t i) { return (i < M) ? other[i] : other.sign(); });
        }

        template<typename T>
            requires std::is_integral_v<T>
        nbits_int<N>& operator+=(const T& val) noexcept {
            constexpr std::size_t M = sizeof(T) * 8;
            const bit val_sign = (val >> (M - 1) & 1);

            return _bitwise_add_assign<M>(
            [this](std::size_t i) { return (i < N) ? _data[i] : this->sign(); },
            [&val, &val_sign](std::size_t i) { return (i < M) ? (val >> i & 1) : val_sign; });
        }

        template<std::size_t M>
        nbits_int<N>& operator-=(const nbits_int<M>& other) noexcept {
            return _bitwise_sub_assign<M>(
            [this](std::size_t i) { return (i < N) ? _data[i] : this->sign(); },
            [&other](std::size_t i) { return (i < M) ? other[i] : other.sign(); });
        }

        template<typename T>
            requires std::is_integral_v<T>
        nbits_int<N>& operator-=(const T& val) noexcept {
            constexpr std::size_t M = sizeof(T) * 8;
            const bit val_sign = (val >> (M - 1) & 1);

            return _bitwise_add_assign<M>(
            [this](std::size_t i) { return (i < N) ? _data[i] : this->sign(); },
            [&val, &val_sign](std::size_t i) { return (i < M) ? (val >> i & 1) : val_sign; });
        }

        template<std::size_t M>
        nbits_int<N>& operator*=(const nbits_int<M>& other) noexcept {
            auto lhs_abs = std::move(this->abs());
            auto rhs_abs = std::move(other.abs());
            
            nbits_int<N> result = 0;
            for (std::size_t i = 0; i < M; ++i) {
                if (rhs_abs[i]) {
                    result += (lhs_abs << i);
                }
            }

            if (this->sign() != other.sign()) {
                result = ~result + 1;
            }
            *this = std::move(result);
            return *this;
        }

        template<typename T>
            requires std::is_integral_v<T>
        nbits_int<N>& operator*=(const T& val) noexcept {
            *this *= nbits_int<sizeof(T) * 8>(val);
            return *this;
        }

        template<std::size_t M>
        nbits_int<N>& operator/=(const nbits_int<M>& other) {
            if (other == 0) {
                throw std::runtime_error("divided by zero!");
            }
            auto lhs_abs = std::move(this->abs());
            auto rhs_abs = std::move(other.abs());

            nbits_int<N> quotient = 0;
            nbits_int<N> remainder = 0;

            for (std::size_t i = 0; i < N; i++) {
                remainder <<= 1;
                remainder[0] = lhs_abs[N - 1- i];

                if (remainder >= rhs_abs) {
                    remainder = remainder - rhs_abs;
                    quotient[N - 1 - i] = 1;
                }
            }

            if (this->sign() != other.sign()) {
                quotient = ~quotient + nbits_int<std::max(N, M)>(1);
            }

            *this = std::move(quotient);
            return *this;
        }

        template<typename T>
            requires std::is_integral_v<T>
        nbits_int<N>& operator/=(const T& val) {
            *this /= nbits_int<sizeof(T) * 8>(val);
            return *this;
        }

        template<std::size_t M>
        nbits_int<N>& operator%=(const nbits_int<M>& other) {
            if (other == 0) {
                throw std::runtime_error("divided by zero!");
            }

            auto lhs_abs = std::move(this->abs());
            auto rhs_abs = std::move(other.abs());

            nbits_int<N> remainder = 0;

            for (std::size_t i = 0; i < N; i++) {
                remainder <<= 1;
                remainder[0] = lhs_abs[N - 1- i];

                if (remainder >= rhs_abs) {
                    remainder = remainder - rhs_abs;
                }
            }

            if (this->sign() != other.sign()) {
                remainder = ~remainder + nbits_int<std::max(N, M)>(1);
            }

            *this = std::move(remainder);
            return *this;
        }

        template<typename T>
            requires std::is_integral_v<T>
        nbits_int<N>& operator%=(const T& val) {
            *this %= nbits_int<sizeof(T) * 8>(val);
            return *this;
        }

        nbits_int<N> operator~() const noexcept {
            auto copy = *this;
            for (std::size_t i = 0; i < N; i++) {
                copy[i] = !copy[i];
            }
            return copy;
        }

        template<std::size_t M>
        nbits_int<N> operator&(const nbits_int<M>& other) const noexcept {
            auto copy = *this;
            for (std::size_t i = 0; i < std::max(N, M); i++) {
                bit lbit = (i < N) ? _data[i] : this->sign();
                bit rbit = (i < M) ? other[i] : other.sign();
                if (i < N) {
                    copy[i] = lbit & rbit;
                }
            }
            return copy;
        }

        template<std::size_t M>
        nbits_int<N>& operator&=(const nbits_int<M>& other) noexcept {
            for (std::size_t i = 0; i < N; i++) {
                _data[i] &= (i < M) ? other[i] : other.sign();
            }
            return *this;
        }

        template<std::size_t M>
        nbits_int<N> operator|(const nbits_int<M>& other) const noexcept {
            auto copy = *this;
            for (std::size_t i = 0; i < std::max(N, M); i++) {
                bit lbit = (i < N) ? _data[i] : this->sign();
                bit rbit = (i < M) ? other[i] : other.sign();
                if (i < N) {
                    copy[i] = lbit | rbit;
                }
            }
            return copy;
        }

        template<std::size_t M>
        nbits_int<N>& operator|=(const nbits_int<M>& other) noexcept {
            for (std::size_t i = 0; i < N; i++) {
                _data[i] |= (i < M) ? other[i] : other.sign();
            }
            return *this;
        }

        template<std::size_t M>
        nbits_int<N> operator^(const nbits_int<M>& other) const noexcept {
            auto copy = *this;
            for (std::size_t i = 0; i < std::max(N, M); i++) {
                bit lbit = (i < N) ? _data[i] : this->sign();
                bit rbit = (i < M) ? other[i] : other.sign();
                if (i < N) {
                    copy[i] = lbit ^ rbit;
                }
            }
            return copy;
        }

        template<std::size_t M>
        nbits_int<N>& operator^=(const nbits_int<M>& other) noexcept {
            for (std::size_t i = 0; i < N; i++) {
                _data[i] ^= (i < M) ? other[i] : other.sign();
            }
            return *this;
        }

        template<std::size_t M>
        bool operator==(const nbits_int<M>& other) const noexcept {
            if (this->sign() != other.sign()) {
                return false;
            }

            for (std::size_t i = std::max(N, M); ~i; i--) {
                bit lbit = (i < N) ? _data[i] : this->sign();
                bit rbit = (i < M) ? other[i] : other.sign();

                if (lbit != rbit) {
                    return false;
                }
            }
            return true;
        }

        template<typename T>
            requires std::is_integral_v<T>
        bool operator==(const T& val) const noexcept {
            return *this == nbits_int<sizeof(T) * 8>(val);
        }

        template<std::size_t M>
        bool operator!=(const nbits_int<M>& other) const noexcept {
            return !(*this == other);
        }

        template<typename T>
            requires std::is_integral_v<T>
        bool operator!=(const T& val) const noexcept {
            return *this != nbits_int<sizeof(T) * 8>(val);
        }

        template<std::size_t M>
        bool operator<(const nbits_int<M>& other) const noexcept {
            if (this->sign() != other.sign()) {
                return this->sign() == 1;
            }

            for (std::size_t i = std::max(N, M); ~i; i--) {
                bit lbit = (i < N) ? _data[i] : this->sign();
                bit rbit = (i < M) ? other[i] : other.sign();

                if (lbit != rbit) {
                    return lbit < rbit;
                }
            }
            return false;
        }

        template<typename T>
            requires std::is_integral_v<T>
        bool operator<(const T& val) const noexcept {
            return *this < nbits_int<sizeof(T) * 8>(val);
        }

        template<std::size_t M>
        bool operator<=(const nbits_int<M>& other) const noexcept {
            return *this == other || *this < other;
        }

        template<typename T>
            requires std::is_integral_v<T>
        bool operator<=(const T& val) const noexcept {
            return *this <= nbits_int<sizeof(T) * 8>(val);
        }

        template<std::size_t M>
        bool operator>(const nbits_int<M>& other) const noexcept {
            if (this->sign() != other.sign()) {
                return this->sign() == 0;
            }

            for (std::size_t i = std::max(N, M); ~i; i--) {
                bit lbit = (i < N) ? _data[i] : sign();
                bit rbit = (i < M) ? other[i] : other.sign();

                if (lbit != rbit) {
                    return lbit > rbit;
                }
            }
            return false;
        }

        template<typename T>
            requires std::is_integral_v<T>
        bool operator>(const T& val) const noexcept {
            return *this > nbits_int<sizeof(T) * 8>(val);
        }

        template<std::size_t M>
        bool operator>=(const nbits_int<M>& other) const noexcept {
            return *this == other || *this > other;
        }

        template<typename T>
            requires std::is_integral_v<T>
        bool operator>=(const T& val) const noexcept {
            return *this >= nbits_int<sizeof(T) * 8>(val);
        }

        nbits_int<N> operator<<(std::size_t x) const noexcept {
            auto copy = *this;
            if (x >= N) {
                std::fill(copy.begin(), copy.end(), 0);
            } else {
                for (std::size_t i = N - 1; ~i; i--) {
                    copy[i] = (i < x) ? 0 : _data[i - x];
                }
            }
            return copy;
        }

        nbits_int<N>& operator<<=(std::size_t x) noexcept {
            if (x >= N) {
                std::fill(begin(), end(), 0);
            } else {
                for (std::size_t i = N - 1; ~i; i--) {
                    _data[i] = (i < x) ? 0 : _data[i - x];
                }
            }
            return *this;
        }

        nbits_int<N> operator>>(std::size_t x) const noexcept {
            auto copy = *this;
            if (x >= N) {
                std::fill(copy.begin(), copy.end(), sign());
            } else {
                for (std::size_t i = 0; i < N; i++) {
                    copy[i] = (i + x < N) ? _data[i + x] : sign();
                }
            }   
            return copy;
        }

        nbits_int<N>& operator>>=(std::size_t x) noexcept {
            if (x >= N) {
                std::fill(begin(), end(), sign());
            } else {
                for (std::size_t i = 0; i < N; i++) {
                    _data[i] = (i + x < N) ? _data[i + x] : sign();
                }
            }   
            return *this;
        }

        constexpr bit* data() const noexcept {
            return _data;
        }

        bit* data() noexcept {
            return _data;
        }

        nbits_int<N> abs() const noexcept {
            return (sign() == 0) ? *this : ~(*this) + nbits_int<N>(1);
        }

        bit sign() const noexcept {
            return _data[N - 1];
        }

        constexpr std::size_t size() const noexcept {
            return N;
        }

        bit& at(std::size_t idx) {
            if (idx >= N) [[unlikely]]{
                throw std::out_of_range("");
            }
            return _data[idx];
        }

        bit at(std::size_t idx) const {
            if (idx >= N) [[unlikely]]{
                throw std::out_of_range("");
            }
            return _data[idx];
        }

        bit& operator[](std::size_t idx) noexcept {
            return _data[idx];
        }

        bit operator[](std::size_t idx) const noexcept {
            return _data[idx];
        }

        iterator begin() noexcept {
            return _data;
        }

        iterator end() noexcept {
            return _data + N;
        }

        const_iterator begin() const noexcept {
            return _data;
        }

        const_iterator end() const noexcept {
            return _data + N;
        }

        const_iterator cbegin() const noexcept {
            return _data + N;
        }

        const_iterator cend() const noexcept {
            return _data;
        }

        friend std::ostream& operator<<(std::ostream& os, const nbits_int& val) noexcept {
            std::string str;
            for (std::size_t i = val.size() - 1; ~i; i--) {
                str.push_back(val[i] + '0');
            }
            os << str;
            return os;
        }

        long long dec() const noexcept {
            long long base = 1;
            long long res = 0;

            for (std::size_t i = 0; i < N - 1; i++) {
                res += base * _data[i];
                base *= 2;
            }

            res -= base * _data[N - 1];
            return res;
        }

        static std::pair<bit, bit> _full_add(bit x, bit y, bit c) noexcept {
            bit sum = x ^ y ^ c;
            bit carry = (x & y) | ((x ^ y) & c);
            return {sum, carry};
        }

        static std::pair<bit, bit> _full_sub(bit x, bit y, bit b) noexcept {
            bit diff = x ^ y ^ b;
            bit borrow = (!x & (y | b)) | (y & b);
            return {diff, borrow};
        }

        template<std::size_t M, typename LBitFunc, typename RBitFunc>
        inline static nbits_int<std::max(N, M)> _bitwise_add(LBitFunc lbitfunc, RBitFunc rbitfunc) noexcept {
            // 使用位宽扩展 溢出不作处理
            nbits_int<std::max(N, M)> res;
            bit carry = 0;
            
            for (std::size_t i = 0; i < std::max(N, M); i++) {
                auto [sum, car] = _full_add(lbitfunc(i), rbitfunc(i), carry);
                carry = car;
                res[i] = sum;
            }
            return res;
        }

        template<std::size_t M, typename LBitFunc, typename RBitFunc>
        inline auto&& _bitwise_add_assign(LBitFunc lbitfunc, RBitFunc rbitfunc) noexcept {
            // 使用位宽扩展，溢出时不作处理
            bit carry = 0;
            
            for (std::size_t i = 0; i < std::max(N, M); i++) {
                auto [sum, car] = _full_add(lbitfunc(i), rbitfunc(i), carry);
                carry = car;
                if (i < N) {
                    _data[i] = sum;
                }
            }
            return *this;
        }

        template<std::size_t M, typename LBitFunc, typename RBitFunc>
        inline static nbits_int<std::max(N, M)> _bitwise_sub(LBitFunc lbitfunc, RBitFunc rbitfunc) noexcept {
            // 使用位宽扩展 溢出不作处理
            nbits_int<std::max(N, M)> res;
            bit borrow = 0;
            
            for (std::size_t i = 0; i < std::max(N, M); i++) {
                auto [diff, borr] = _full_sub(lbitfunc(i), rbitfunc(i), borrow);
                borrow = borr;
                res[i] = diff;
            }
            return res;
        }

        template<std::size_t M, typename LBitFunc, typename RBitFunc>
        inline auto&& _bitwise_sub_assign(LBitFunc lbitfunc, RBitFunc rbitfunc) noexcept {
            // 使用位宽扩展，溢出时不作处理
            bit borrow = 0;
            
            for (std::size_t i = 0; i < std::max(N, M); i++) {
                auto [diff, borr] = _full_sub(lbitfunc(i), rbitfunc(i), borrow);
                borrow = borr;
                if (i < N) {
                    _data[i] = diff;
                }
            }
            return *this;
        }
    };
}
