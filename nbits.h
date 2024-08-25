#pragma once
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <ostream>
#include <ratio>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace zstl {

    template<std::size_t N>
    struct nbits_int {
        using bit = bool;
        using iterator = bit*;
        using const_iterator = const bit*;
        using type = nbits_int<N>;
        
        bit _data[N];

        nbits_int() noexcept {
            std::fill(std::begin(_data), std::end(_data), 0);
        }

        nbits_int(const nbits_int<N>& other) noexcept {
            std::copy(other.begin(), other.end(), this->begin());
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

        template <std::size_t M>
        nbits_int<std::max(N, M)> operator+(const nbits_int<M>& other) noexcept {
            // 使用位宽扩展 溢出不作处理
            nbits_int<std::max(N, M)> res;
            bit carry = 0;
            
            for (std::size_t i = 0; i < std::max(N, M); i++) {
                bit lbit = (i < N) ? _data[i] : this->sign();
                bit rbit = (i < M) ? other[i] : other.sign();

                auto [sum, car] = _full_add(lbit, rbit, carry);
                carry = car;
                res[i] = sum;
            }
            return res;
        }

        template<typename T>
            requires std::is_integral_v<T>
        nbits_int<std::max(N, sizeof(T) * 8)> operator+(const T& other) noexcept {
            return *this + nbits_int<sizeof(T) * 8>(other);
        }

        template<typename T>
            requires std::is_integral_v<T>
        friend nbits_int<std::max(sizeof(T) * 8, N)> operator+(const T& lhs, const nbits_int<N>& rhs) noexcept {
            return nbits_int<sizeof(T) * 8>(lhs) + rhs;
        }

        template<std::size_t M>
        nbits_int<std::max(N, M)> operator-(const nbits_int<M>& other) noexcept {
            return *this + (~other + 1);
        }

        template<typename T>
            requires std::is_integral_v<T>
        friend nbits_int<std::max(sizeof(T) * 8, N)> operator-(const T& lhs, const nbits_int<N>& rhs) noexcept {
            return nbits_int<sizeof(T) * 8>(lhs) - rhs;
        }

        template<typename T>
            requires std::is_integral_v<T>
        nbits_int<std::max(N, sizeof(T) * 8)> operator-(const T& other) noexcept {
            return *this - nbits_int<sizeof(T) * 8>(other);
        }

        template<std::size_t M>
        nbits_int<std::max(N, M)> operator*(const nbits_int<M>& other) const noexcept {
            auto lhs_abs = this->abs();
            auto rhs_abs = other.abs();

            nbits_int<std::max(N, M)> res = 0;
            for (std::size_t i = 0; i < M; i++) {
                if (rhs_abs[i]) {
                    res += (nbits_int<std::max(N, M)>(lhs_abs) << i);
                }
            }
            if (this->sign() != other.sign()) {
                res = ~res + 1;
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

            nbits_int<std::max(N, M)> quotient = 0;
            nbits_int<std::max(N, M)> remainder = 0;

            for (std::size_t i = 0; i < N; i++) {
                remainder <<= 1;
                remainder[0] = this->_data[N - 1- i];

                if (remainder >= other) {
                    remainder = remainder - other;
                    quotient[N - 1 - i] = 1;
                }
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

            nbits_int<std::max(N, M)> remainder = 0;

            for (std::size_t i = 0; i < N; i++) {
                remainder <<= 1;
                remainder[0] = this->_data[N - 1- i];

                if (remainder >= other) {
                    remainder = remainder - other;
                }
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
            // 使用位宽扩展，溢出时不作处理
            bit carry = 0;
            
            for (std::size_t i = 0; i < std::max(N, M); i++) {
                bit lbit = (i < N) ? _data[i] : this->sign();
                bit rbit = (i < M) ? other._data[i] : other.sign();
                auto [sum, car] = _full_add(lbit, rbit, carry);
                carry = car;
                if (i < N) {
                    _data[i] = sum;
                }
            }
            return *this;
        }

        template<typename T>
            requires std::is_integral_v<T>
        nbits_int<N>& operator+=(const T& val) noexcept {
            // 使用位宽扩展，溢出时不作处理
            *this += nbits_int<sizeof(T) * 8>(val);
            return *this;
        }

        template<std::size_t M>
        nbits_int<N>& operator-=(const nbits_int<M>& other) noexcept {
            *this += (~other + 1);
            return *this;
        }

        template<typename T>
            requires std::is_integral_v<T>
        nbits_int<N>& operator-=(const T& val) noexcept {
            *this += -val;
            return *this;
        }

        template<std::size_t M>
        nbits_int<N>& operator*=(const nbits_int<M>& other) noexcept {
            *this = *this * other;
            return *this;
        }

        template<typename T>
            requires std::is_integral_v<T>
        nbits_int<N>& operator*=(const T& val) noexcept {
            *this = *this * val;
            return *this;
        }

        template<std::size_t M>
        nbits_int<N>& operator/=(const nbits_int<M>& other) noexcept {
            *this = *this / other;
            return *this;
        }

        template<typename T>
            requires std::is_integral_v<T>
        nbits_int<N>& operator/=(const T& val) noexcept {
            *this = *this / val;
            return *this;
        }

        template<std::size_t M>
        nbits_int<N>& operator%=(const nbits_int<M>& other) noexcept {
            *this = *this % other;
            return *this;
        }

        template<typename T>
            requires std::is_integral_v<T>
        nbits_int<N>& operator%=(const T& val) noexcept {
            *this = *this % val;
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

        nbits_int<N> abs() const noexcept {
            return (sign() == 0) ? *this : ~(*this) + nbits_int<N>(1);
        }

        bit sign() const noexcept {
            return _data[N - 1];
        }

        constexpr std::size_t size() const noexcept {
            return N;
        }

        bit& operator[](std::size_t idx) {
            if (idx >= N) [[unlikely]]{
                throw std::out_of_range("");
            }
            return _data[idx];
        }

        bit operator[](std::size_t idx) const {
            if (idx >= N) [[unlikely]]{
                throw std::out_of_range("");
            }
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

        static std::pair<bit, bit> _full_add(bit a, bit b, bit c) noexcept {
            bit sum = a ^ b ^ c;
            bit carry = (a & b) | ((a ^ b) & c);
            return {sum, carry};
        }
    };
}
